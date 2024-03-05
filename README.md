# Libbarrett 3.0.0
> Libbarrett is a real-time controls library written in C++ that runs Barrett
Technology products (WAM and BarrettHand)

` This version of Libbarrett works with a non-real-time kernel (a low-latency Ubuntu 20.04 kernel) and should only be used when a hard real-time guarantee is not critical for your application.`


### Download package
```
cd && git clone https://git.barrett.com/software/libbarrett
```

### Install dependencies
```
cd ~/libbarrett/scripts && ~/libbarrett/scripts/install_dependencies.sh
```

### Reboot into the new kernel (after reboot, "uname -r" should show "lowlatency"): 
```
sudo reboot
```

### Build and install the Peak pcan driver
We recommend the following CAN hardware:

 - PCAN-PCI
 - PCAN-PCIe
 - PCAN-USB
 - PCAN-ISA 

NOTE: These cards are supported by the SocketCAN driver built into Linux, however, we have found that the SocketCAN txbuffer can occasionally fail to send CAN frames without reporting an error, and this causes the WAM to E-Stop itself unexpectedly. This can happen a few seconds, a few minutes, or even several hours after starting a WAM application. Fortunately, Peak offers an alternative driver (pcan) which is still socket-based but avoids this SocketCAN bug.
```
sh ~/libbarrett/scripts/install_pcan.sh
```

### For PCAN-ISA only, manually configure the driver (not plug-and-play): 
```
sudo tee /etc/modprobe.d/pcan.conf <<EOF
options pcan type=isa,isa io=0x300,0x320 irq=7,5
install pcan modprobe --ignore-install pcan
EOF
echo 'pcan' |sudo tee -a /etc/modules-load.d/modules.conf
```

### Reboot to use the new CAN driver (after reboot, both "cat /proc/pcan" and "ifconfig" should list can0): 
```
sudo reboot
```

### Build libbarrett (using clang)
```
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cd ~/libbarrett && cmake .
make -j$(nproc)
```

### Install libbarrett
```
sudo make install
```

### Build the libbarrett example programs
```
cd ~/libbarrett/examples && cmake .
make -j$(nproc)
```

### Additional Makefile targets
Optional: Update or install configuration files only - not necessary if you have already done a full make (above)
```
make install_config
```

Optional: Package the library as a tar-ball (not common)
```
make package
```

### Git-controlling Configuration-folders
Due to the end-effector of the robot being swappable between the BHand and the stump, it is nescessary to be able to swap-out the inertia's of said component in the config-file. In order to prevent any un-due changes from occuring in the configuration-files (since it will directly impact how the robot behaves), we git-control the documnets in the configuration-folders.

