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

	char trajFile[] = "MM_10_DOF_joint_seq_modified_map.txt";

	char recTrajFile[] = "recorded_traj_XXXXXX";
	if (mkstemp(recTrajFile) == -1) {
		printf("ERROR: Couldn't create temporary file!\n");
		return 1;
	}

	// Create input-stream connection with a file
    std::ifstream myfile(trajFile);
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
	
	math::Spline<jp_type> spline(vec);
	
	printf("Press [Enter] to play back the recorded trajectory.\n");
	waitForEnter();

	// First, move to the starting position
	wam.moveTo(spline.eval(spline.initialS()));
	
	// Then, get ready to play back the recorded motion
	time.setOutput(spline.initialS());

	systems::Callback<double, jp_type> trajectory(boost::ref(spline));
	connect(time.output, trajectory.input); // Connect a time-source to the trajectory.
	// This time source will be used to dictate the current reference-position to be tracked.

	// And now, we connect the reference to be followed (output) to the reference tracker.
	wam.trackReferenceSignal(trajectory.output);

	// Log the running joint positions, with time
	// This is the same format of the custom tuple: jp_sample_type
	systems::TupleGrouper<double, jp_type> jpLogTg;
	// This will be used as a sort of collecting-agent, which will collect various 
	// kinds of information sources: such as joint position, velocity, EE pose, etc.

	// The sources will then be routed through this collection, and this collection will then be 
	// handed over to the logger, which will log packets made up of this tuple.
	connect(time.output, jpLogTg.template getInput<0>()); // Connecting the same time-source, that is being used for trajectory-following, to a particular index of the collection-source tuple
	connect(wam.jpOutput, jpLogTg.template getInput<1>()); // Connecting the joint-position-source to a particular index of the collection-source tuple

	// Get Time-period from execution manager
	const double T_s = pm.getExecutionManager()->getPeriod();

	// At what time-period multiple should the recording be done at
	const size_t PERIOD_MULTIPLIER = 10;
	// Record at a fraction of the loop-rate
	// The template `jp_sample_type' indicates what kinds of information, and in what sequence, will they be recorded in
	systems::PeriodicDataLogger<jp_sample_type> jpLogger(pm.getExecutionManager(),
			new barrett::log::RealTimeWriter<jp_sample_type>(recTrajFile, PERIOD_MULTIPLIER*T_s),
			PERIOD_MULTIPLIER);
	// This defines how the logger will be accepting information.
	// Notice how the packet definition is the same as the source-collection definition defined previously.
	// This will allow for a seamless connection between a collection-source and the logger.
	connect(jpLogTg.output, jpLogger.input); // Connecting the collection-source to the logger

	// Start ticking the clock
	time.start();

	while (trajectory.input.getValue() < spline.finalS()) {
		usleep(100000);
	}

	// Stop recording
	jpLogger.closeLog();
	disconnect(jpLogger.input);

	std::cout << "Finished following the user-provided trajectory, and stopped recording the actual joint positions." << std::endl;
	printf("Press [Enter] to go home.\n");
	waitForEnter();
	std::cout << "Going home now." << std::endl;
	wam.moveHome();
	printf("Gone home. Press [Enter] to idle the WAM.\n");
	waitForEnter();
	wam.idle();
	pm.getSafetyModule()->waitForMode(SafetyModule::IDLE);

	std::cout << "WAM in idle-state. Writing recorded trajectory to file ..." << std::endl;
	// Read the logger data into a vector
	log::Reader<jp_sample_type> lr(recTrajFile);
	std::vector<jp_sample_type> vec_rec_Traj_File;
	for (size_t i = 0; i < lr.numRecords(); ++i) {
		vec_rec_Traj_File.push_back(lr.getRecord());
	}
	// Output stuff to an external file
    std::ofstream outfile ("traj_file.txt");
    // Set precision: tells the maximum number of digits to use not the minimum; so no trailing zeros (https://stackoverflow.com/a/17342002/19163020)
    outfile << std::setprecision (std::numeric_limits<double>::digits10 + 1);
    // Output each element of the vector vec_rec_Traj_File at a time
    for (size_t row_index = 0; row_index<vec_rec_Traj_File.size(); ++row_index)
    {
        // Write the time of the recording
        outfile << boost::get<0>(vec_rec_Traj_File[row_index]) << ",";
		// Write the joint positions
        for (size_t col_index = 0; col_index<DOF; ++col_index)
        {
            outfile << "," << boost::get<1>(vec_rec_Traj_File[row_index])[col_index];
        }
        // Start on new line
        outfile << std::endl;
    }
    std::cout << "Done writing to file." << std::endl;

	//std::remove(tmpFile);
	
	return 0;
}
