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
        
        # Only enable editing if an interactive wall is selected
        is_interactive_wall_selected = context.active_object is not None and context.active_object.name.startswith("InteractiveWall")
        
        wall_props_box = box.column()
        wall_props_box.enabled = is_interactive_wall_selected
        wall_props_box.prop(props, "wall_dimensions")
        wall_props_box.prop(props, "wall_voxel_size")

        # Door settings
        door_box = box.column()
        door_box.enabled = is_interactive_wall_selected
        door_box.prop(props, "has_door")
        if props.has_door:
            door_settings = door_box.box()
            door_settings.prop(props, "door_dimensions")
            door_settings.prop(props, "door_position")

        # Window settings
        window_box = box.column()
        window_box.enabled = is_interactive_wall_selected
        window_box.prop(props, "has_window")
        if props.has_window:
            window_settings = window_box.box()
            window_settings.prop(props, "window_dimensions")
            window_settings.prop(props, "window_position")

        box.operator("wot.generate_wall", text="Generate New Wall")

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