Due to the Ubuntu-version (Groovy) being present in the WAM internal-PC being a legacy software, we avoid updating the packages, least something else breaks. Due to this, the ssh-protocol `ed25519`, which is what is being commonly used now for initializing a git-ssh connection (2024), is un-available. Support for `RSA` protocol has also been [dropped](https://github.blog/2021-09-01-improving-git-protocol-security-github/). Hence, the option we have choosen to follow is to use `ECDSA`, specifically the protocol `ecdsa-sha2-nistp256`, which is still supported by Git. This is a `256-bit` encryption, and the command to create a ssh-key using this protocol can be found [here](https://cloud.ibm.com/docs/hp-virtual-servers?topic=hp-virtual-servers-generate_ssh), which is:
```
ssh-keygen -t ecdsa -b 256 -C "axxxx@uwaterloo.ca"
```
DO NOT use the `sudo` privelage-elevation to execute the above line, as that will create ssh-keys for the root-user, not for the current robot-user, and we cannot access them subsequently.

DO NOT enter a name in the field:
```
Enter file in which to save the key (/home/robot/.ssh/id_ecdsa):
```
as the debug-command `ssh -vT git@github.com` is not able to locate them, so you will not be able to diagnose any problems that may arise. This will create the ssh-id called `id_ecdsa` (and its corresponding public-key `id_ecdsa.pub`).

We then add the created ssh-key:
```
ssh-add ~/.ssh/id_ecdsa
```
so that the ssh-client is able to use this key to connect with Git.

Before connecting with git and pushing, we add the public-key onto the user's SSH key-list on Github (please follow the rest of Jack's procedure, starting step 2, as outlined [here](https://github.com/UW-Advanced-Robotics-Lab/lab-wiki/wiki/Waterloo-Steel%3APlatform-Setup-Workstation#133-ssh-keys--github)).

Once this is done, we can verify that this PC can indeed connect with Git: `ssh -vT git@github.com`. It should terminate with an `Exit status 1` debug-message. In-case of failure, it will end with `Permission denied (publickey).`.

### Configuration Files for the WAM
Upon installation of libbarrett, the configuration files of the robot are installed to the `/etc/barrett` directory. However, to give an additional flexibility of each user maintaining their own configurations for the same robot, by default, the configuration files are read from `~/.barrett` directory if it exists. If not, then libbarrett reads the necessary configuration files from `/etc/barrett/` directory. It is up to the user to maintain and populate the `~/.barrett` directory.

### Eclipse IDE setup
It is possible to use CMake to generate several output formats, including Eclipse CDT4 project files. For details, see:
    http://www.paraview.org/Wiki/Eclipse_CDT4_Generator
To generate Eclipse project files, run:
```
cmake . -G"Eclipse CDT4 - Unix Makefiles"
```
Then import the generated project into your Eclipse workspace using:
File -> Import -> General -> Existing Projects into Workspace

### Gravity Calibration

For gravity calibration, follow the instructions on the [Quick Start Guide](https://web.barrett.com/support/WAM_Documentation/WAM_QuickStartGuide.pdf). Do not follow process defined in [Barrett Wiki](https://support.barrett.com/wiki/WAM/Calibration), as the function `./calibrate`->`Calibrate Gravity Compensation` fails to load.

The default `calibration.conf`, in `~/.barrett/`, has the following look:

```
    wam7w:
	(
		(     0.0, -1.5708,     0.0,  1.5708,     0.0,     0.0,     0.0 ),
		(  0.7854, -1.5708,  1.5708,  1.5708, -1.5708,     0.0,     0.0 ),
		( -0.7854, -1.5708, -1.5708,  1.5708, -1.5708,     0.0,     0.0 ),
		( -0.7854,  1.5708,  1.5708,  1.5708,     0.0,     1.5,     0.0 ),
		(     0.0,  0.7854,     0.0,  2.3562,     0.0, -0.7854,     0.0 ),
		(  0.7854,  1.5708, -1.5708,  1.5708,     0.0,     1.5,     0.0 ),
		(     0.0,     0.0,  0.7854,  1.5708,  0.7854,  0.7854,  0.7854 ),
		(     0.0,  1.5708, -1.5708, -0.3927,  0.7854, -1.0781, -0.7854 ),
		( -0.7854,  0.7854, -0.7854, -0.7854,  0.7854, -0.7854,  0.7854 )
	);
```

According to the [Barrett Wiki](https://support.barrett.com/wiki/WAM/Calibration), gravity calibration process approaches these poses twice: once from larger joint angle values, and once from lower joint angle values. This is because the joint torques expereinced during bothe these approaches are different, and so they take their average and record it.

Because of this, we cannot have joint angles of these sample poses too close to the physical joint-angle limits of each joint.

You can see that, in poses 4 and 6, joint 6 comes quite close to its joint limits. Since this poses were included as default-ones in the installation package, we can assume that this level of margin from their respective joint-limits is tolerable for this procedure. 

The gravity calibration process gives us mass values of the involved links, to be used when trying to counter-act gravity. These calibration values are to be introduced into the WAM configuration file, above the `safety{}` section [Barrett Wiki](https://support.barrett.com/wiki/WAM/Calibration).

### Configuration File

The configuration file for the WAM is located on the WAM PC at `~/btclient/config/`, and, for our 7 DOF WAM, it is called the `WAM7.conf`. An introductory video to walk through various aspects of the configuration file can be found [here](https://web.barrett.com/support/WAM_VideoSupport/Getting_Started/04_WAMConfigFile.mov).

By comparing the link inertia values stored in the config-file with that in the [Barrett Wiki](https://web.barrett.com/support/WAM_Documentation/WAM_InertialSpecifications_AC-02.pdf), it was determined that the inertias being used correspond to the intertia being defined at the center-of-mass and aligned w.r.t the output coordinate system. We were able to determine this by comparing the values, and found that the inertia-matrix value for this definition most closely matches with the one found in the config-file. This information is nescessary as there are 3 descriptions of the inertia-matrix in the documentation, and the same type is being used to describe the inertia property of all links, including the end-effector.

The inertia, and COM location of the end-effector were not matching/present in the original config-file. Since our WAM will be using the Barrett-Hand as an end-effector, it is nescessary to provide the correct inertia values for more accurate planned-trajectory-adherence. The following additions were made:

#### COM Location
```
com = <0.006, 0.0, 0.057> # BarrettHand BH8-280 (https://support.barrett.com/wiki/Hand/280)
```

#### Inertia values
```
# Inertial matrix, chosen at this link's center of mass and aligned with the output coordinate (page 3 of https://web.barrett.com/support/BarrettHand_Documentation/BarrettHand280MassProp-2010Dec10.pdf)
        #I = <<0.0, 0.0, 0.0>,<0.0, 0.0, 0.0>,<0.0, 0.0, 0.0>>
        I = << 0.00152162,-0.00000366, 0.00001980 >, 
             <-0.00000366, 0.00207291,-0.00000102 >,
             < 0.00001980,-0.00000102, 0.00161521 >> # BarrettHand
```
, and the D-H parameters, corresponding to the Barrett-Hand, that were already present in the config-file, were un-commented.

Note: The motors being referred to in this config are the motors driving the cables, not any joint motors (that one may be defining in ROS).

### Gravity Compensation calibration: Errors

Here, we examine the causes of periodic/persistant causes of Gravity compensation calibration failure, particularly when executing pose 8 mentioned in the above documentation of the poses being adhered to during calibration.

Upon examining the Error log-files `syslog`, located in `/var/log/', it is still unclear what is the cause of the failure:

```
Mar  1 09:03:45 localhost bt-wam-gravitycal: ProductManager::ProductManager()
Mar  1 09:03:45 localhost bt-wam-gravitycal:   Config file (from default): /home/robot/.barrett/default.conf
Mar  1 09:03:45 localhost bt-wam-gravitycal: CANSocket::open(0) using RTCAN driver
Mar  1 09:03:45 localhost bt-wam-gravitycal: ProductManager::enumerate()
Mar  1 09:03:45 localhost bt-wam-gravitycal:   Pucks:
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 1 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 2 VERS=200 ROLE=0x0100 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 3 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 4 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 5 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 6 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 7 VERS=200 ROLE=0x01c0 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID= 8 VERS= 25 ROLE=0x0106 TYPE=ForceTorque
Mar  1 09:03:45 localhost bt-wam-gravitycal:     --
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID=10 VERS=197 ROLE=0x0082 TYPE=Safety
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID=11 VERS=200 ROLE=0x1e85 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID=12 VERS=200 ROLE=0x1e85 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID=13 VERS=200 ROLE=0x1e85 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     ID=14 VERS=200 ROLE=0x1285 TYPE=Motor
Mar  1 09:03:45 localhost bt-wam-gravitycal:   Products:
Mar  1 09:03:45 localhost bt-wam-gravitycal:     7-DOF WAM (Wrist)
Mar  1 09:03:45 localhost bt-wam-gravitycal:     Safety Module
Mar  1 09:03:45 localhost bt-wam-gravitycal:     Force-Torque Sensor
Mar  1 09:03:45 localhost bt-wam-gravitycal:     BarrettHand
Mar  1 09:03:45 localhost bt-wam-gravitycal: LowLevelWam::LowLevelWam()
Mar  1 09:03:45 localhost bt-wam-gravitycal:   Config setting: wam7w.conf => "wam7w.low_level"
Mar  1 09:03:45 localhost bt-wam-gravitycal:   WAM was already zeroed
Mar  1 09:03:53 localhost bt-wam-gravitycal: Hand::Hand()
Mar  1 09:03:53 localhost bt-wam-gravitycal:   Found 3 Fingertip torque sensors
Mar  1 09:03:53 localhost bt-wam-gravitycal:   Found 4 Tactile arrays
Mar  1 09:03:54 localhost bt-wam-gravitycal: Pose is: <  0.0000, -1.5708,  0.0000,  1.5708,  0.0000,  0.0000,  0.0000>
Mar  1 09:03:54 localhost bt-wam-gravitycal: Moving to: <  0.0300, -1.5408,  0.0300,  1.6008,  0.0300,  0.0300,  0.0300>
Mar  1 09:04:19 localhost bt-wam-gravitycal: Pose is: <  0.7854, -1.5708,  1.5708,  1.5708, -1.5708,  0.0000,  0.0000>
Mar  1 09:04:19 localhost bt-wam-gravitycal: Moving to: <  0.8154, -1.5408,  1.6008,  1.6008, -1.5408,  0.0300,  0.0300>
Mar  1 09:04:39 localhost bt-wam-gravitycal: Pose is: < -0.7854, -1.5708, -1.5708,  1.5708, -1.5708,  0.0000,  0.0000>
Mar  1 09:04:39 localhost bt-wam-gravitycal: Moving to: < -0.7554, -1.5408, -1.5408,  1.6008, -1.5408,  0.0300,  0.0300>
Mar  1 09:05:02 localhost bt-wam-gravitycal: Pose is: < -0.7854,  1.5708,  1.5708,  1.5708,  0.0000,  1.5000,  0.0000>
Mar  1 09:05:02 localhost bt-wam-gravitycal: Moving to: < -0.7554,  1.6008,  1.6008,  1.6008,  0.0300,  1.5300,  0.0300>
Mar  1 09:05:27 localhost bt-wam-gravitycal: Pose is: <  0.0000,  0.7854,  0.0000,  2.3562,  0.0000, -0.7854,  0.0000>
Mar  1 09:05:27 localhost bt-wam-gravitycal: Moving to: <  0.0300,  0.8154,  0.0300,  2.3862,  0.0300, -0.7554,  0.0300>
Mar  1 09:05:49 localhost bt-wam-gravitycal: Pose is: <  0.7854,  1.5708, -1.5708,  1.5708,  0.0000,  1.5000,  0.0000>
Mar  1 09:05:49 localhost bt-wam-gravitycal: Moving to: <  0.8154,  1.6008, -1.5408,  1.6008,  0.0300,  1.5300,  0.0300>
Mar  1 09:06:10 localhost bt-wam-gravitycal: Pose is: <  0.0000,  0.0000,  0.7854,  1.5708,  0.7854,  0.7854,  0.7854>
Mar  1 09:06:10 localhost bt-wam-gravitycal: Moving to: <  0.0300,  0.0300,  0.8154,  1.6008,  0.8154,  0.8154,  0.8154>
Mar  1 09:06:32 localhost bt-wam-gravitycal: Pose is: <  0.0000,  1.5708, -1.5708, -0.3927,  0.7854, -1.0781, -0.7854>
Mar  1 09:06:32 localhost bt-wam-gravitycal: Moving to: <  0.0300,  1.6008, -1.5408, -0.3627,  0.8154, -1.0481, -0.7554>
Mar  1 09:06:56 localhost bt-wam-gravitycal: Pose is: < -0.7854,  0.7854, -0.7854, -0.7854,  0.7854, -0.7854,  0.7854>
Mar  1 09:06:56 localhost bt-wam-gravitycal: Moving to: < -0.7554,  0.8154, -0.7554, -0.7554,  0.8154, -0.7554,  0.8154>
Mar  1 09:07:27 localhost bt-wam-gravitycal: RealTimeExecutionManager control-loop stats (microseconds):
Mar  1 09:07:27 localhost bt-wam-gravitycal:   target period = 2000
Mar  1 09:07:27 localhost bt-wam-gravitycal:   min = 841
Mar  1 09:07:27 localhost bt-wam-gravitycal:   ave = 862.618
Mar  1 09:07:27 localhost bt-wam-gravitycal:   max = 4363
Mar  1 09:07:27 localhost bt-wam-gravitycal:   stdev = 26.875
Mar  1 09:07:27 localhost bt-wam-gravitycal:   num total cycles = 110935
Mar  1 09:07:27 localhost bt-wam-gravitycal:   num missed release points = 1
Mar  1 09:07:27 localhost bt-wam-gravitycal:   num overruns = 2
```

Since the pendant was showing Torque-Fault, we assume that the cause was of not modelling the inertias of the links, especially the BHand, as we had previously added its inertias to the `~/btclient/config/WAM7.conf` file.

From the above log, we see that it first accesses the default configuration file, located at `/home/robot/.barrett/default.conf`, then, depending upon the current system configuration (what end-effector has been mounted), the system decides on using a wam configuration-file, namely `wam7w.conf`, located in `/home/robot/.barrett/`. Upon opening it, we find no tool description, inspite us adding them to the `~/btclient/config/WAM7.conf`. So, it remains unclear what purpose does the file at `~/btclient/config/WAM7.conf` serve.

#### Incorrect End-Effector frame location

This was the DH-parameters for specifying the end-effector frame location:

```
toolplate = { alpha_pi = 0; theta_pi = 0; a = 0; d = 0; };
```

Using the DH parameters already available for the BHand in `~/btclient/config/WAM7.conf`, we modified the same to reflect:
```
        # BarrettHand: BH8-280 (https://support.barrett.com/wiki/Hand/280)
        toolplate = { alpha_pi = 0; theta_pi = 0; a = 0; d = 0.1; }; # From WAM7.conf in ~/btclient/conf/
```
#### No End-Effector inertia information

This was the state of the Tool inertia info in the original config file:

```
    dynamics:
	{
		# From inertial specifications Sept 2008
		moving:
		(   
            {...
            },
			{
				# Link 7
				mass = 0.07548270;
				com = ( 0.00014836,0.00007252,-0.00335185 );
				I = (( 0.00003911, 0.00000019, 0.00000000 ),
					 ( 0.00000019, 0.00003877, 0.00000000 ),
					 ( 0.00000000, 0.00000000, 0.00007614 ));
			}
            # Tool ?
        );
```
We then copied over the BHand-inertias that we had entered in `~/btclient/config/WAM7.conf` file to get:
```
    dynamics:
	{
		# From inertial specifications Sept 2008
		moving:
		(   
            {...
            },
			{
				# Link 7
				mass = 0.07548270;
				com = ( 0.00014836,0.00007252,-0.00335185 );
				I = (( 0.00003911, 0.00000019, 0.00000000 ),
					 ( 0.00000019, 0.00003877, 0.00000000 ),
					 ( 0.00000000, 0.00000000, 0.00007614 ));
			}
        );
        toolplate =
		    {
		    # Tool: BarrettHand: BH8-280 (https://support.barrett.com/wiki/Hand/280)
		    mass = 1.2;
		    com = (0.006, 0.0, 0.057 ); # BarrettHand (page 3 of https://web.barrett.com/support/BarrettHand_Documentation/BarrettHand280MassProp-2010Dec10.pdf)
		    # Inertial matrix, chosen at this link's center of mass and aligned with the output coordinate (page 3 of https://web.barrett.com/support/BarrettHand_Documentation/BarrettHand280MassProp-2010Dec10.pdf)
		    I = (( 0.00152162,-0.00000366, 0.00001980 ), 
		         (-0.00000366, 0.00207291,-0.00000102 ),
		         ( 0.00001980,-0.00000102, 0.00161521 )); # BarrettHand
		    };
    }
```
This should aid the torque-computation under dynamics.