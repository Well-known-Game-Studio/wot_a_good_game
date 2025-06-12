import cv2
import numpy as np
from PIL import Image
import pytesseract

import argparse

# Argument parser for command line usage
parser = argparse.ArgumentParser(description='Clean map image by inpainting text and icons.')
parser.add_argument('input_image', type=str, help='Path to the input map image.')
parser.add_argument('output_image', type=str, help='Path to save the cleaned map image.')
parser.add_argument('--debug_mask', type=str, default=None, help='Optional path to save the mask image.')
args = parser.parse_args()

input_image = args.input_image
output_image = args.output_image

# Load image
img = cv2.imread(input_image)
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# 1. Detect text regions with pytesseract
boxes = pytesseract.image_to_boxes(Image.fromarray(gray))
mask = np.zeros(gray.shape, dtype=np.uint8)
for b in boxes.splitlines():
    b = b.split(' ')
    x1, y1, x2, y2 = int(b[1]), int(b[2]), int(b[3]), int(b[4])
    # Tesseract's y is from bottom, OpenCV's from top
    y1 = img.shape[0] - y1
    y2 = img.shape[0] - y2
    cv2.rectangle(mask, (x1, y2), (x2, y1), 255, -1)

# 2. Detect icons/flags by color/contour (find saturated or non-green regions)
hsv = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
sat = hsv[...,1]
# Threshold for high saturation (likely icons/flags)
icon_mask = cv2.inRange(sat, 100, 255)
# Remove green areas (likely not icons)
green_mask = cv2.inRange(hsv, (35, 40, 40), (85, 255, 255))
icon_mask = cv2.bitwise_and(icon_mask, cv2.bitwise_not(green_mask))
# Find contours and add to mask
contours, _ = cv2.findContours(icon_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
for cnt in contours:
    area = cv2.contourArea(cnt)
    if area > 100:  # Only large enough regions
        cv2.drawContours(mask, [cnt], -1, 255, -1)

# 3. Dilate and blur mask to cover edges
mask = cv2.dilate(mask, np.ones((5,5), np.uint8), iterations=1)
mask = cv2.medianBlur(mask, 5)

# (Optional) Save mask for debugging
if args.debug_mask:
    cv2.imwrite(args.debug_mask, mask)

# 4. Inpaint with both methods and blend
inpainted_telea = cv2.inpaint(img, mask, 7, cv2.INPAINT_TELEA)
inpainted_ns = cv2.inpaint(img, mask, 7, cv2.INPAINT_NS)
# Blend the two results for smoother output
inpainted = cv2.addWeighted(inpainted_telea, 0.6, inpainted_ns, 0.4, 0)

# Save result
cv2.imwrite(output_image, inpainted)
