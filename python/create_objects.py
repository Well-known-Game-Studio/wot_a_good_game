import bpy
from mathutils import Vector

# make some materials for the different colors we'll use to keep material could low
def create_material(name, color):
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs[0].default_value = color  # Set base color
    # to make our lives easier in the future (to bake materials into textures)
    # we'll add a texture node
    tex_node = mat.node_tree.nodes.new('ShaderNodeTexImage')
    return mat

# make a function which will take in an object and ensure the object is not a
# parent heirarchy but instead is a single mesh (unparent and join all children)
def unparent_and_join(parent):
    # Unparent all children, keep transforms
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.select_all(action='DESELECT')
    parent.select_set(True)
    for child in parent.children:
        child.select_set(True)
    bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')

    # Only select the former children (now siblings) that were just unparented
    mesh_children = [obj for obj in bpy.context.selected_objects if obj.type == 'MESH' and obj.parent is None]
    for obj in mesh_children:
        obj.select_set(True)
    # Set the first mesh as active
    if mesh_children:
        bpy.context.view_layer.objects.active = mesh_children[0]
        bpy.ops.object.join()
        joined_obj = bpy.context.active_object
    else:
        joined_obj = None

    # Remove the empty parent if it still exists and is not the joined mesh
    if parent.name in bpy.context.scene.objects and parent != joined_obj:
        bpy.data.objects.remove(parent, do_unlink=True)

    return joined_obj

# make a function to ensure the pivot point for the passed in object is in the
# bottom center of the object's bounding box
def set_pivot_to_bottom_center(obj):
    # Deselect all, select only the object
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    # Get the bounding box in world coordinates
    bbox = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    min_z = min(v.z for v in bbox)
    center_x = sum(v.x for v in bbox) / 8
    center_y = sum(v.y for v in bbox) / 8
    bottom_center = Vector((center_x, center_y, min_z))

    # Move the 3D cursor to the bottom center
    bpy.context.scene.cursor.location = bottom_center

    # # Calculate the offset from the object's origin to the bottom center
    # offset = obj.location - bottom_center

    # # Move the object so the bottom center is at the desired location
    # obj.location = obj.location + offset

    # Set the origin to the current location (which is now the bottom center)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='BOUNDS')

def create_pine_tree(location, scale=1.0):
    parent = bpy.data.objects.new('PineTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    # Trunk
    for h in range(3):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"PineTrunk_{h}"
        mat = create_material("TrunkMaterial", (0.4, 0.2, 0.05, 1))
        obj.data.materials.append(mat)
        voxels.append(obj)
    # Leaves (cone)
    for h in range(3, 6):
        size = 2 - (h-3)
        for x in range(-size, size+1):
            for y in range(-size, size+1):
                if abs(x)+abs(y) <= size:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+h*scale))
                    obj = bpy.context.active_object
                    obj.name = f"PineLeaf_{x}_{y}_{h}"
                    # Create a new material for the leaves
                    mat = create_material("LeafMaterial", (0.1, 0.5, 0.1, 1))
                    # set the input for the base color to the material specifically for this instance
                    mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.1, 0.5, 0.1, 1)
                    obj.data.materials.append(mat)
                    voxels.append(obj)
    # Parent all voxels
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

def create_round_tree(location, scale=1.0):
    parent = bpy.data.objects.new('RoundTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    for h in range(3):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"RoundTrunk_{h}"
        # Create a new material for the trunk
        mat = create_material("TrunkMaterial", (0.4, 0.2, 0.05, 1))
        obj.data.materials.append(mat)
        voxels.append(obj)
    for x in range(-2, 3):
        for y in range(-2, 3):
            for z in range(3, 6):
                if x**2 + y**2 + (z-4)**2 <= 4:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+z*scale))
                    obj = bpy.context.active_object
                    obj.name = f"RoundLeaf_{x}_{y}_{z}"
                    # Create a new material for the leaves
                    mat = create_material("LeafMaterial", (0.2, 0.7, 0.2, 1))
                    # make sure the color is set as the base color
                    mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.2, 0.7, 0.2, 1)
                    obj.data.materials.append(mat)
                    voxels.append(obj)
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

