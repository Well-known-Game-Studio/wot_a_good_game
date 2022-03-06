# copied from https://blender.stackexchange.com/questions/173764/can-i-get-number-of-edges-connected-to-a-vertex

import bpy, bmesh
# Get selected object's mesh
mesh = bpy.context.object.data  
# Create new edit mode bmesh to easily acces mesh data
bm = bmesh.from_edit_mesh(mesh) 
# go through all the vertices of the model
for v in bm.verts:
    # select only vertices which have 6 (all) edges
    correct_edges = len(v.link_edges) == 6
    # and which are connected to all possible 12 faces
    correct_faces = len(v.link_faces) == 12
    v.select_set(correct_edges and correct_faces)
# Transfer the data back to the object's mesh
bmesh.update_edit_mesh(mesh)