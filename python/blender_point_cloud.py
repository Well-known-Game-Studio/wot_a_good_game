import bpy
from PIL import Image
import numpy as np
import json
import os

# Set working directory
os.chdir("/Users/bob/well-known-game-studio/wot_a_good_game/python")

# Load city matches
with open("city_matches.json", "r") as f:
    matches = json.load(f)

# Load map image
img = Image.open("processed_map.png").convert("RGB")
arr = np.array(img)

def close_color(rgb, target, tol=30):
    return all(abs(int(a) - int(b)) <= tol for a, b in zip(rgb, target))

def classify_pixel(rgb):
    # if close_color(rgb, (201, 229, 178)):  # standard land
    #     return 'land'
    # elif close_color(rgb, (59, 131, 21)):
    #     return 'forest'
    # elif close_color(rgb, (165, 192, 222)):
    #     return 'water'
    # elif close_color(rgb, (31, 43, 133)):
    #     return 'river' # 0x1F2B85
    # elif close_color(rgb, (203, 167, 139)):
    #     return 'mountain_region'
    # elif close_color(rgb, (183, 133, 92)):
    #     return 'mountain'
    # elif close_color(rgb, (216, 198, 133)):
    #     return 'blight'
    # else:
    #     return 'other'
    if close_color(rgb, (255,255,255)):
        return 'land'
    elif close_color(rgb, (0, 0, 0)):
        return 'land'
    elif close_color(rgb, (59, 131, 21)):
        return 'forest'
    elif close_color(rgb, (165, 192, 222)):
        return 'water'
    elif close_color(rgb, (67, 69, 255)):
        return 'river'
    elif close_color(rgb, (129, 129, 129)):
        return 'mountain'
    elif close_color(rgb, (65, 65, 65)):
        return 'mountain'
    elif close_color(rgb, (216, 198, 133)):
        return 'blight'
    else:
        return 'land'

def pixel_to_world(x, y, img_shape, scale=0.1):
    cx, cy = img_shape[1] // 2, img_shape[0] // 2
    wx = (x - cx) * scale
    wy = (cy - y) * scale  # Invert y for Blender
    return wx, wy

# Clear scene
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete()

# --- Create a point cloud mesh for the landscape ---
import bmesh

verts = []
colors = []

shape = arr.shape
for y in range(shape[1]):
    for x in range(shape[0]):
        terrain = classify_pixel(arr[y, x])
        if terrain in ['land', 'forest', 'mountain', 'mountain_region', 'water', 'river', 'blight']:
            wx, wy = pixel_to_world(x, y, arr.shape)
            verts.append((wx, wy, 0))
            # Store color as a tuple for later use (optional)
            color = {
                'land': (0.34, 0.53, 0.23, 1),
                'forest': (0.2, 0.4, 0.1, 1),
                'mountain': (0.6, 0.6, 0.6, 1),
                'mountain_region': (0.7, 0.5, 0.3, 1),
                'water': (0.2, 0.4, 0.8, 1),
                'river': (0.1, 0.3, 0.7, 1),
                'blight': (0.5, 0.5, 0.2, 1),
            }[terrain]
            colors.append(color)

# Create mesh with only vertices (no faces/edges)
mesh = bpy.data.meshes.new("VoxelPointCloud")
mesh.from_pydata(verts, [], [])
obj = bpy.data.objects.new("VoxelPointCloud", mesh)
bpy.context.collection.objects.link(obj)

# Add vertex color attribute for terrain type
if "Col" not in mesh.color_attributes:
    mesh.color_attributes.new(name="Col", type='FLOAT_COLOR', domain='POINT')
col_attr = mesh.color_attributes["Col"]

for i, color in enumerate(colors):
    col_attr.data[i].color = color

# --- Place houses at detected city locations as empties (for instancing) ---
for m in matches:
    wx, wy = pixel_to_world(m['pixel'][0], m['pixel'][1], arr.shape)
    empty = bpy.data.objects.new(f"House_{m['city'].replace(' ', '_')}", None)
    empty.location = (wx, wy, 0.15)
    bpy.context.collection.objects.link(empty)

print("Point cloud and city empties created. Use Geometry Nodes to instance cubes/meshes at these points.")