def create_bushy_tree(location, scale=1.0):
    parent = bpy.data.objects.new('BushyTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    for h in range(2):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"BushyTrunk_{h}"
        # Create a new material for the trunk
        # and set the base color
        mat = create_material("TrunkMaterial", (0.4, 0.2, 0.05, 1))
        mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.4, 0.2, 0.05, 1)
        obj.data.materials.append(mat)
        voxels.append(obj)
    for x in range(-2, 3):
        for y in range(-2, 3):
            for z in range(2, 4):
                if abs(x)+abs(y) <= 3:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+z*scale))
                    obj = bpy.context.active_object
                    obj.name = f"BushyLeaf_{x}_{y}_{z}"
                    # Create a new material for the leaves
                    mat = create_material("LeafMaterial", (0.15, 0.6, 0.15, 1))
                    # set the base color for the leaves
                    mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.15, 0.6, 0.15, 1)
                    obj.data.materials.append(mat)
                    voxels.append(obj)
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

def create_mountain(location, scale=1.0):
    parent = bpy.data.objects.new('Mountain', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    for h in range(9):
        size = max(1, 4-h//2)
        for x in range(-size, size+1):
            for y in range(-size, size+1):
                if abs(x)+abs(y) <= size:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+h*scale))
                    obj = bpy.context.active_object
                    obj.name = f"Mountain_{x}_{y}_{h}"
                    # Create a new material for the mountain
                    # and set the base color
                    mat = create_material("MountainMaterial", (0.5, 0.5, 0.5, 1))
                    mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.5, 0.5, 0.5, 1)
                    obj.data.materials.append(mat)
                    voxels.append(obj)
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

def create_house(location, scale=1.0):
    parent = bpy.data.objects.new('House', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    for x in range(-1, 2):
        for y in range(-1, 2):
            for z in range(0, 2):
                bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+z*scale))
                obj = bpy.context.active_object
                obj.name = f"HouseWall_{x}_{y}_{z}"
                # Create a new material for the walls
                mat = create_material("HouseWallMaterial", (0.8, 0.7, 0.5, 1))
                # set the base color for the walls
                mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.8, 0.7, 0.5, 1)
                obj.data.materials.append(mat)
                voxels.append(obj)
    for h in range(2):
        for x in range(-1+h, 2-h):
            for y in range(-1+h, 2-h):
                bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+(2+h)*scale))
                obj = bpy.context.active_object
                obj.name = f"HouseRoof_{x}_{y}_{2+h}"
                # Create a new material for the roof
                # and set the base color
                mat = create_material("HouseRoofMaterial", (0.5, 0.2, 0.1, 1))
                # set the base color for the roof
                mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.5, 0.2, 0.1, 1)
                obj.data.materials.append(mat)
                voxels.append(obj)
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

def create_big_building(location, scale=1.0):
    parent = bpy.data.objects.new('BigBuilding', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    for x in range(-2, 3):
        for y in range(-1, 2):
            for z in range(0, 3):
                bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+z*scale))
                obj = bpy.context.active_object
                obj.name = f"BigBuildingWall_{x}_{y}_{z}"
                # Create a new material for the walls
                # and set the base color
                mat = create_material("BigBuildingWallMaterial", (0.7, 0.7, 0.8, 1))
                # set the base color for the walls
                mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.7, 0.7, 0.8, 1)
                obj.data.materials.append(mat)
                voxels.append(obj)
    for x in range(-2, 3):
        for y in range(-1, 2):
            bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+(3.0*scale)))
            print(f"Creating roof voxel at: {location[0]+x*scale}, {location[1]+y*scale}, {location[2]+(3.0*scale)}")
            obj = bpy.context.active_object
            obj.name = f"BigBuildingRoof_{x}_{y}_3"
            # Create a new material for the roof
            # and set the base color
            mat = create_material("BigBuildingRoofMaterial", (0.3, 0.15, 0.05, 1))
            # set the base color for the roof
            mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.3, 0.15, 0.05, 1)
            obj.data.materials.append(mat)
            voxels.append(obj)
    for obj in voxels:
        obj.select_set(True)
    parent.select_set(True)
    bpy.context.view_layer.objects.active = parent
    bpy.ops.object.parent_set(type='OBJECT')
    for obj in voxels:
        obj.select_set(False)
    parent.select_set(False)
    return parent

# now we can create the objects in the scene
def create_objects(scale=1.0):
    # Clear existing objects
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    # Create objects at specified locations
    objs = [
        create_pine_tree((0, 0, 0), scale=scale),
        create_round_tree((2, 2, 0), scale=scale),
        create_bushy_tree((-2, -2, 0), scale=scale),
        create_mountain((4, -4, 0), scale=scale),
        create_house((1, -1, 0), scale=scale),
        create_big_building((-3, 3, 0), scale=scale),
    ]
    for obj in objs:
        joined_obj = unparent_and_join(obj)
        if joined_obj:
            set_pivot_to_bottom_center(joined_obj)

# Run the function to create objects
if __name__ == "__main__":
    create_objects(0.25)
