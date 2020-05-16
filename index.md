Corwin Stout (corwinjs@umich.edu) - May 9, 2020

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/79151577-87bf4800-7d7f-11ea-9401-d91e8a487f54.gif" alt="VehicleVisionDemoTrimmed"/>
</p>

Autonomous driving is making its way from science fiction to reality and is the focus of a large amount of research both in the automotive industry and in academia. One important area of this research is computer vision, which put simply is computers analyzing camera images to extract useful information. In the context autonomous driving, a typical self-driving vehicle is instrumented with several cameras so that, using computer vision algorithms, their images can be analyzed by onboard computers to tell the vehicle about its environment. Computer vision is itself a very broad topic of research. An autonomous vehicle needs to know where pedestrians are, where vehicles are and how fast they are moving, how far away the next stop sign is, and so many other things. The more information that needs to be extracted from camera images, the more complicated the computer vision algorithms become.

The first piece of visual information someone might think is important for an autonomous vehicle is where the lanes are. After all, it needs to know how to stay on the road somehow.  In the interest of learning more about computer vision, I decided to start with lane line detection. Thankfully, because of the rising interest in autonomous vehicles, there are many papers and examples online for approaches to this problem. Perfect! However, validation of computer vision programs tends to come in the form of testing it with a collection of images taken from one database or another. I thought it would be more fun to try a different approach.

The Unreal Engine is a video game engine with excellent graphics rendering and built-in physics. Implemented in C++, it is also very flexible to work with. Also, it's free and comes with a game template for vehicles. Nice. I decided it would be fun to use Unreal to build a simulator to work with my lane detection program. Within Unreal, I set up a camera on a vehicle and had it drive around while sending the camera images to the lane detector in real time. Then, based on what the lane detection program interpreted from those camera images and sends back to the vehicle, the vehicle knows how far to turn in which direction in order to stay in its lane. If the vehicle always stays on track, then that is a pretty thorough validation of the computer vision program.

In this article I will first explain the design of the simulation setup and how the two aspects of the system, computer vision and the simulator itself, communicate. I will then walk through how I implemented the lane detection component and the simulator component. At the end I wrap up with the results of my work, links to my software both as source code and in precompiled form, and some instructions for how to use it yourself.

