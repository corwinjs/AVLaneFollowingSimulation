import socket
import pdb
from PIL import Image
import numpy as np
import matplotlib.pyplot as plt
import cv2
import math

# Camera dimensions in pixels, must match the simulator's image dimensions
height = 512
width = 1024
# Lookahead distance in terms of pixels from the bottom of the received image
yloc = 175
# If true, plot image that visualizes image processing in real time
visualize = True

def getMaskedImage(img, vertices):
    mask = np.zeros_like(img)
    match_mask_color = 255
    
    cv2.fillPoly(mask, vertices, match_mask_color)
    masked_image = cv2.bitwise_and(img, mask)
    return masked_image

def sortLines(lines):
    first_right = True
    first_left = True
    right_lines = None
    left_lines = None
    for line in lines:
        for x1, y1, x2, y2 in line:
            slope = (y2 - y1) / (x2 - x1)
            if math.fabs(slope) < 0.5: # Only consider larger slopes
                continue
            if slope <= 0:
                if first_left:
                    left_lines = np.array([x1, y1, x2, y2])
                    first_left = False
                else:
                     left_lines = np.vstack((left_lines, np.array([x1, y1, x2, y2])))
            else:
                if first_right:
                    right_lines = np.array([x1, y1, x2, y2])
                    first_right = False
                else:
                    right_lines = np.vstack((right_lines, np.array([x1, y1, x2, y2])))
    return (right_lines, left_lines)

def getConsolidatedLine(lines, ymin, ymax):
    y_coordinates = np.concatenate((lines[:,1], lines[:,3]))
    x_coordinates = np.concatenate((lines[:,0], lines[:,2]))
    poly_fit_function = np.poly1d(np.polyfit(
        y_coordinates,
        x_coordinates,
        deg=1
    ))
    x_start = int(poly_fit_function(ymin))
    x_end = int(poly_fit_function(ymax))
    consolidatedLine = np.array([x_start, ymin, x_end, ymax])
    return consolidatedLine

def addLinesToImage(img, lines, color=[255, 0, 0], thickness=3):
    if lines is None:
        return
    img = np.copy(img)
    line_img = np.zeros((img.shape[0], img.shape[1], 3), dtype=np.uint8)
    # Loop over all lines and draw them on the blank image.
    for line in lines:
        for x1, y1, x2, y2 in line:
            cv2.line(line_img, (x1, y1), (x2, y2), color, thickness)
    img = cv2.addWeighted(img, 0.8, line_img, 1.0, 0.0)
    return img

def getCenterAtRow(line1, line2, row):
    slope1 = (line1[3] - line1[1])/(line1[2] - line1[0])
    intercept1 = (line1[0]*line1[3] - line1[2]*line1[1])/(line1[0] - line1[2])
    col1 = (row - intercept1)/slope1

    slope2 = (line2[3] - line2[1])/(line2[2] - line2[0])
    intercept2 = (line2[0]*line2[3] - line2[2]*line2[1])/(line2[0] - line2[2])
    col2 = (row - intercept2)/slope2

    center_column = int((col1 + col2)/2)
    return center_column

def highlightPoint(img, row, col):
    radius = 5
    for i in range(row - radius, row + radius + 1):
        for j in range(col - radius, col + radius + 1):
            img[i, j] = 255


# create a socket object
s = socket.socket()  
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)        
print("Socket successfully created")
# bind socket to a port (must match port Unreal simulator is using)
port = 27015                
s.bind(('', port))         
print("socket binded to " + str(port))
# put the socket into listening mode 
s.listen(5)
print("socket is listening")
# listen for clients and open a connection when one is detected
c, addr = s.accept()
print('Got connection from' + str(addr))

plt.ion()

while True:
    # Read Image Data From Client/Simulator
    message = c.recv(height*width, socket.MSG_WAITALL)
    data = [int(i) for i in message]
    image_out = Image.new("L",(width, height))
    image_out.putdata(data)
    image = np.array(image_out)

    # Gaussian Blur
    kernel_size = 5
    gauss_img = cv2.GaussianBlur(image,(kernel_size, kernel_size), 0)

    # Canny Edge Detection
    low_threshold = 100
    high_threshold = 200
    canny_img = cv2.Canny(gauss_img, low_threshold, high_threshold)

    # Crop Image to Region of Interest (hopefully lane lines)
    end_width = 200
    mask_vertices = np.array([[(0, height),
        (int(width/2 - end_width/2), int(.4*height)),
        (int(width/2 + end_width/2), int(.4*height)),
        (width, height)]])
    cropped_image = getMaskedImage(canny_img, mask_vertices)

    # Hough Line Detection
    hough_lines = cv2.HoughLinesP(
        cropped_image,
        rho=6,
        theta=np.pi / 60,
        threshold=160,
        lines=np.array([]),
        minLineLength=40,
        maxLineGap=25
    )

    # The first frame from Unreal may be black, in which case no lines will be found
    if hough_lines is None:
        center_x = int(width/2)
        center_y = int(height/2)
        locationString = str(center_y) + "," + str(center_x)
        c.send(locationString.encode())
        continue

    # Sort and Consolidate into Left and Right Lane Lines
    (right_lines, left_lines) = sortLines(hough_lines)
    min_y = int(.4*image.shape[0])
    max_y = int(image.shape[0])  # bottom of the image
    left_line = getConsolidatedLine(left_lines, min_y, max_y)
    right_line = getConsolidatedLine(right_lines, min_y, max_y)

    # Find Center Between Lane Lines
    center_y = height - yloc
    center_x = getCenterAtRow(left_line, right_line, center_y)

    # Visualize Image Processing
    color_image = cv2.cvtColor(image,cv2.COLOR_GRAY2RGB)
    line_image_grouped = addLinesToImage(
        color_image, [[left_line]], color=[255, 0, 0], thickness=5,
    )
    line_image_grouped = addLinesToImage(
        line_image_grouped, [[right_line]], color=[0, 255, 0], thickness=5,
    )
    highlightPoint(line_image_grouped, center_y, center_x)
    if visualize:
        plt.imshow(line_image_grouped)
        plt.show()
        plt.pause(.001)
        plt.cla()

    # Send detected lane center back to Unreal
    locationString = str(center_y) + "," + str(center_x)
    c.send(locationString.encode())

c.close() 