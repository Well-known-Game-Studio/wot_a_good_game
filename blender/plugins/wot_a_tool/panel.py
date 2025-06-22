import bpy

class WOT_PT_VoxelToolPanel(bpy.types.Panel):
    """Creates the settings panel for the Voxel Brush tool"""
    bl_label = "Voxel Brush Settings"
    bl_idname = "WOT_PT_VoxelToolPanel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Tool'
    bl_context = "objectmode"

    @classmethod
    def draw_settings_panel(cls, layout, context):
        props = context.scene.wot_tool_props

        box = layout.box()
        box.label(text="Brush Settings")
        box.prop(props, "voxel_size")
        box.prop(props, "voxel_color")
        
        # --- Voxel Color Palette ---
        palette_box = layout.box()
        palette_box.label(text="Color Palette")
        
        col = palette_box.column()
        row = col.row(align=True)

        # Find all voxel materials in the project
        voxel_materials = [m for m in bpy.data.materials if m.name.startswith("VoxelColor_")]
        
        for i, mat in enumerate(voxel_materials):
            if i > 0 and i % 8 == 0:
                row = col.row(align=True)
            
            op = row.operator("wot.select_voxel_color", text="", icon_value=mat.preview.icon_id)
            op.material_name = mat.name

    def draw(self, context):
        # This draw function is now only a fallback
        # The main UI is drawn by draw_settings_panel
        layout = self.layout
        self.draw_settings_panel(layout, context)