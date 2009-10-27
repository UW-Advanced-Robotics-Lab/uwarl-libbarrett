/*
 * controller_impl.h
 *
 *  Created on: Oct 20, 2009
 *      Author: dc
 */

#ifndef CONTROLLER_IMPL_H_
#define CONTROLLER_IMPL_H_


#include <barrett/systems/abstract/controller.h>


template<typename InputType, typename OutputType = InputType>
class ControllerImpl :
		public barrett::systems::Controller<InputType, OutputType> {
public:
	ControllerImpl() {}
	explicit ControllerImpl(const OutputType& initialOutputValue) :
		barrett::systems::Controller<InputType, OutputType>(initialOutputValue) {}

protected:
	virtual void operate() {}
};


#endif /* CONTROLLER_IMPL_H_ */