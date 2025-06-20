import bpy
from mathutils import Vector
import uuid

import os

# Ensure Cycles render engine is selected for baking support
for scene in bpy.data.scenes:
    scene.render.engine = 'CYCLES'

def export_each_object_to_fbx(objs, export_dir=None):
    # Use the directory of the current .blend file, or fallback to current working directory
    if export_dir is None:
        blend_path = bpy.data.filepath
        if blend_path:
            base_dir = os.path.dirname(blend_path)
        else:
            base_dir = os.getcwd()
        export_dir = os.path.join(base_dir, "exports")
    if not os.path.exists(export_dir):
        os.makedirs(export_dir)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objs:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        export_path = os.path.join(export_dir, f"{obj.name}.fbx")
        bpy.ops.export_scene.fbx(
            filepath=export_path,
            use_selection=True,
            apply_unit_scale=True,
            bake_space_transform=True,
            object_types={'MESH'},
            path_mode='COPY',
            embed_textures=True
        )
        obj.select_set(False)

def export_each_object_to_gltf(objs, export_dir=None):
    # Use the directory of the current .blend file, or fallback to current working directory
    if export_dir is None:
        blend_path = bpy.data.filepath
        if blend_path:
            base_dir = os.path.dirname(blend_path)
        else:
            base_dir = os.getcwd()
        export_dir = os.path.join(base_dir, "exports")
    if not os.path.exists(export_dir):
        os.makedirs(export_dir)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objs:
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        export_path = os.path.join(export_dir, f"{obj.name}.glb")
        bpy.ops.export_scene.gltf(
            filepath=export_path,
            use_selection=True,
            export_format='GLB',
            export_materials='EXPORT',
            export_image_format='AUTO',
            export_texcoords=True,
            export_normals=True,
            export_colors=True,
            export_apply=True,
            export_yup=True,
            export_cameras=False,
            export_extras=True,
        )
        obj.select_set(False)

def delete_all_materials():
    for material in bpy.data.materials:
        material.user_clear()
        bpy.data.materials.remove(material)

def create_material(name, color, shared_image):
    if name in bpy.data.materials:
        mat = bpy.data.materials[name]
    else:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs[0].default_value = color  # Set base color

    # Ensure a single image texture node exists (for baking)
    tex_node = None
    for node in mat.node_tree.nodes:
        if node.type == 'TEX_IMAGE' and node.image == shared_image:
            tex_node = node
            break
    if not tex_node:
        tex_node = mat.node_tree.nodes.new('ShaderNodeTexImage')
        tex_node.image = shared_image

    # DO NOT connect tex_node to the shader!
    # Just leave it in the node tree for baking.

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

    # Set the origin to the current location (which is now the bottom center)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='BOUNDS')

def create_dragonmount(location, scale=1.0, shared_image=None):
    import bpy
    from mathutils import Vector

    parent = bpy.data.objects.new('Dragonmount', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    # Tall, conical mountain (volcano)
    for h in range(15):
        size = max(1, 8 - h // 2)
        for x in range(-size, size + 1):
            for y in range(-size, size + 1):
                if abs(x) + abs(y) <= size:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0] + x * scale, location[1] + y * scale, location[2] + h * scale))
                    obj = bpy.context.active_object
                    obj.name = f"Dragonmount_{x}_{y}_{h}"
                    mat = create_material(f"DragonmountMaterial_{parent.name}", (0.25, 0.22, 0.18, 1), shared_image)
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

def create_white_tower(location, scale=1.0, shared_image=None):
    import bpy
    from mathutils import Vector

    parent = bpy.data.objects.new('WhiteTower', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    # Tall, slender, white spire with a wider base
    for h in range(12):
        if h < 3:
            radius = 2
        elif h < 9:
            radius = 1
        else:
            radius = 0
        for x in range(-radius, radius + 1):
            for y in range(-radius, radius + 1):
                if x ** 2 + y ** 2 <= (radius + 0.2) ** 2:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0] + x * scale, location[1] + y * scale, location[2] + h * scale))
                    obj = bpy.context.active_object
                    obj.name = f"WhiteTower_{x}_{y}_{h}"
                    mat = create_material(f"WhiteTowerMaterial_{parent.name}", (0.95, 0.95, 1.0, 1), shared_image)
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

