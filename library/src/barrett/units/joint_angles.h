/*
 * joint_angles.h
 *
 *  Created on: Oct 20, 2009
 *      Author: dc
 */

#ifndef JOINT_ANGLES_H_
#define JOINT_ANGLES_H_


#include "./joint_torques.h"

namespace barrett {
namespace units {


// measured in radians
class JointAngles : public JointTorques {  // TODO(dc): needed a quick fix...
public:
	// associate with the joint space actuator type
	typedef JointTorques actuator_type;

	JointAngles() :
			JointTorques() {}
	JointAngles(double ja_array[]) :  //NOLINT: ctor deliberately not marked explicit
			JointTorques(ja_array) {}
};


// TODO(dc): this makes me cry a little on the inside
inline const JointAngles operator- (
		const JointAngles& jt1, const JointAngles& jt2)
throw(std::invalid_argument)
{
	JointAngles result;
	for (size_t i = 0; i < jt1.size(); ++i) {
		result[i] = jt1[i] - jt2[i];
	}

	return result;
}


}
}


#endif /* JOINT_ANGLES_H_ */