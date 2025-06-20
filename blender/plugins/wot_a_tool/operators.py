import bpy

def add_boolean_object(context, dimensions, position, name):
    """Helper function to create a boolean object"""
    bpy.ops.mesh.primitive_cube_add(size=1, enter_editmode=False, align='WORLD', location=position)
    obj = context.active_object
    obj.name = name
    obj.dimensions = dimensions
    obj.display_type = 'WIRE'
    obj.hide_render = True # Don't render the cutters
    return obj

class WOT_OT_GenerateWall(bpy.types.Operator):
    """Generate a Voxelized Wall"""
    bl_idname = "wot.generate_wall"
    bl_label = "Generate Wall"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        props = context.scene.wot_tool_props
        cursor_location = context.scene.cursor.location

        # Create the main wall object
        bpy.ops.mesh.primitive_cube_add(size=1, enter_editmode=False, align='WORLD', location=cursor_location)
        wall = context.active_object
        wall.name = "InteractiveWall"
        wall.dimensions = props.wall_dimensions

        # Create door cutout object if needed
        if props.has_door:
            door_pos = cursor_location + props.door_position
            door = add_boolean_object(context, props.door_dimensions, door_pos, "DoorCutout")
            door.parent = wall # Parent to wall
            
            bool_mod = wall.modifiers.new(name="DoorBoolean", type='BOOLEAN')
            bool_mod.object = door
            bool_mod.operation = 'DIFFERENCE'

        # Create window cutout object if needed
        if props.has_window:
            window_pos = cursor_location + props.window_position
            window = add_boolean_object(context, props.window_dimensions, window_pos, "WindowCutout")
            window.parent = wall # Parent to wall

            bool_mod = wall.modifiers.new(name="WindowBoolean", type='BOOLEAN')
            bool_mod.object = window
            bool_mod.operation = 'DIFFERENCE'

        # Add the remesh modifier but don't apply it
        remesh = wall.modifiers.new(name="Remesh", type='REMESH')
        remesh.mode = 'BLOCKS'
        remesh.voxel_size = props.wall_voxel_size

        self.report({'INFO'}, "Generated interactive wall. Move cutters to adjust.")
        return {'FINISHED'}

class WOT_OT_FinalizeObject(bpy.types.Operator):
    """Applies all modifiers and removes helper objects"""
    bl_idname = "wot.finalize_object"
    bl_label = "Finalize Object"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        # Only work if there is an active object
        return context.active_object is not None

    def execute(self, context):
        obj_to_finalize = context.active_object

        # Store children to be deleted
        children_to_delete = [child for child in obj_to_finalize.children]

        # Apply modifiers
        depsgraph = context.evaluated_depsgraph_get()
        evaluated_obj = obj_to_finalize.evaluated_get(depsgraph)
        final_mesh = bpy.data.meshes.new_from_object(evaluated_obj)
        
        # Clear modifiers and assign new mesh data
        obj_to_finalize.modifiers.clear()
        obj_to_finalize.data = final_mesh

        # Delete children
        for child in children_to_delete:
            bpy.data.objects.remove(child, do_unlink=True)
            
        self.report({'INFO'}, f"Finalized {obj_to_finalize.name}")
        return {'FINISHED'} 