- [System Design](#system-design)
- [Computer Vision - Lane Detection](#computer-vision---lane-detection)
- [Simulation](#simulation)
- [Results](#results)
- [Run It Yourself](#run-it-yourself)

## System Design

While computer vision techniques can be applied using any computer language, Python and Matlab have become the languages of choice in both academia and the industry. This was largely due to the toolboxes available in each that made applying computer vision algorithms quicker and easier, and because they are popular the latest tools are being developed for them which makes them that much more popular and so on. I chose to implement my computer vision application using Python in order to have access to toolboxes such as Numpy (for linear algebra), Matplotlib (for visualizations), and OpenCV (for image handling and manipulation).  

I wrote my simulator application using the Unreal Engine so that I could make use of its built-in physics libraries, in particular those for wheeled vehicles, and for its graphics for visualization purposes. The Unreal Engine comes with a game template for vehicles which includes a simple vehicle with a camera that is controlled by arrow keys, and I used this template as a starting point for my work. The language used for work with Unreal is C++, which is of course very fast, powerful, and popular. However, while computer vision toolboxes exist for C++, it is not as popular a choice for that type of work, so I chose not to implement my computer vision algorithms in the Unreal application directly.  

In order to write my computer vision application using Python and my simulator application using Unreal/C++ and have them work together in real time, I needed a way to have them communicate with each other quickly and consistently. I did this using TCP-based socket libraries. During each tick/frame/step, of the simulation, the physics are applied and the location of the vehicle is updated. The current view of the camera is turned into a flat array of uint8 values and send as a TCP message to the computer vision application. The image is then reconstructed and computer vision techniques are applied to isolate, detect, and locate the two lane lines in the image. Once those lines are found, the midpoint between the two is found at a specified lookahead distance. That center point is then sent as a TCP message back to the simulator. Within the simulator, the vehicle’s controller uses this center point to determine the steering wheel angle for the next time step, and the whole process repeats for every step.

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/81484177-848d6f80-91f8-11ea-82a1-c8385bb18064.png" alt="project-diagram-updated"/>
</p>

By using this configuration, the computer vision application and the simulator application are kept mostly independent. The only requirement for them to be used together is that both applications need to know what size image is being sent over TCP. This level of independence is very useful for two main reasons. The first, which is discussed earlier as well, is that Unreal applications are developed using C++ whereas computer vision applications are typically developed using Python or Matlab. By keeping the applications independent they each benefit from being written in languages that best suit them. The second reason is that now that the simulator has been developed, others can write their own computer vision applications that can still be compatible as long as they send and receive TCP messages in the same way. Unreal applications can be packaged into executables that can be run on other computers without needing an installation of Unreal Engine or any additional libraries. Someone interested in only working on computer vision can simply download the packaged simulator application and use it to test and run their software without needing to know how to use Unreal Engine at all.

## Computer Vision - Lane Detection

Image processing applications typically consist of a pipeline of algorithms or functions that each take in image data, work on it in some way, and output modified image data or other meaningful information about the image. My application is no exception. The pipeline I created begins with image data received from the simulator and in the end sends a string containing lane center coordinates back to the simulator. Here is a diagram showing the pipeline and its subsystems.

![vision-pipeline](https://user-images.githubusercontent.com/37941576/81484196-9a9b3000-91f8-11ea-8e39-0de2261b28c3.png)

#### Image Read and Reconstruction

The lane detection program takes in image data in the form of an array of pixel values. When running alongside the simulator, the first action taken at each time step is to receive the pending TCP message that contains the data for the current image seen by the simulated camera. Here is an example of an image that the simulated camera will see.

![original](https://user-images.githubusercontent.com/37941576/81483406-b996c380-91f2-11ea-8fa5-76b32cc13901.png)

The simulated camera captures images of size 1024 by 512 pixels, which when flattened fits into a one-dimensional array with a length of 524,288 pixels. It then converts the image to grayscale, with pixel values ranging from 0-255 which are stored as unsigned 8-bit integers (uint8). The simulator then sends the image data array, which is received by the lane detector program. The data is then resized to its original dimensions to reconstruct the image. For the purpose of demonstrating each step of the computer vision pipeline, we'll follow one static image (the one above) as it goes through each step. Here is what that image looks like after it has been reconstructed by the lane detection program.

![gray_image](https://user-images.githubusercontent.com/37941576/81483398-ac79d480-91f2-11ea-974b-81514d36664a.png)

#### Gaussian Blur

Image data is typically subject to many different sources of noise. Examples include shadows and reflections, lens flare, surface roughness, and birds flying by, and the list goes on and on. One way of reducing some of that noise is by applying a Gaussian Blur to the image, which entails convolving the image with a convolutional mask generated using a two-dimensional gaussian function. The resulting image appears slightly blurred, though not enough to lose the context of the image. Notice that while features of the image below have been subtly softened, the lane lines are still prominent.

![gauss_img](https://user-images.githubusercontent.com/37941576/81483394-a84db700-91f2-11ea-9ce1-0a5c3dbf9915.png)

#### Canny Edge Detection

Edges in an image are defined by changes in color values between pixels. The Canny algorithm is a popular method of finding edges. It works by finding the color gradients throughout the image and removing those above and below specified threshold values. Removing gradients above the high threshold is done to remove edges caused by noise and removing gradients below the low threshold removes less important or interesting edges. The algorithm also removes any isolated gradient points because if they are isolated, they are not likely to be a part of a significant edge. Here is what the original image (without the Gaussian Blur applied) looks like after undergoing Canny Edge Detection.

![cannyed_image_raw](https://user-images.githubusercontent.com/37941576/81483379-9d932200-91f2-11ea-8b76-f4168feae400.png)

It is clear that this process was largely successful in isolating important lines in the image. However, notice that there are still lines that are less important, such as in the clouds and due to the roughness of the lane lines themselves. These are a couple of reasons among many that we apply the Gaussian Blur. By applying that blurring and then the Canny Edge Detection, this is the new result.

![cannyed_image](https://user-images.githubusercontent.com/37941576/81483384-a126a900-91f2-11ea-91e6-1b24a1586935.png)

Notice that some of the remaining less important lines have been removed, and the outlines of the lane lines themselves feature all the more prominently.

#### Crop to Region of Interest

Now that the important edges in the image have been extracted, we see that there are still other edges in the image besides the lane lines. This will always be the case, and even more so with images of the real world compared to these images from this simulated world with very little in it. If the features you are trying to extract are usually in the same area of an image, isolating those features from the rest can be as simple as defining the region of interest and cropping out the rest. In the case with these lane lines, they will always reach near the bottom of the image and then get closer together until they are too far away to detect reliably. The trapezoidal shape that they make, with some buffer to account for how the lanes will look during a turn, serves as the region of interest and the features outside of that, such as the walls and clouds, are effectively cropped out. The red area of the following image represents the region of interest used.

![blendimg](https://user-images.githubusercontent.com/37941576/81483371-9409ba00-91f2-11ea-93fb-6db9a3ed1203.png)

After cropping out everything not within that region of interest, the image now shows only the most important lines for this process: the outlines of the lane lines themselves.

![cropped_image](https://user-images.githubusercontent.com/37941576/81483389-a4219980-91f2-11ea-86a6-40dbbdfc7b49.png)

#### Hough Line Detection

Even though the lane lines have now been effectively isolated, they are still embedded in the image and are not in a form that is useful for mathematical operation. One way to extract the lines from an image is to apply the Hough Line Transform. This transform works by expressing each point of an image in polar coordinates which makes it easier to determine how many points are intersected by candidate lines. Lines are extracted and parametrized if they intersect a minimum threshold of points in the image. While we might expect (or hope) there are only four lines returned by the Hough Transform, one for each side of a lane line, the lines in the image are not perfect and as a result more lines that make up segments of the lane lines are returned instead. In the image we have been working on, eleven separate lines were detected by the Hough Transform. In the image below, each of those lines have been assigned a different color and overlaid on the image.

![raw_lines_img](https://user-images.githubusercontent.com/37941576/81483410-c1eefe80-91f2-11ea-8b3e-414c5896f5fe.png)

#### Sort and Consolidate Lines

With the lines that make up the lanes in a more mathematical form, we can now determine their slope which should indicate which lane line they belong to. Lines with a negative slope belong to the left lane line, and those with a positive slope belong to the right lane line (this is the
opposite of what one might expect, due to the image coordinate convention of having X = 0, Y = 0 be at the top left of the image). The image below shows the lines that were determined to be a part of the left lane line.

![left_lines_img](https://user-images.githubusercontent.com/37941576/81483401-b00d5b80-91f2-11ea-87c6-aab36ae5107f.png)

The remaining lines of course were determined to be a part of the right lane line.

![right_lines_img](https://user-images.githubusercontent.com/37941576/81483413-c74c4900-91f2-11ea-9e52-9ed0b51f294f.png)

Now that they are sorted, each group of lines can be consolidated into one by fitting a line through them using a first order least squares fit. 

![sorted_lines_img](https://user-images.githubusercontent.com/37941576/81483415-cca99380-91f2-11ea-9894-ebc1775201b2.png)

#### Calculate and Send Lane Center

With the lanes finally isolated and in a convenient mathematical form, the center between the lane lines at a given lookahead distance can be easily calculated. That coordinates of that center pixel is stored in a string and sent via the TCP connection back to the simulator, where it is used to determine how to steer towards that center and therefore hopefully remain in the lane.

#### Visualization

The application I developed can optionally display the results of its image processing by plotting it in an external figure. Taking the time to update that plot in real time slows down the simulation a small amount, but it can be worth it to see more clearly how the application is performing. I chose to highlight the estimated left lane line in red and the right lane line in green to conform with the color codes used for port and starboard sides of a boat or airplane. The following is what the image looks like at the end of the computer vision pipeline.

![line_image_grouped](https://user-images.githubusercontent.com/37941576/81483402-b3084c00-91f2-11ea-826a-dd3b8bd3f31c.png)

The python script I used to generate these images is located [here](https://github.com/corwinjs/AVLaneFollowingSimulation/tree/gh-pages/image-generation).

## Simulation

I built my simulator code on top of the Vehicle template that comes included with the Unreal Engine. This template comes with a simple asphalt world with lighting already set up, as well as a car that can be controlled using arrow keys on the keyboard. The car also comes with two cameras already set up: one behind the car to provide a third person view of the car during gameplay, and one inside the car in the driver’s seat for a more realistic driving perspective. Having these components already set up was very useful and let me begin work on the simulator-specific code much more quickly.

<!--
![new-project](https://user-images.githubusercontent.com/37941576/81484174-7b9c9e00-91f8-11ea-88a2-3394c49eb5b9.png)
-->

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/81484174-7b9c9e00-91f8-11ea-88a2-3394c49eb5b9.png" alt="new-project"/>
</p>

I kept the third person camera where it was and moved the internal camera out to the front and center of the vehicle to serve as the camera that will be sending its view to the computer vision application. 

<!--
![car-and-cameras](https://user-images.githubusercontent.com/37941576/81484157-645db080-91f8-11ea-98d0-3da5787c7572.png)
-->

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/81484157-645db080-91f8-11ea-98d0-3da5787c7572.png" alt="car-and-cameras"/>
</p>

I also removed most of the content that came with the map and drew a racetrack using landscape splines. The lines of this track will serve as the lanes that the vehicle follows.

<!--
![track](https://user-images.githubusercontent.com/37941576/81484185-940cb880-91f8-11ea-8dc5-5ddfb7ec1e70.png)
-->

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/81484185-940cb880-91f8-11ea-8dc5-5ddfb7ec1e70.png" alt="track"/>
</p>

The first step I took was to set up the communication pipeline from the camera’s view to a TCP message. To do this I needed to add a Render Target, which is essentially a struct that holds an image, for the camera to write its contents to. I then had to put the contents of the Render Target into an array of uint8 values and send it via a TCP connection. Fortunately, Unreal comes with libraries for TCP communication, and those libraries work the same on all supported platforms including Windows, Linux, and MacOS. However, the library is cumbersome to use directly, requiring low level knowledge of sockets and multiple function calls to accomplish conceptually simple tasks such as connecting and sending. Because of this, I took the time to write [my own library](https://github.com/corwinjs/AVLaneFollowingSimulation/blob/master/VehicleVision/Source/VehicleVision/TCPClientSocket.h) that abstracts away a lot of the details of the socket setup and used that in the simulator.  

At each time step, the simulator sends out message containing the camera image data to the computer vision application and receives a message containing a string that says where the center point of the lane in the image is. The string is in the format “column, row”, where the values are the row and column of the pixel within the sent image that corresponds to the lane center point. I set up a very basic lateral control scheme that finds the difference between the center point column received with the image center (512 for an image width of 1024 pixels). This difference is fed to the controller as the error signal. The vehicle’s steering wheel angle is adjusted proportional to that error in an effort to drive that error to zero, which is the same as steering towards the center of the lane. I also created a simple longitudinal controller that uses proportional control to adjust the throttle to maintain the vehicle speed at a specified target speed throughout the entire simulation.  

There are a lot of parameters that are useful to be able to tune when matching the simulator with a computer vision application. During the process of building up my application, I quickly found it tedious to make small changes to these parameters and recompile the whole project every time I wanted to try a different set of values. I decided to develop a process to read a configuration value containing those values instead, so that the values can be set at runtime instead of having to recompile the code. The following configuration file contains parameters for one possible working configuration. 

<!--
![config](https://user-images.githubusercontent.com/37941576/81484164-6e7faf00-91f8-11ea-9664-f017383d84f5.png)
-->

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/81484164-6e7faf00-91f8-11ea-9664-f017383d84f5.png" alt="config"/>
</p>

## Results

Both the lane detection program and the simulator work pretty well! The car is able to stay within its lane on the racetrack perpetually, and the lane detection program is always able to correctly isolate the lane lines within images it receives and determine the lane center. Here is a brief gif showing a screen capture I took with the simulator running on the right and the lane detection program running on the left.

<p align="center">
  <img src="https://user-images.githubusercontent.com/37941576/79151577-87bf4800-7d7f-11ea-9401-d91e8a487f54.gif" alt="VehicleVisionDemoTrimmed"/>
</p>

Both the lane detection and the simulator have a ton of potential for improvements and additions. It is immediately obvious that the racetrack I quickly put together is nowhere near a realistic driving environment. Real roads have multiple lanes, faded or missing lane lines, intersections, sidewalks, and potholes. There will be other cars, pedestrians, and animals that need to be avoided as well. The time of day will produce different lighting, trees will cast shade, rain will get on camera lenses. In short, there is so much in the real world that is not represented in my simulator. My lane detection program is good enough to work with my nearly empty simulator world, but would it work in a more realistic setting? I hope it would be somewhat robust to some more realism, but it would absolutely fail in a real-world setting. I got away with doing basic line detection in my program, but more advanced feature detection would be necessary to determine what other objects are as they are added to the simulator. If you have any interest in picking up where I am leaving off, please feel free to! In the next section I'll talk about where to find my software and how to get it running.

## Run It Yourself

All you need to run my lane detection program, called LaneTracker.py, on your computer is Python 3 with the [Pillow](https://pypi.org/project/Pillow/), [Numpy](https://pypi.org/project/numpy/), [Matplotlib](https://pypi.org/project/matplotlib/), and [OpenCV](https://pypi.org/project/opencv-python/) libraries. Building and running my simulator source code requires that you have version 4.22 or later of Unreal Engine. While Unreal is free, after installing it takes up over 100GB of hard drive space which I recognize can be prohibitive. Also, while Unreal is built on C++, using the Unreal Editor requires some work to learn and use effectively. In an effort to make the simulator more accessible, I have created precompiled packaged versions of my Unreal simulator program, called VehicleVision, for Windows, Mac, and Linux, so that users have the option to use those without the need to install or know how to use Unreal at all.

For those who do not have Unreal installed or simply want a more lightweight method of running the simulator with a computer vision application in the loop (either my LaneTracker software or one that you write), please download the appropriate package for your computer from the [releases section of my GitHub repo](https://github.com/corwinjs/AVLaneFollowingSimulation/releases).

If you have Unreal Engine installed and you are interested in building my simulator from source, you can clone, fork, or download my project from the project's [home GitHub page](https://github.com/corwinjs/AVLaneFollowingSimulation/).

Instructions for running the simulation using either the precompiled or the built from source simulator can be found in the readme on the project's [home GitHub page](https://github.com/corwinjs/AVLaneFollowingSimulation/tree/master#av-lane-following-simulation).

The python script I used to generate the images for the computer vision walkthrough is located [here](https://github.com/corwinjs/AVLaneFollowingSimulation/tree/gh-pages/image-generation).

<!--
![right-turn](https://user-images.githubusercontent.com/37941576/81484178-8820f680-91f8-11ea-8c16-6ca3e4b0382a.png)
![straightaway](https://user-images.githubusercontent.com/37941576/81484180-8ce5aa80-91f8-11ea-8fad-f473382d4872.png)
-->
