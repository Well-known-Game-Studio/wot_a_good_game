import bpy
import bmesh
import math
import gpu
from gpu_extras.batch import batch_for_shader
from bpy_extras import view3d_utils
from mathutils import Vector, Matrix

def get_or_create_material(color):
    """Checks for an existing material with the given color, or creates one."""
    
    # Format color name from RGBA values
    color_name = f"VoxelColor_{color[0]:.2f}_{color[1]:.2f}_{color[2]:.2f}_{color[3]:.2f}"
    
    # Check if material already exists
    if color_name in bpy.data.materials:
        return bpy.data.materials[color_name]
    
    # If not, create a new material
    mat = bpy.data.materials.new(name=color_name)
    mat.use_nodes = True
    principled_bsdf = mat.node_tree.nodes.get('Principled BSDF')
    if principled_bsdf:
        principled_bsdf.inputs['Base Color'].default_value = color
    
    return mat

class WOT_OT_VoxelBrush(bpy.types.Operator):
    """A modal operator that functions as a voxel brush tool."""
    bl_idname = "wot.voxel_brush_operator"
    bl_label = "Voxel Brush Operator"
    bl_options = {'REGISTER', 'UNDO'}

    def _get_snapped_location(self, context, event):
        """Perform a raycast and return the snapped grid location."""
        region = context.region
        rv3d = context.region_data
        coord = event.mouse_region_x, event.mouse_region_y

        # Raycast from mouse into the scene
        origin = view3d_utils.region_2d_to_origin_3d(region, rv3d, coord)
        ray_dir = view3d_utils.region_2d_to_vector_3d(region, rv3d, coord)

        # Blender 4.1+ uses different raycast logic
        try:
            result, location, normal, index, object, matrix = context.scene.ray_cast(context.view_layer.depsgraph, origin, ray_dir)
        except TypeError: # Older versions
            result, location, normal, index, object, matrix = context.scene.ray_cast(context.view_layer, origin, ray_dir)

        props = context.scene.wot_tool_props
        grid_size = props.voxel_size
        
        if result and object: # Snap to existing face
            snapped_location = location + normal * (grid_size / 2.0)
            return Vector((
                round(snapped_location.x / grid_size) * grid_size,
                round(snapped_location.y / grid_size) * grid_size,
                round(snapped_location.z / grid_size) * grid_size,
            ))
        else: # Snap to world grid (project onto XY plane)
            # If ray is parallel to the plane, this will fail, but it's a decent fallback
            if ray_dir.z != 0:
                t = -origin.z / ray_dir.z
                if t > 0: # Check if intersection is in front of the camera
                    point = origin + t * ray_dir
                    return Vector((
                        round(point.x / grid_size) * grid_size,
                        round(point.y / grid_size) * grid_size,
                        0.0,
                    ))

            # Fallback if ray is parallel to the plane, just use the ray origin snapped to grid
            return Vector((
                round(origin.x / grid_size) * grid_size,
                round(origin.y / grid_size) * grid_size,
                round(origin.z / grid_size) * grid_size,
            ))

    def _update_preview_batch(self, context):
        """Update the GPU batch for the preview voxel."""
        props = context.scene.wot_tool_props
        s = props.voxel_size / 2.0
        
        verts = [
            (self.preview_location[0]-s, self.preview_location[1]-s, self.preview_location[2]-s),
            (self.preview_location[0]+s, self.preview_location[1]-s, self.preview_location[2]-s),
            (self.preview_location[0]+s, self.preview_location[1]+s, self.preview_location[2]-s),
            (self.preview_location[0]-s, self.preview_location[1]+s, self.preview_location[2]-s),
            (self.preview_location[0]-s, self.preview_location[1]-s, self.preview_location[2]+s),
            (self.preview_location[0]+s, self.preview_location[1]-s, self.preview_location[2]+s),
            (self.preview_location[0]+s, self.preview_location[1]+s, self.preview_location[2]+s),
            (self.preview_location[0]-s, self.preview_location[1]+s, self.preview_location[2]+s),
        ]
        
        indices = (
            (0, 1), (1, 2), (2, 3), (3, 0), (4, 5), (5, 6),
            (6, 7), (7, 4), (0, 4), (1, 5), (2, 6), (3, 7)
        )
        
        self.batch = batch_for_shader(self.shader, 'LINES', {"pos": verts}, indices=indices)

    def modal(self, context, event):
        if event.type in {'RIGHTMOUSE', 'ESC'}:
            self._finish(context)
            return {'CANCELLED'}

        if event.type == 'MOUSEMOVE':
            if (event.mouse_x, event.mouse_y) != self.last_mouse_pos:
                self.preview_location = self._get_snapped_location(context, event)
                self._update_preview_batch(context)
                context.area.tag_redraw()
            self.last_mouse_pos = (event.mouse_x, event.mouse_y)

        elif event.type == 'LEFTMOUSE' and event.value == 'PRESS':
            if event.ctrl: # Voxel Removal
                self._remove_voxel(context)
            elif event.shift: # Voxel Painting
                self._paint_voxel(context)
            else: # Voxel Addition
                self._place_voxel(context)
            return {'RUNNING_MODAL'}

        return {'RUNNING_MODAL'}

    def invoke(self, context, event):
        self.shader = gpu.shader.from_builtin('UNIFORM_COLOR')
        self.batch = None
        self.draw_handle = None
        self.last_mouse_pos = (0, 0)
        self.preview_location = self._get_snapped_location(context, event)
        self._update_preview_batch(context)
        
        self.draw_handle = bpy.types.SpaceView3D.draw_handler_add(
            self.draw_callback, (context,), 'WINDOW', 'POST_VIEW'
        )
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def draw_callback(self, context):
        if self.batch:
            self.shader.bind()
            self.shader.uniform_float("color", (0.8, 0.8, 0.1, 0.5))
            self.batch.draw(self.shader)

    def _place_voxel(self, context):
        props = context.scene.wot_tool_props
        target_obj = context.active_object

        # If no object is selected, or the selected one isn't a mesh, create a new one
        if not target_obj or target_obj.type != 'MESH':
            bpy.ops.mesh.primitive_cube_add(size=props.voxel_size, location=self.preview_location)
            voxel_obj = context.active_object
            voxel_obj.name = "VoxelMesh"
            voxel_obj.data.materials.append(get_or_create_material(props.voxel_color))
            return

        # If we have a valid mesh object, add the voxel to it
        # Ensure we are in object mode to modify mesh data
        if context.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')

        # Get the material and add it to the object if it's not there
        mat = get_or_create_material(props.voxel_color)
        if mat.name not in target_obj.data.materials:
            target_obj.data.materials.append(mat)
        
        # Find the material index
        mat_index = target_obj.data.materials.find(mat.name)

        # Calculate the transformation matrix to place the cube correctly
        # in the target object's local space.
        
        # 1. Get the inverse of the target object's world matrix
        matrix_inv = target_obj.matrix_world.inverted()
        
        # 2. Create a new matrix for the desired voxel position and scale in world space
        mat_trans = Matrix.Translation(self.preview_location)
        mat_scale = Matrix.Scale(props.voxel_size, 4)
        mat_world_new_voxel = mat_trans @ mat_scale

        # 3. Combine them to get the final local transformation
        mat_local_new_voxel = matrix_inv @ mat_world_new_voxel
        
        # Use bmesh to add a cube to the existing mesh
        bm = bmesh.new()
        bm.from_mesh(target_obj.data)

        # Get sets of existing geometry to definitively find the new cube
        verts_before = set(bm.verts)
        faces_before = set(bm.faces)
        
        # Create a unit cube
        bmesh.ops.create_cube(bm, size=1.0)
        
        # Get the new geometry by taking the difference of the sets
        new_verts = list(set(bm.verts) - verts_before)
        new_faces = list(set(bm.faces) - faces_before)
        
        # Transform the new vertices to the correct location and scale
        bmesh.ops.transform(bm, verts=new_verts, matrix=mat_local_new_voxel)
        
        # Assign the material to the new faces
        for face in new_faces:
            face.material_index = mat_index

        # Update the mesh and free the bmesh
        bm.to_mesh(target_obj.data)
        bm.free()

        # Update the view
        target_obj.data.update()

    def _paint_voxel(self, context):
        """Finds and recolors a voxel from an existing mesh."""
        # We need a raycast to find what's under the mouse
        region = context.region
        rv3d = context.region_data
        coord = self.last_mouse_pos
        
        origin = view3d_utils.region_2d_to_origin_3d(region, rv3d, coord)
        ray_dir = view3d_utils.region_2d_to_vector_3d(region, rv3d, coord)

        try:
            result, location, normal, index, obj, matrix = context.scene.ray_cast(context.view_layer.depsgraph, origin, ray_dir)
        except TypeError:
            result, location, normal, index, obj, matrix = context.scene.ray_cast(context.view_layer, origin, ray_dir)

        # If we hit a mesh object, and it has faces
        if result and obj and obj.type == 'MESH' and len(obj.data.polygons) > 0:
            props = context.scene.wot_tool_props
            
            # Get the target material
            target_mat = get_or_create_material(props.voxel_color)
            if target_mat.name not in obj.data.materials:
                obj.data.materials.append(target_mat)
            
            target_mat_index = obj.data.materials.find(target_mat.name)

            # --- Switch to Edit Mode to safely select ---
            context.view_layer.objects.active = obj
            bpy.ops.object.mode_set(mode='EDIT')
            bpy.ops.mesh.select_all(action='DESELECT')

            # Select the hit face in edit mode
            bm = bmesh.from_edit_mesh(obj.data)
            bm.faces.ensure_lookup_table()
            if index < len(bm.faces):
                bm.faces[index].select = True
            
            bmesh.update_edit_mesh(obj.data)
            
            # Use the reliable bpy operator
            bpy.ops.mesh.select_linked(delimit=set())
            
            # Update the bmesh with the new selection
            bmesh.update_edit_mesh(obj.data)

            # Now assign the material to the selected faces
            for face in bm.faces:
                if face.select:
                    face.material_index = target_mat_index
            
            # Finalize the mesh and switch back to Object Mode
            bmesh.update_edit_mesh(obj.data)
            bpy.ops.object.mode_set(mode='OBJECT')

    def _remove_voxel(self, context):
        """Finds and removes a voxel from an existing mesh."""
        # We need a raycast to find what's under the mouse
        region = context.region
        rv3d = context.region_data
        coord = self.last_mouse_pos # Use the last known mouse position
        
        origin = view3d_utils.region_2d_to_origin_3d(region, rv3d, coord)
        ray_dir = view3d_utils.region_2d_to_vector_3d(region, rv3d, coord)

        try:
            result, location, normal, index, object, matrix = context.scene.ray_cast(context.view_layer.depsgraph, origin, ray_dir)
        except TypeError:
            result, location, normal, index, object, matrix = context.scene.ray_cast(context.view_layer, origin, ray_dir)

        # If we hit a mesh object, and it has faces
        if result and object and object.type == 'MESH' and len(object.data.polygons) > 0:
            
            # We need to make sure this is the active object to go into edit mode
            context.view_layer.objects.active = object

            # Switch to Edit Mode
            bpy.ops.object.mode_set(mode='EDIT')
            
            # Deselect all geometry first
            bpy.ops.mesh.select_all(action='DESELECT')
            
            # Get the mesh from the edit-mode object and select the face
            bm = bmesh.from_edit_mesh(object.data)
            bm.faces.ensure_lookup_table()
            if index < len(bm.faces):
                bm.faces[index].select = True
            
            # Update the bmesh to reflect the selection in the viewport
            bmesh.update_edit_mesh(object.data)

            # Now use the bpy operator to select linked geometry
            bpy.ops.mesh.select_linked(delimit=set())

            # And delete the selected vertices
            bpy.ops.mesh.delete(type='VERT')

            # Switch back to Object Mode
            bpy.ops.object.mode_set(mode='OBJECT')
        
    def _finish(self, context):
        bpy.types.SpaceView3D.draw_handler_remove(self.draw_handle, 'WINDOW')
        context.area.tag_redraw()

class WOT_OT_SelectVoxelColor(bpy.types.Operator):
    """Selects a voxel color from the palette."""
    bl_idname = "wot.select_voxel_color"
    bl_label = "Select Voxel Color"
    
    material_name: bpy.props.StringProperty()

    def execute(self, context):
        mat = bpy.data.materials.get(self.material_name)
        if not mat:
            return {'CANCELLED'}

        # Find the Principled BSDF node to get the color
        if mat.use_nodes and 'Principled BSDF' in mat.node_tree.nodes:
            color_node = mat.node_tree.nodes['Principled BSDF']
            context.scene.wot_tool_props.voxel_color = color_node.inputs['Base Color'].default_value
            return {'FINISHED'}
            
        return {'CANCELLED'}
