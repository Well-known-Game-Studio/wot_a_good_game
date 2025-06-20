import bpy
from PIL import Image
import numpy as np
import json

# print the current working directory
import os
print("Current working directory:", os.getcwd())

# change to the script's directory
os.chdir("/Users/bob/well-known-game-studio/wot_a_good_game/python")

# Load city matches
with open("city_matches.json", "r") as f:
    matches = json.load(f)

# Load map image
img = Image.open("processed_map.png").convert("RGB")
arr = np.array(img)

def close_color(rgb, target, tol=10):
    return all(abs(int(a) - int(b)) <= tol for a, b in zip(rgb, target))

def classify_pixel(rgb):
    if close_color(rgb, (201, 229, 178)):  # standard land
        return 'land'
    elif close_color(rgb, (59, 131, 21)):
        return 'forest'
    elif close_color(rgb, (165, 192, 222)):
        return 'water'
    elif close_color(rgb, (31, 43, 133)):
        return 'river' # 0x1F2B85
    elif close_color(rgb, (203, 167, 139)):
        return 'mountain_region'
    elif close_color(rgb, (183, 133, 92)):
        return 'mountain'
    elif close_color(rgb, (216, 198, 133)):
        return 'blight'
    else:
        return 'other'

def pixel_to_world(x, y, img_shape, scale=0.1):
    cx, cy = img_shape[1] // 2, img_shape[0] // 2
    wx = (x - cx) * scale
    wy = (cy - y) * scale  # Invert y for Blender
    return wx, wy

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# Place ground and features (small region for demo)
for y in range(400, 600):
    for x in range(400, 600):
        terrain = classify_pixel(arr[y, x])
        if terrain in ['land', 'forest', 'mountain', 'mountain_region']:
            wx, wy = pixel_to_world(x, y, arr.shape)
            bpy.ops.mesh.primitive_cube_add(size=0.1, location=(wx, wy, 0))
            obj = bpy.context.active_object
            obj.name = f"Ground_{x}_{y}"
            color = {
                'land': (0.34, 0.53, 0.23, 1),
                'forest': (0.2, 0.4, 0.1, 1),
                'mountain': (0.6, 0.6, 0.6, 1),
                'mountain_region': (0.7, 0.5, 0.3, 1)
            }[terrain]
            mat = bpy.data.materials.new(f"{terrain}_mat")
            mat.use_nodes = True
            mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = color
            obj.data.materials.append(mat)

# Place houses at detected city locations
for m in matches:
    wx, wy = pixel_to_world(m['pixel'][0], m['pixel'][1], arr.shape)
    bpy.ops.mesh.primitive_cube_add(size=0.3, location=(wx, wy, 0.15))
    obj = bpy.context.active_object
    obj.name = f"House_{m['city'].replace(' ', '_')}"
    mat = bpy.data.materials.new("House_mat")
    mat.use_nodes = True
    mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.8, 0.7, 0.5, 1)
    obj.data.materials.append(mat)
