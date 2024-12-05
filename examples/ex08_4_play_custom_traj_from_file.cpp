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

	char tmpFile[] = "MM_10_DOF_joint_seq_modified_map.txt";

	// Create input-stream connection with a file
    std::ifstream myfile(tmpFile);
	// Check if the file can be read
	if(myfile.good()==false)
	{
		std::cout << "External file could not be found/opened."<< std::endl;
        return 1;
	}
    // Create a temp-string to hold the float-as-a-string joint-space way-point of all joints
    std::string tmp_str_wp;
    // Create a temp-string to hold the float-as-a-string joint-space way-point for each joint
    std::string tmp_str_jnt_wp;
    // Create a vector of jp_sample_type to store the trajectory sequence
    std::vector<jp_sample_type> vec;
	// Initialize an empty jp_sample_type object to store a trajectory information
	jp_sample_type tmp_wam_traj_wp;
	// Initialize an empty jp_type object to store a WAM joint way-point information
	jp_type tmp_wam_wp;
	// Number of DOF of Summit
	const size_t num_DOF_sum = 3;
	// Number of DOF of complete system
	const size_t num_DOF = num_DOF_sum + DOF;
	// Initialize an empty array object to store a Summit+WAM joint way-point information
	double tmp_wp[num_DOF];
	// Keep track of traj text-file column index
	size_t col_index;
	// Time-step (sec)
	const float time_step = 0.5;
	// Counter (to be incremented everytime a new line has been read from the file)
	unsigned long int counter = 0;
    // Read the file line-by line, and store it in a vector
    while (std::getline(myfile,tmp_str_wp))
    {
		// Enter the current time
		boost::get<0>(tmp_wam_traj_wp) = time_step*counter;
		// Initialize traj text-file column index
		col_index = 0;
        // Make a string-stream to read the individual comma-seperated segments in each line
        std::istringstream ss_wp(tmp_str_wp);
        while(std::getline(ss_wp,tmp_str_jnt_wp,',') && col_index<num_DOF)
        {
            // Convert the string into a long-double
			std::istringstream ss_val(tmp_str_jnt_wp);
			ss_val >> tmp_wp[col_index];
			++col_index;
        }
		// Re-initialize the current WAM joint way-point
		for(size_t index = 0; index<DOF; ++index)
		{
			tmp_wam_wp[index] = tmp_wp[index+num_DOF_sum];
		}
		// Set the 2nd element of the tuple to this tmp_wam_wp object
		boost::get<1>(tmp_wam_traj_wp) = tmp_wam_wp;
		// Push the tuple object into the vector of tuples to construct the trajectory
		vec.push_back(tmp_wam_traj_wp);
		// Increment the time counter
		++counter;
    }

	// Begin gravity compensation
	wam.gravityCompensate();

	systems::Ramp time(pm.getExecutionManager());

	// Build spline between recorded points
	/*
	// Read joint 1 of 1st reading
	jp_sample_type first_red = vec[0];
	// bo_tu_type example(3.14,3.15);
	// std::cout << "Current time: " << example.get<0>() << " sec." << std::endl;
	// Seperate out the tupple
	// Time
	double curr_t = boost::get<0>(first_red);
	std::cout << "Current time: " << curr_t << " sec." << std::endl;
	std::cout << "Next time: " << boost::get<0>(vec[1]) << " sec." << std::endl;
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
	// Number of elements in the vector vec
	std::cout << "Number of elements in the vector vec: " << vec.size() << " ." << std::endl;
	// Check the value of spline.finalS()
	*/
	
	math::Spline<jp_type> spline(vec);
	
	printf("Press [Enter] to play back the recorded trajectory.\n");
	waitForEnter();

	// First, move to the starting position
	wam.moveTo(spline.eval(spline.initialS()));
	
	// Then play back the recorded motion
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
	
	return 0;
}
