/*
 * ex08_teach_and_play.cpp
 *
 *  Created on: Sep 29, 2009
 *      Author: dc
 */

#include <iostream>
#include <vector>
#include <string>

#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/tuple/tuple.hpp>

#include <barrett/detail/stl_utils.h>  // waitForEnter()
#include <barrett/math.h>
#include <barrett/units.h>
#include <barrett/systems.h>
#include <barrett/log.h>
#include <barrett/products/product_manager.h>

#include <barrett/standard_main_function.h>


using namespace barrett;
using detail::waitForEnter;
using systems::connect;
using systems::disconnect;
using systems::reconnect;



template<size_t DOF>
int wam_main(int argc, char** argv, ProductManager& pm, systems::Wam<DOF>& wam) {
	BARRETT_UNITS_TEMPLATE_TYPEDEFS(DOF);
	typedef boost::tuple<double, jp_type> jp_sample_type;
	typedef boost::tuple<double, double> bo_tu_type;

	char tmpFile[] = "/home/robot/libbarrett_examples/btvlwW4T";
	/*
	if (mkstemp(tmpFile) == -1) {
		printf("ERROR: Couldn't create temporary file!\n");
		return 1;
	}
	
	const double T_s = pm.getExecutionManager()->getPeriod();


	wam.gravityCompensate();

	systems::Ramp time(pm.getExecutionManager());

	systems::TupleGrouper<double, jp_type> jpLogTg;

	// Record at 1/10th of the loop rate
	systems::PeriodicDataLogger<jp_sample_type> jpLogger(pm.getExecutionManager(),
			new barrett::log::RealTimeWriter<jp_sample_type>(tmpFile, 10*T_s), 10);


	printf("Press [Enter] to start teaching.\n");
	waitForEnter();
	{
		// Make sure the Systems are connected on the same execution cycle
		// that the time is started. Otherwise we might record a bunch of
		// samples all having t=0; this is bad because the Spline requires time
		// to be monotonic.
		BARRETT_SCOPED_LOCK(pm.getExecutionManager()->getMutex());

		connect(time.output, jpLogTg.template getInput<0>());
		connect(wam.jpOutput, jpLogTg.template getInput<1>());
		connect(jpLogTg.output, jpLogger.input);
		time.start();
	}

	printf("Press [Enter] to stop teaching.\n");
	waitForEnter();
	jpLogger.closeLog();
	disconnect(jpLogger.input);
	*/

	// Build spline between recorded points
	log::Reader<jp_sample_type> lr(tmpFile);
	std::vector<jp_sample_type> vec;
	for (size_t i = 0; i < lr.numRecords(); ++i) {
		vec.push_back(lr.getRecord());
	}

	// Read joint 1 of 1st reading
	jp_sample_type first_red = vec[0];
	// bo_tu_type example(3.14,3.15);
	// std::cout << "Current time: " << example.get<0>() << " sec." << std::endl;
	// Seperate out the tupple
	// Time
	double curr_t = boost::get<0>(first_red);
	std::cout << "Current time: " << curr_t << " sec." << std::endl;
	// Joint state
	jp_type curr_state = boost::get<1>(first_red);
	// What is the current system DOF?
	std::cout << "Current system DOF: " << DOF << " -." << std::endl;
	// Joint 1 reading
	std::cout << "Current joint 1 state: " << curr_state[0] << " rad." << std::endl;
	// Joint 2 reading
	std::cout << "Current joint 2 state: " << curr_state[1] << " rad." << std::endl;
	// Joint 3 reading
	std::cout << "Current joint 3 state: " << curr_state[2] << " rad." << std::endl;
	// Joint 4 reading
	std::cout << "Current joint 4 state: " << curr_state[3] << " rad." << std::endl;
	// Joint 5 reading
	std::cout << "Current joint 5 state: " << curr_state[4] << " rad." << std::endl;
	// Joint 6 reading
	std::cout << "Current joint 6 state: " << curr_state[5] << " rad." << std::endl;
	// Joint 7 reading
	std::cout << "Current joint 7 state: " << curr_state[6] << " rad." << std::endl;
	/*
	math::Spline<jp_type> spline(vec);

	printf("Press [Enter] to play back the recorded trajectory.\n");
	waitForEnter();

	// First, move to the starting position
	wam.moveTo(spline.eval(spline.initialS()));

	// Then play back the recorded motion
	time.stop();
	time.setOutput(spline.initialS());

	systems::Callback<double, jp_type> trajectory(boost::ref(spline));
	connect(time.output, trajectory.input);
	wam.trackReferenceSignal(trajectory.output);

	time.start();

	while (trajectory.input.getValue() < spline.finalS()) {
		usleep(100000);
	}


	printf("Press [Enter] to idle the WAM.\n");
	waitForEnter();
	wam.idle();


	//std::remove(tmpFile);
	pm.getSafetyModule()->waitForMode(SafetyModule::IDLE);
	*/
	return 0;
}
