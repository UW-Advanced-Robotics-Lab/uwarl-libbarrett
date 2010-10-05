/*
 * puck.h
 *
 *  Created on: Aug 20, 2010
 *      Author: dc
 */

#ifndef BARRETT_PUCK_H_
#define BARRETT_PUCK_H_


#include <stdexcept>
#include <syslog.h>
#include <barrett/bus/abstract/communications_bus.h>


namespace barrett {


class Puck {

public:
	enum PuckType {
		PT_Monitor, PT_Safety, PT_Motor, PT_ForceTorque, PT_Unknown
	};

// include the generated file containing the list of available properties
#	include <barrett/detail/property_list.h>

public:
	Puck(const CommunicationsBus& bus, int id, enum PuckType type = PT_Unknown);
	~Puck();

	int getProperty(enum Property prop) { return getProperty(bus, id, getPropertyId(prop)); }
	void setProperty(enum Property prop, int value) { setProperty(bus, id, getPropertyId(prop), value); }

	int getPropertyId(enum Property prop) throw(std::runtime_error) {
		return getPropertyId(prop, effectiveType, vers);
	}
	int getPropertyIdNoThrow(enum Property prop) {
		return getPropertyIdNoThrow(prop, effectiveType, vers);
	}

	static int getProperty(const CommunicationsBus& bus, int id, int propId);
	static void setProperty(const CommunicationsBus& bus, int id, int propId, int value);

	static int getPropertyId(enum Property prop, enum PuckType pt, int fwVers) throw(std::runtime_error);
	static int getPropertyIdNoThrow(enum Property prop, enum PuckType pt, int fwVers);

	static const int MIN_ID = 1;
	static const int MAX_ID = 31;
	static const int HOST_ID = 0;  // the Node ID of the control PC

	static int sendGetPropertyRequest(const CommunicationsBus& bus, int id, int propId);
	static int receiveGetPropertyReply(const CommunicationsBus& bus, int id, int propId,
			bool blocking, bool* successful);

protected:
	static int nodeId2BusId(int id) {
		return (id & TO_MASK) | (HOST_ID << NODE_ID_WIDTH);
	}
	static int busId2NodeId(int busId) {
		return (busId & FROM_MASK) >> NODE_ID_WIDTH;
	}
	static int encodeBusId(int fromId, int toId) {
		return (toId & TO_MASK) | ((fromId & NODE_ID_MASK) << NODE_ID_WIDTH);
	}
	static void decodeBusId(int busId, int* fromId, int* toId) {
		*fromId = (busId & FROM_MASK) >> NODE_ID_WIDTH;
		*toId = busId & TO_MASK;
	}

	static const int NODE_ID_WIDTH = 5;
	static const int NODE_ID_MASK = 0x1f;
	static const int GROUP_MASK = 0x400;
	static const int FROM_MASK = 0x3e0;
	static const int TO_MASK = 0x41f;

	static const int SET_MASK = 0x80;
	static const int PROPERTY_MASK = 0x7f;

	const CommunicationsBus& bus;
	int id;
	enum PuckType type, effectiveType;
	int vers;

private:
	friend class CANSocket;
};


inline int Puck::getPropertyId(enum Property prop, enum PuckType pt, int fwVers) throw(std::runtime_error) {
	int propId = getPropertyIdNoThrow(prop, pt, fwVers);
	if (propId == -1) {
		syslog(LOG_ERR, "Puck::getPropertyId(): Pucks with type %d and firmware version %d do not respond to property %d.", pt, fwVers, prop);
		throw std::runtime_error("Puck::getPropertyId(): Invalid property. Check /var/log/syslog for details.");
	}
	return propId;
}


}


#endif /* BARRETT_PUCK_H_ */