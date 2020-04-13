## AV Lane Following Simulation
Author: Corwin Stout - corwinjs@umich.edu  
Date: April 7, 2020

![VehicleVisionDemoTrimmed](https://user-images.githubusercontent.com/37941576/79151577-87bf4800-7d7f-11ea-9401-d91e8a487f54.gif)

### Prerequisites:
  - Python 3 installed with the following libraries:
    - Pillow
    - Numpy
    - Matplotlib
    - OpenCV (cv2)
  - If working with the Unreal source code: Unreal Engine version 4.22 or newer (4.22 and 4.24 tested). This is not necessary if you are working with a packaged version of the project from the releases section.

### Running The Project In Unreal Editor:
  1. Review and make any desired changes to VehicleVision/config.txt 
  2. If you want to run the project in Autonomous Mode, then in a Command Prompt or your terminal of choice, run LaneTracker.py. If it is running correctly, you should see the following text:
  ```
       Socket successfully created
       socket binded to 27015
       socket is listening
  ```
  3. Windows/MacOS: In a File Explorer/Finder window, navigate to the VehicleVision folder and double-click on VehicleVision.uproject to start the simulator  
     Linux: In a terminal, navigate to the VehicleVision directory (where VehicleVision.uproject is located) and run `$Absolute/Path/To/Editor $Absolute/Path/To/VehicleVision.uproject`. On my computer that looks like:
     ```
     $ cd AVLaneFollowingSimulation/VehicleVision/
     $ ~/UnrealEngine4.22/Engine/Binaries/Linux/UE4Editor ~/Desktop/AVLaneFollowingSimulation/VehicleVision/VehicleVision.uproject 
     
### Running The Packaged Project:
  1. Review and make any desired changes to VehicleVision/config.txt 
  2. If you want to run the project in Autonomous Mode, then in a Command Prompt or your terminal of choice, run LaneTracker.py. If it is running correctly, you should see the following text:
  ```
       Socket successfully created
       socket binded to 27015
       socket is listening
  ```
  3. Windows: In a File Explorer window, navigate to the VehicleVision folder and double-click on VehicleVision.exe to start the simulator  
     Linux: Run VehicleVision/VehicleVision.sh  
     MacOS:  
        1. `$ chmod +x VehicleVision.app/Contents/MacOS/VehicleVision`
        2. Double-click on VehicleVision.app. You will likely see a pop-up window that says something along the lines of the identity of the developer cannot be confirmed and the app cannot be run.
        3. Open System Preferences and navigate to Security & Privacy. In the General tab you should see an option to allow VehicleVision to be opened. Check the box to allow this.
        4. Run VehicleVision/VehicleVision.sh

### Simulator Controls:
  - [spacebar]: Pause the simulator (the LaneTracker application will wait)
  - [esc]: End the simulation
  - [up],[down],[left],[right]: In manual driving mode, use arrow keys to
    drive the vehicle
  - Linux/Windows:
    - [ctrl]+[alt]+[enter]: Toggle the simulator between full screen mode and
    windowed mode
    - [ctrl]+[alt]+[esc]: Regain control of the mouse pointer
  - MacOS:
    - [option]+[return]: Toggle the simulator between full screen mode and windowed mode
  
### Source Files:
Located at https://github.com/corwinjs/AVLaneFollowingSimulation/

### Packaged Project:
Located in the releases at https://github.com/corwinjs/AVLaneFollowingSimulation/releases
