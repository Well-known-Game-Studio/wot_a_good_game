import bpy
from .panel import WOT_PT_VoxelToolPanel

class WOT_VoxelBrushTool(bpy.types.WorkSpaceTool):
    """The Voxel Brush tool that appears in the toolbar."""
    bl_space_type = 'VIEW_3D'
    bl_context_mode = 'OBJECT'
    bl_idname = "wot.voxel_brush_tool"
    bl_label = "Voxel Brush"
    bl_description = "Click to place colored voxels on the grid or on existing faces."
    bl_icon = "ops.mesh.cube_add"
    bl_widget = None # We use our own panel
    bl_keymap = (
        ("wot.voxel_brush_operator", {"type": 'LEFTMOUSE', "value": 'PRESS'}, None),
    )

    def draw_settings(context, layout, tool):
        WOT_PT_VoxelToolPanel.draw_settings_panel(layout, context)
