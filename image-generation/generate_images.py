# https://medium.com/@mrhwick/simple-lane-detection-with-opencv-bfeb6ae54ec0

import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import numpy as np
import cv2
import math
import pdb
from PIL import Image

def getMaskedImage(img, vertices):
    mask = np.zeros_like(img)
    match_mask_color = 255
    
    cv2.fillPoly(mask, vertices, match_mask_color)
    masked_image = cv2.bitwise_and(img, mask)
    return masked_image

def addLinesToImage(img, lines, color=[255, 0, 0], thickness=3):
    # If there are no lines to draw, exit.
    if lines is None:
        return
    # Make a copy of the original image.
    img = np.copy(img)
    # Create a blank image that matches the original in size.
    line_img = np.zeros(
        (
            img.shape[0],
            img.shape[1],
            3
        ),
        dtype=np.uint8,
    )
    # Loop over all lines and draw them on the blank image.
    for line in lines:
        for x1, y1, x2, y2 in line:
            cv2.line(line_img, (x1, y1), (x2, y2), color, thickness)
    # Merge the image with the lines onto the original.
    img = cv2.addWeighted(img, 1.0, line_img, 1.0, 0.0)
    # Return the modified image.
    return img

def sortLines(lines):
    first_right = True
    first_left = True
    right_lines = None
    left_lines = None
    for line in lines:
        for x1, y1, x2, y2 in line:
            slope = (y2 - y1) / (x2 - x1) # <-- Calculating the slope.
            if math.fabs(slope) < 0.5: # <-- Only consider extreme slope
                continue
            if slope <= 0: # <-- If the slope is negative, left group.
                if first_left:
                    left_lines = np.array([x1, y1, x2, y2])
                    first_left = False
                else:
                     left_lines = np.vstack((left_lines, np.array([x1, y1, x2, y2])))
            else: # <-- Otherwise, right group.
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

def saveImage(image_array, filename):
    image = Image.fromarray(image_array)
    image.save(filename)

save_images = True
plot_images = False

image = mpimg.imread('original.jpg')
if plot_images:
    plt.imshow(image)
    plt.show()
if save_images:
    saveImage(image, "images/original.png")

height = image.shape[0]
width = image.shape[1]

gray_image = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
if plot_images:
    plt.imshow(gray_image, cmap='gray', vmin=0, vmax=255)
    plt.show()
if save_images:
    saveImage(gray_image, "images/gray_image.png")

kernel_size = 5
gauss_img = cv2.GaussianBlur(gray_image,(kernel_size, kernel_size), 0)
if plot_images:
    plt.imshow(gauss_img, cmap='gray', vmin=0, vmax=255)
    plt.show()
if save_images:
    saveImage(gauss_img, "images/gauss_img.png")

cannyed_image = cv2.Canny(gauss_img, 100, 200)
if plot_images:
    plt.imshow(cannyed_image)
    plt.show()
if save_images:
    saveImage(cannyed_image, "images/cannyed_image.png")

cannyed_image_raw = cv2.Canny(gray_image, 100, 200)
if plot_images:
    plt.imshow(cannyed_image_raw)
    plt.show()
if save_images:
    saveImage(cannyed_image_raw, "images/cannyed_image_raw.png")

end_width = 200
mask_vertices = np.array([[(0, height),
    (int(width/2 - end_width/2), int(.4*height)),
    (int(width/2 + end_width/2), int(.4*height)),
    (width, height)]])
cropped_image = getMaskedImage(cannyed_image, mask_vertices)
if plot_images:
    plt.imshow(cropped_image)
    plt.show()
if save_images:
    saveImage(cropped_image, "images/cropped_image.png")

mask = np.zeros_like(image)
match_mask_color = 255
cv2.fillPoly(mask, mask_vertices, match_mask_color)
blendimg = cv2.addWeighted(mask, .5, image, .5, 0)
if plot_images:
    plt.imshow(blendimg)
    plt.show()
if save_images:
    saveImage(blendimg, "images/blendimg.png")

lines = cv2.HoughLinesP(
    cropped_image,
    rho=6,
    theta=np.pi / 60,
    threshold=160,
    lines=np.array([]),
    minLineLength=40,
    maxLineGap=25
)

RED = [255, 0, 0]
GREEN = [0, 255, 0]
BLUE = [0, 0, 255]
YELLOW = [255, 255, 0]
CYAN = [0, 255, 255]
MAGENTA = [255, 0, 255]
COLORS = [RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA]

cropped_image_color = cv2.cvtColor(cropped_image,cv2.COLOR_GRAY2RGB)

raw_lines_img = np.copy(cropped_image_color)
i_color = 0
for line in lines:
    line_array = np.array([line[0][0], line[0][1], line[0][2], line[0][3]])
    raw_lines_img = addLinesToImage(
        raw_lines_img, [[line_array]], color=COLORS[i_color], thickness=10,
    )
    i_color = (i_color + 1) % len(COLORS)
if plot_images:
    plt.imshow(raw_lines_img)
    plt.show()
if save_images:
    saveImage(raw_lines_img, "images/raw_lines_img.png")

(right_lines, left_lines) = sortLines(lines)

left_lines_img = np.copy(cropped_image_color)
i_color = 0
for line in left_lines:
    line_array = np.array([line[0], line[1], line[2], line[3]])
    left_lines_img = addLinesToImage(
        left_lines_img, [[line_array]], color=COLORS[i_color], thickness=10,
    )
    i_color = (i_color + 1) % len(COLORS)
if plot_images:
    plt.imshow(left_lines_img)
    plt.show()
if save_images:
    saveImage(left_lines_img, "images/left_lines_img.png")

right_lines_img = np.copy(cropped_image_color)
i_color = 0
for line in right_lines:
    line_array = np.array([line[0], line[1], line[2], line[3]])
    right_lines_img = addLinesToImage(
        right_lines_img, [[line_array]], color=COLORS[i_color], thickness=10,
    )
    i_color = (i_color + 1) % len(COLORS)
if plot_images:
    plt.imshow(right_lines_img)
    plt.show()
if save_images:
    saveImage(right_lines_img, "images/right_lines_img.png")


min_y = int(image.shape[0] * (2 / 5)) # <-- Just below the horizon
max_y = int(image.shape[0]) # <-- The bottom of the image

left_line = getConsolidatedLine(left_lines, min_y, max_y)
right_line = getConsolidatedLine(right_lines, min_y, max_y)



sorted_lines_img = np.copy(cropped_image_color)
i_color = 0
sorted_lines_img = addLinesToImage(
    sorted_lines_img, [[left_line]], color=RED, thickness=10,
)
sorted_lines_img = addLinesToImage(
    sorted_lines_img, [[right_line]], color=GREEN, thickness=10,
)
if plot_images:
    plt.imshow(sorted_lines_img)
    plt.show()
if save_images:
    saveImage(sorted_lines_img, "images/sorted_lines_img.png")



color_image = cv2.cvtColor(gray_image,cv2.COLOR_GRAY2RGB)
line_image_grouped = addLinesToImage(
    color_image, [[left_line]], color=RED, thickness=10,
)
line_image_grouped = addLinesToImage(
    line_image_grouped, [[right_line]], color=GREEN, thickness=10,
)
if plot_images:
    plt.imshow(line_image_grouped)
    plt.show()
if save_images:
    saveImage(line_image_grouped, "images/line_image_grouped.png")