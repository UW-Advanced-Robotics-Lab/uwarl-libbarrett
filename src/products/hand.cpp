/**
 *	Copyright 2009-2014 Barrett Technology <support@barrett.com>
 *
 *	This file is part of libbarrett.
 *
 *	This version of libbarrett is free software: you can redistribute it
 *	and/or modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, either version 3 of the
 *	License, or (at your option) any later version.
 *
 *	This version of libbarrett is distributed in the hope that it will be
 *	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this version of libbarrett.  If not, see
 *	<http://www.gnu.org/licenses/>.
 *
 *
 *	Barrett Technology Inc.
 *	73 Chapel Street
 *	Newton, MA 02458
 *
 */

/**
 * @file hand.cpp
 * @date 11/09/2010
 * @author Dan Cody
 * 
 */

#include <stdexcept>
#include <vector>
#include <algorithm>
#include <limits>

#include <boost/thread/locks.hpp>

#include <barrett/os.h>
#include <barrett/detail/stl_utils.h>
#include <barrett/products/abstract/multi_puck_product.h>
#include <barrett/products/puck.h>
#include <barrett/products/puck_group.h>
#include <barrett/products/motor_puck.h>
#include <barrett/products/tactile_puck.h>
#include <barrett/products/hand.h>


namespace barrett {


const enum Puck::Property Hand::props[] = { Puck::HOLD, Puck::CMD, Puck::MODE, Puck::P, Puck::T, Puck::SG };

/** Hand Constructor */
Hand::Hand(const std::vector<Puck*>& _pucks) :
	MultiPuckProduct(DOF, _pucks, PuckGroup::BGRP_HAND, props, sizeof(props)/sizeof(props[0]), "Hand::Hand()"),
	hasFtt(false), hasTact(false), useSecondaryEncoders(true), encoderTmp(DOF), primaryEncoder(DOF, 0), secondaryEncoder(DOF, 0), ftt(DOF, 0), tactilePucks()
{
	// Check for TACT and FingertipTorque options.
	int numFtt = 0;
	for (size_t i = 0; i < DOF; ++i) {
		if (pucks[i]->hasOption(Puck::RO_Strain)) {
			++numFtt;
			hasFtt = true;
		}
	}
	logMessage("  Found %d Fingertip torque sensors") % numFtt;

	bool tactError = false;
	for (size_t i = 0; i < DOF; ++i) {
		if (pucks[i]->hasOption(Puck::RO_Tact)) {
			try {
				// The TactilePuck ctor might throw if there was an initialization error
				tactilePucks.push_back(new TactilePuck(pucks[i]));
				hasTact = true;
			} catch (std::runtime_error e) {
				tactError = true;
			}
		}
	}
	logMessage("  Found %d Tactile arrays") % tactilePucks.size();
	if (tactError) {
		logMessage("  Initialization error! Disabling Tactile arrays");
		hasTact = false;
	}

	// record HOLD values
	group.getProperty(Puck::HOLD, holds);


	// For the fingers
	for (size_t i = 0; i < DOF - 1; ++i) {
		j2pp[i] = motorPucks[i].getCountsPerRad() * J2_RATIO;
		j2pt[i] = motorPucks[i].getIpnm() / J2_RATIO;
	}
	// For the spread
	j2pp[SPREAD_INDEX] = motorPucks[SPREAD_INDEX].getCountsPerRad() * SPREAD_RATIO;
	j2pt[SPREAD_INDEX] = motorPucks[SPREAD_INDEX].getIpnm() / SPREAD_RATIO;
}
/** Hand Destructor */
Hand::~Hand()
{
	detail::purge(tactilePucks);
}
/** initialize Method */
void Hand::initialize() const
{
	for (size_t i = 0; i < DOF-1; ++i) {
		pucks[i]->setProperty(Puck::CMD, CMD_HI);
	}
	btsleep(1.0); // Pucks take at least 0.5 seconds to respond to CAN messages again after cmdHI
	waitUntilDoneMoving();

	pucks[SPREAD_INDEX]->setProperty(Puck::CMD, CMD_HI);
	btsleep(1.0);
	waitUntilDoneMoving();
}
/** doneMoving Method */
bool Hand::doneMoving(unsigned int whichDigits, bool realtime) const
{
	int modes[DOF];

	// TODO(dc): Avoid asking for modes from the Pucks we don't care about.
	group.getProperty(Puck::MODE, modes, realtime);

	for (size_t i = 0; i < DOF; ++i) {
		if (
				digitsInclude(whichDigits, i)  &&
				modes[i] != MotorPuck::MODE_IDLE  &&
				(modes[i] != MotorPuck::MODE_PID  ||  holds[i] == 0)
				)
		{
			return false;
		}
	}
	return true;
}
/** waitUntilDoneMoving Method prevents any subsequent actions until finger movement is completed. */
void Hand::waitUntilDoneMoving(unsigned int whichDigits, double period_s) const
{
	while ( !doneMoving(whichDigits) ) {
		btsleep(period_s);
	}
}
/** open Method */
void Hand::open(unsigned int whichDigits, bool blocking) const {
	setProperty(whichDigits, Puck::CMD, CMD_OPEN);
	blockIf(blocking, whichDigits);
}
/** close Method */
void Hand::close(unsigned int whichDigits, bool blocking) const {
	setProperty(whichDigits, Puck::CMD, CMD_CLOSE);
	blockIf(blocking, whichDigits);
}
/** trapezoidalMove Method */
void Hand::trapezoidalMove(const jp_type& jp, unsigned int whichDigits, bool blocking) const
{
	setProperty(whichDigits, Puck::E, (j2pp.array() * jp.array()).matrix());
	setProperty(whichDigits, Puck::MODE, MotorPuck::MODE_TRAPEZOIDAL);
	blockIf(blocking, whichDigits);
}
/** velocityMove Method */
void Hand::velocityMove(const jv_type& jv, unsigned int whichDigits) const
{
	// Convert to counts/millisecond
	setProperty(whichDigits, Puck::V, (j2pp.array() * jv.array()).matrix() / 1000.0);
	setProperty(whichDigits, Puck::MODE, MotorPuck::MODE_VELOCITY);
}

/** setPositionMode Method */
void Hand::setPositionMode(unsigned int whichDigits) const {
	setProperty(whichDigits, Puck::MODE, MotorPuck::MODE_PID);
}
/** setPositionCommand Method */
void Hand::setPositionCommand(const jp_type& jp, unsigned int whichDigits) const
{
	setProperty(whichDigits, Puck::P, (j2pp.array() * jp.array()).matrix());
}
/** setTorqueMode Method */
void Hand::setTorqueMode(unsigned int whichDigits) const {
	setProperty(whichDigits, Puck::MODE, MotorPuck::MODE_TORQUE);
}
/** setTorqueCommand Method */
void Hand::setTorqueCommand(const jt_type& jt, unsigned int whichDigits) const
{
	pt = (j2pt.array() * jt.array()).matrix();
	if (whichDigits == WHOLE_HAND) {
		MotorPuck::sendPackedTorques(pucks[0]->getBus(), group.getId(), Puck::T, pt.data(), DOF);
	} else {
		setProperty(whichDigits, Puck::T, pt);
	}
}
/** update Method */
void Hand::update(unsigned int sensors, bool realtime)
{
	// Do we need to lock?
	//boost::unique_lock<thread::Mutex> ul(bus.getMutex(), boost::defer_lock);
	//if (realtime) {
	//	ul.lock();
	//}

	// Send requests
	if (sensors & S_POSITION) {
		{
			BARRETT_SCOPED_LOCK(bus.getMutex());
			group.sendGetPropertyRequest(group.getPropertyId(Puck::P));

			group.receiveGetPropertyReply<MotorPuck::CombinedPositionParser<int> >(group.getPropertyId(Puck::P), encoderTmp.data(), realtime);
		}
		boost::this_thread::yield();

		for (size_t i = 0; i < DOF; ++i) {
			primaryEncoder[i] = encoderTmp[i].get<0>();
			secondaryEncoder[i] = encoderTmp[i].get<1>();
		}
		// For the fingers
		for (size_t i = 0; i < DOF-1; ++i) {
			// If we got a reading from the secondary encoder and it's enabled...
			if (useSecondaryEncoders  &&  secondaryEncoder[i] != std::numeric_limits<int>::max()) {
				innerJp[i] = motorPucks[i].counts2rad(secondaryEncoder[i]) / J2_ENCODER_RATIO;
				outerJp[i] = motorPucks[i].counts2rad(primaryEncoder[i]) * (1.0/J2_RATIO + 1.0/J3_RATIO) - innerJp[i];
			} else {
				// These calculations are only valid before breakaway!
				innerJp[i] = motorPucks[i].counts2rad(primaryEncoder[i]) / J2_RATIO;
				outerJp[i] = innerJp[i] * J2_RATIO / J3_RATIO;
			}
		}

		// For the spread
		innerJp[SPREAD_INDEX] = outerJp[SPREAD_INDEX] = motorPucks[SPREAD_INDEX].counts2rad(primaryEncoder[SPREAD_INDEX]) / SPREAD_RATIO;
	}
	
	if (hasFingertipTorqueSensors()  &&  sensors & S_FINGERTIP_TORQUE) {
		{
			BARRETT_SCOPED_LOCK(bus.getMutex());
			group.sendGetPropertyRequest(group.getPropertyId(Puck::SG));

			group.receiveGetPropertyReply<Puck::StandardParser>(group.getPropertyId(Puck::SG), ftt.data(), realtime);
		}
		boost::this_thread::yield();
	}
	
	
	if (hasTactSensors()) {
		if(sensors & S_TACT_FULL){
			BARRETT_SCOPED_LOCK(bus.getMutex());
			// This should be TactilePuck::requestFull()
			group.setProperty(Puck::TACT, TactilePuck::FULL_FORMAT);

			for (size_t i = 0; i < tactilePucks.size(); ++i) {
				tactilePucks[i]->receiveFull(realtime);
			}
		}

		if (sensors & S_TACT_TOP10) {
			BARRETT_SCOPED_LOCK(bus.getMutex());
			// This should be TactilePuck::requestTop10()
			group.setProperty(Puck::TACT, TactilePuck::TOP10_FORMAT);

			for (size_t i = 0; i < tactilePucks.size(); ++i) {
				tactilePucks[i]->receiveTop10(realtime);
			}
		}
		boost::this_thread::yield();
	}
}

/** */
void Hand::setProperty(unsigned int whichDigits, enum Puck::Property prop, int value) const
{
	if (whichDigits == WHOLE_HAND) {
		group.setProperty(prop, value);
	} else {
		for (size_t i = 0; i < DOF; ++i) {
			if (digitsInclude(whichDigits, i)) {
				pucks[i]->setProperty(prop, value);
			}
		}
	}
}
/** setProperty Method */
void Hand::setProperty(unsigned int whichDigits, enum Puck::Property prop, const v_type& values) const
{
	for (size_t i = 0; i < DOF; ++i) {
		if (digitsInclude(whichDigits, i)) {
			pucks[i]->setProperty(prop, values[i]);
		}
	}
}
/** blockIf Method */
void Hand::blockIf(bool blocking, unsigned int whichDigits) const {
	if (blocking) {
		waitUntilDoneMoving(whichDigits);
	}
}


}