def create_stone_of_tear(location, scale=1.0, shared_image=None):
    import bpy
    from mathutils import Vector

    parent = bpy.data.objects.new('StoneOfTear', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    # Fortress: large, blocky, stepped structure
    for h in range(6):
        size = 4 - h // 2
        for x in range(-size, size + 1):
            for y in range(-size, size + 1):
                if abs(x) <= size and abs(y) <= size:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0] + x * scale, location[1] + y * scale, location[2] + h * scale))
                    obj = bpy.context.active_object
                    obj.name = f"StoneOfTear_{x}_{y}_{h}"
                    mat = create_material(f"StoneOfTearMaterial_{parent.name}", (0.7, 0.7, 0.75, 1), shared_image)
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

def create_pine_tree(location, scale=1.0, shared_image=None):
    parent = bpy.data.objects.new('PineTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    # Generate unique material names for this tree
    trunk_mat_name = f"PineTrunkMaterial"
    leaf_mat_name = f"PineLeafMaterial"
    # Trunk
    for h in range(3):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"PineTrunk_{h}"
        mat = create_material(trunk_mat_name, (0.4, 0.2, 0.05, 1), shared_image)
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
                    mat = create_material(leaf_mat_name, (0.1, 0.5, 0.1, 1), shared_image)
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

def create_round_tree(location, scale=1.0, shared_image=None):
    parent = bpy.data.objects.new('RoundTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    trunk_mat_name = f"RoundTrunkMaterial"
    leaf_mat_name = f"RoundLeafMaterial"
    for h in range(3):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"RoundTrunk_{h}"
        mat = create_material(trunk_mat_name, (0.4, 0.2, 0.05, 1), shared_image)
        obj.data.materials.append(mat)
        voxels.append(obj)
    for x in range(-2, 3):
        for y in range(-2, 3):
            for z in range(3, 6):
                if x**2 + y**2 + (z-4)**2 <= 4:
                    bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+z*scale))
                    obj = bpy.context.active_object
                    obj.name = f"RoundLeaf_{x}_{y}_{z}"
                    mat = create_material(leaf_mat_name, (0.2, 0.7, 0.2, 1), shared_image)
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

def create_bushy_tree(location, scale=1.0, shared_image=None):
    parent = bpy.data.objects.new('BushyTree', None)
    parent.location = location
    bpy.context.collection.objects.link(parent)
    voxels = []
    trunk_mat_name = f"BushyTrunkMaterial"
    leaf_mat_name = f"BushyLeafMaterial"
    for h in range(2):
        bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0], location[1], location[2]+h*scale))
        obj = bpy.context.active_object
        obj.name = f"BushyTrunk_{h}"
        mat = create_material(trunk_mat_name, (0.4, 0.2, 0.05, 1), shared_image)
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
                    mat = create_material(leaf_mat_name, (0.15, 0.6, 0.15, 1), shared_image)
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

def create_mountain(location, scale=1.0, shared_image=None):
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
                    mat = create_material("MountainMaterial", (0.5, 0.5, 0.5, 1), shared_image)
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

def create_house(location, scale=1.0, shared_image=None):
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
                mat = create_material("HouseWallMaterial", (0.8, 0.7, 0.5, 1), shared_image)
                mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.8, 0.7, 0.5, 1)
                obj.data.materials.append(mat)
                voxels.append(obj)
    for h in range(2):
        for x in range(-1+h, 2-h):
            for y in range(-1+h, 2-h):
                bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+(2+h)*scale))
                obj = bpy.context.active_object
                obj.name = f"HouseRoof_{x}_{y}_{2+h}"
                mat = create_material("HouseRoofMaterial", (0.5, 0.2, 0.1, 1), shared_image)
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

