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

For gravity calibration, follow the instructions on [Barrett Wiki](https://support.barrett.com/wiki/WAM/Calibration).

The default `cal.conf` has the following look:

```
# WAM Gravity Calibration Pose Configuration

calibration-poses-wam4{
    poseCount = 5
    pose[] = < 0.0, -1.5708,     0.0,    0.0 >
    pose[] = < 0.0, -1.5708, -1.5708, 1.5708 >
    pose[] = < 0.0,     0.0, -1.5708, 1.5708 >
    pose[] = < 0.0,     0.0,     0.0,    0.0 >
    pose[] = < 0.0,  1.5708,     0.0,    0.0 >
}

# Note - J5 joint stops are at +1.24 and -4.76
#        J6 joint stops are at +1.57 and -1.57
calibration-poses-wam7{
   poseCount = 9
   pose[] = <     0.0, -1.5708,     0.0,  1.5708,     0.0,     0.0,     0.0 >
   pose[] = <  0.7854, -1.5708,  1.5708,  1.5708, -1.5708,     0.0,     0.0 >
   pose[] = < -0.7854, -1.5708, -1.5708,  1.5708, -1.5708,     0.0,     0.0 >
   pose[] = < -0.7854,  1.5708,  1.5708,  1.5708,     0.0,     1.5,     0.0 >
   pose[] = <     0.0,  0.7854,     0.0,  2.3562,     0.0, -0.7854,     0.0 > 
   pose[] = <  0.7854,  1.5708, -1.5708,  1.5708,     0.0,     1.5,     0.0 >
   pose[] = <     0.0,     0.0,  0.7854,  1.5708,  0.7854,  0.7854,  0.7854 >
   pose[] = <     0.0,  1.5708, -1.5708, -0.3927,  0.7854, -1.0781, -0.7854 >
   pose[] = < -0.7854,  0.7854, -0.7854, -0.7854,  0.7854, -0.7854,  0.7854 >
}
```

According to the [Barrett Wiki](https://support.barrett.com/wiki/WAM/Calibration), gravity calibration process approaches these poses twice: once from larger joint angle values, and once from lower joint angle values. This is because the joint torques expereinced during bothe these approaches are different, and so they take their average and record it.

Because of this, we cannot have joint angles of these sample poses too close to the physical joint-angle limits of each joint. So, we extend the information for joint angle limits to other joints and replace it with the following copy:

```
# WAM Gravity Calibration Pose Configuration

calibration-poses-wam4{
    poseCount = 5
    pose[] = < 0.0, -1.5708,     0.0,    0.0 >
    pose[] = < 0.0, -1.5708, -1.5708, 1.5708 >
    pose[] = < 0.0,     0.0, -1.5708, 1.5708 >
    pose[] = < 0.0,     0.0,     0.0,    0.0 >
    pose[] = < 0.0,  1.5708,     0.0,    0.0 >
}

# Note - J1 joint stops are at +2.6  and -2.6  rad
#        J2 joint stops are at +2.0  and -2.0  rad
#        J3 joint stops are at +2.8  and -2.8  rad
#        J4 joint stops are at +3.1  and -0.9  rad
#        J5 joint stops are at +1.24 and -4.76 rad
#        J6 joint stops are at +1.57 and -1.57 rad | They are slightly different on the official wiki (https://support.barrett.com/wiki/WAM/KinematicsJointRangesConversionFactors)
#        J7 joint stops are at +3.0  and -3.0  rad
calibration-poses-wam7{
   poseCount = 9
   pose[] = <     0.0, -1.5708,     0.0,  1.5708,     0.0,     0.0,     0.0 >
   pose[] = <  0.7854, -1.5708,  1.5708,  1.5708, -1.5708,     0.0,     0.0 >
   pose[] = < -0.7854, -1.5708, -1.5708,  1.5708, -1.5708,     0.0,     0.0 >
   pose[] = < -0.7854,  1.5708,  1.5708,  1.5708,     0.0,     1.5,     0.0 >
   pose[] = <     0.0,  0.7854,     0.0,  2.3562,     0.0, -0.7854,     0.0 > 
   pose[] = <  0.7854,  1.5708, -1.5708,  1.5708,     0.0,     1.5,     0.0 >
   pose[] = <     0.0,     0.0,  0.7854,  1.5708,  0.7854,  0.7854,  0.7854 >
   pose[] = <     0.0,  1.5708, -1.5708, -0.3927,  0.7854, -1.0781, -0.7854 >
   pose[] = < -0.7854,  0.7854, -0.7854, -0.7854,  0.7854, -0.7854,  0.7854 >
}
```

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