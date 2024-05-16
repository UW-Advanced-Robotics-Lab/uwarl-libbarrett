#include <iostream>
#include <string>

#include <boost/tuple/tuple.hpp>

#include <barrett/units.h>
#include <barrett/systems.h>
#include <barrett/log.h>
#include <barrett/products/product_manager.h>
#include <barrett/detail/stl_utils.h>

#include <barrett/standard_main_function.h>


using namespace barrett;
using detail::waitForEnter;


template<size_t DOF>
// FOr the single-output template, the input could be anything; it's just that there was a pre-existing example of
// having `JointPositions` already present.
class J_const_torque : public systems::SingleIO<typename units::JointPositions<DOF>::type, typename units::JointTorques<DOF>::type> {
	BARRETT_UNITS_TEMPLATE_TYPEDEFS(DOF);

public:
	explicit J_const_torque(double jt_const, const std::string& sysName = "J_const_torque") :
		systems::SingleIO<jp_type, jt_type>(sysName), max_torque(jt_const), jt(0.0) {}
	virtual ~J_const_torque() { this->mandatoryCleanUp(); }

protected:
	// Which joint do you want to exert constant torque on?
	static const size_t J_IDX_1 = 4; // Joint 5
	static const size_t J_IDX_2 = 5; // Joint 6
	// What is the max amount of torque do you want to exert?
	double max_torque;
	// At what rate do you want to increase torque amount by till max torque?
	static const double TORQUE_INC_RATE = 0.05/500;
	jt_type jt;

	virtual void operate() {
		if(jt[J_IDX_1]>-max_torque) {
			jt[J_IDX_1] = jt[J_IDX_1] - TORQUE_INC_RATE;
			jt[J_IDX_2] = jt[J_IDX_2] - TORQUE_INC_RATE;
		} else {
			jt[J_IDX_1] = -max_torque;
			jt[J_IDX_2] = -max_torque;
		}
		// What ever is in the `jt` variable, keep publishing it as an output
		this->outputValue->setData(&jt);
	}

private:
	DISALLOW_COPY_AND_ASSIGN(J_const_torque);
};


template<size_t DOF>
int wam_main(int argc, char** argv, ProductManager& pm, systems::Wam<DOF>& wam) {
	BARRETT_UNITS_TEMPLATE_TYPEDEFS(DOF);
	// To record a collection of joint-positions and commanded joint torques, make a tuple to record them in
	typedef boost::tuple<double, jp_type, jt_type> jp_jt_sample_type;
	// File name for recording joint position and torques
	char rec_traj_file[] = "Const_Motor_6_torque_2024_05_16_rec_traj_V2.txt";

	// A temporary file to log the relevant data
	char recTrajFile[] = "recorded_traj_XXXXXX";
	if (mkstemp(recTrajFile) == -1) {
		printf("ERROR: Couldn't create temporary file!\n");
		return 1;
	}

	//Don't need gravity compensation as that would lead to closed-loop control of joint torques.
	wam.gravityCompensate(false);

	// Start a time variable
	systems::Ramp time(pm.getExecutionManager());
	// Log the running joint positions and torque, with time
	// This is the same format of the custom tuple: jp_jt_sample_type
	systems::TupleGrouper<double, jp_type, jt_type> jp_jt_LogTg;
	// This will be used as a sort of collecting-agent, which will collect various 
	// kinds of information sources: such as joint position, velocity, EE pose, etc.

	// The sources will then be routed through this collection, and this collection will then be 
	// handed over to the logger, which will log packets made up of this tuple.
	connect(time.output, jp_jt_LogTg.template getInput<0>()); // Connecting the same time-source, that is being used for trajectory-following, to a particular index of the collection-source tuple
	connect(wam.jpOutput, jp_jt_LogTg.template getInput<1>()); // Connecting the joint-position-source to a particular index of the collection-source tuple
    // On page 30 of https://web.barrett.com/support/WAM_Documentation/WAM_Training_Documentation.pdf, the `jtSum` is used to output the commanded joint torques.  
    connect(wam.jtSum.output, jp_jt_LogTg.template getInput<2>()); // Connecting the joint-torque-source to a particular index of the collection-source tuple
	// Get Time-period from execution manager
	const double T_s = pm.getExecutionManager()->getPeriod();

	// At what time-period multiple should the recording be done at
	const size_t PERIOD_MULTIPLIER = 10;
	// Record at a fraction of the loop-rate
	// The template `jp_jt_sample_type' indicates what kinds of information, and in what sequence, will they be recorded in
	systems::PeriodicDataLogger<jp_jt_sample_type> jp_jt_Logger(pm.getExecutionManager(),
			new barrett::log::RealTimeWriter<jp_jt_sample_type>(recTrajFile, PERIOD_MULTIPLIER*T_s),
			PERIOD_MULTIPLIER);
	// This defines how the logger will be accepting information.
	// Notice how the packet definition is the same as the source-collection definition defined previously.
	// This will allow for a seamless connection between a collection-source and the logger.
	connect(jp_jt_LogTg.output, jp_jt_Logger.input); // Connecting the collection-source to the logger

	// Start ticking the clock
	time.start();

	printf("Press [Enter] to exert a constant torque.");
	waitForEnter();

	J_const_torque<DOF> j1s(2.0);
	systems::connect(wam.jpOutput, j1s.input);
	wam.trackReferenceSignal(j1s.output);

	// Wait for the user to press Shift-idle
	pm.getSafetyModule()->waitForMode(SafetyModule::IDLE);

	std::cout << "WAM in idle-state. Writing recorded trajectory to file ..." << std::endl;
	// Read the logger data into a vector
	log::Reader<jp_jt_sample_type> lr(recTrajFile);
	std::vector<jp_jt_sample_type> vec_rec_Traj_File;
	for (size_t i = 0; i < lr.numRecords(); ++i) {
		vec_rec_Traj_File.push_back(lr.getRecord());
	}
	// Output stuff to an external file
    std::ofstream outfile (rec_traj_file);
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
        // Write the joint torques
        for (size_t col_index = 0; col_index<DOF; ++col_index)
        {
            outfile << "," << boost::get<2>(vec_rec_Traj_File[row_index])[col_index];
        }
        // Start on new line
        outfile << std::endl;
    }
    std::cout << "Done writing to file." << std::endl;

	std::remove(recTrajFile);

	return 0;
}