def create_big_building(location, scale=1.0, shared_image=None):
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
                mat = create_material("BigBuildingWallMaterial", (0.7, 0.7, 0.8, 1), shared_image)
                mat.node_tree.nodes["Principled BSDF"].inputs[0].default_value = (0.7, 0.7, 0.8, 1)
                obj.data.materials.append(mat)
                voxels.append(obj)
    for x in range(-2, 3):
        for y in range(-1, 2):
            bpy.ops.mesh.primitive_cube_add(size=scale, location=(location[0]+x*scale, location[1]+y*scale, location[2]+(3.0*scale)))
            print(f"Creating roof voxel at: {location[0]+x*scale}, {location[1]+y*scale}, {location[2]+(3.0*scale)}")
            obj = bpy.context.active_object
            obj.name = f"BigBuildingRoof_{x}_{y}_3"
            mat = create_material("BigBuildingRoofMaterial", (0.3, 0.15, 0.05, 1), shared_image)
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

    name_prefix = "small_prop_"

    # Create objects at specified locations
    obj_defs = [
        (create_pine_tree, (0, 0, 0), name_prefix + 'pine_tree'),
        (create_round_tree, (2, 2, 0), name_prefix + 'round_tree'),
        (create_bushy_tree, (-2, -2, 0), name_prefix + 'bushy_tree'),
        (create_mountain, (4, -4, 0), name_prefix + 'mountain'),
        (create_house, (1, -1, 0), name_prefix + 'house'),
        (create_big_building, (-3, 3, 0), name_prefix + 'big_building'),
    ]
    created_objs = []
    for create_fn, loc, obj_name in obj_defs:
        # Each object gets its own unique image
        tex_name = obj_name + "_tex"
        unique_image = get_or_create_unique_image(tex_name)
        obj = create_fn(loc, scale=scale, shared_image=unique_image)
        joined_obj = unparent_and_join(obj)
        if joined_obj:
            # rename the joined object
            joined_obj.name = obj_name
            set_pivot_to_bottom_center(joined_obj)
            auto_unwrap(joined_obj)
            bake_to_shared_image(joined_obj, unique_image, bake_type='DIFFUSE')
            connect_baked_texture_to_material(joined_obj, unique_image)
            created_objs.append(joined_obj)

    # Move all created objects to the origin
    for obj in created_objs:
        obj.location = (0, 0, 0)

    # Export each object to GLTF/GLB
    export_each_object_to_gltf(created_objs)

def get_or_create_unique_image(base_name="obj_tex", width=1024, height=1024):
    return bpy.data.images.new(base_name, width=width, height=height, alpha=True, float_buffer=False)

def auto_unwrap(obj):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.03)
    bpy.ops.object.mode_set(mode='OBJECT')

def bake_to_shared_image(obj, shared_image, bake_type='DIFFUSE'):
    # Ensure the image is in the object's material node tree and selected for baking
    for mat in obj.data.materials:
        for node in mat.node_tree.nodes:
            if node.type == 'TEX_IMAGE' and node.image == shared_image:
                mat.node_tree.nodes.active = node
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.ops.object.bake(type=bake_type, use_clear=True, margin=2)

def connect_baked_texture_to_material(obj, baked_image):
    for mat in obj.data.materials:
        nodes = mat.node_tree.nodes
        links = mat.node_tree.links
        # Find the Principled BSDF
        bsdf = nodes.get("Principled BSDF")
        # Find the baked image node
        tex_node = None
        for node in nodes:
            if node.type == 'TEX_IMAGE' and node.image == baked_image:
                tex_node = node
                break
        if bsdf and tex_node:
            # Remove any existing links to Base Color
            for link in list(bsdf.inputs[0].links):
                links.remove(link)
            # Connect the baked image to Base Color
            links.new(tex_node.outputs['Color'], bsdf.inputs[0])

# Run the function to create objects
if __name__ == "__main__":
    delete_all_materials()
    create_objects(0.25)
