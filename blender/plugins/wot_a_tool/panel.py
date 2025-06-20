import bpy

class WOT_PT_MainPanel(bpy.types.Panel):
    """Creates a Panel in the 3D Viewport"""
    bl_label = "Wot A Tool"
    bl_idname = "WOT_PT_MainPanel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Wot A Tool'

    def draw(self, context):
        layout = self.layout
        props = context.scene.wot_tool_props

        box = layout.box()
        box.label(text="Wall Tool")
        box.prop(props, "wall_dimensions")
        box.prop(props, "wall_voxel_size")

        # Door settings
        box.prop(props, "has_door")
        if props.has_door:
            door_box = box.box()
            door_box.prop(props, "door_dimensions")
            door_box.prop(props, "door_position")

        # Window settings
        box.prop(props, "has_window")
        if props.has_window:
            window_box = box.box()
            window_box.prop(props, "window_dimensions")
            window_box.prop(props, "window_position")

        box.operator("wot.generate_wall", text="Generate Wall")

        # Add a new section for finalization
        finalize_box = layout.box()
        finalize_box.label(text="Finalize Tool")
        
        # Only show the finalize button if the active object has modifiers
        if context.active_object and len(context.active_object.modifiers) > 0:
            finalize_box.operator("wot.finalize_object", text="Finalize Selected")
        else:
            row = finalize_box.row()
            row.label(text="Select an object with modifiers")
            row.enabled = False