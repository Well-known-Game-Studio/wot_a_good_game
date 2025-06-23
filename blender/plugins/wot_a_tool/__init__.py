bl_info = {
    "name": "Wot A Tool - Voxel Brush",
    "author": "Gemini",
    "version": (1, 0),
    "blender": (2, 93, 0), # Increased version for modal tools
    "location": "View3D > Toolbar > Voxel Brush",
    "description": "An interactive tool to place and color voxels.",
    "warning": "",
    "doc_url": "",
    "category": "Development",
}

import bpy
import sys
import importlib

# Robust module reloading
addon_package_name = __name__
if addon_package_name in sys.modules:
    modules_to_reload = [
        "properties",
        "operators",
        "panel",
        "tool"
    ]
    for module_name in modules_to_reload:
        full_module_path = f"{addon_package_name}.{module_name}"
        if full_module_path in sys.modules:
            importlib.reload(sys.modules[full_module_path])

# Import modules after they've been potentially reloaded
from . import panel
from . import operators
from . import properties
from . import tool

# Explicitly order the classes for registration
classes_to_register = (
    properties.WOT_VoxelToolProperties,
    operators.WOT_OT_SelectVoxelColor,
    operators.WOT_OT_VoxelBrush,
    panel.WOT_PT_VoxelToolPanel,
    tool.WOT_VoxelBrushTool,
)

def register():
    # Define what to register
    property_class = properties.WOT_VoxelToolProperties
    tool_class = tool.WOT_VoxelBrushTool
    other_classes = (
        operators.WOT_OT_SelectVoxelColor,
        operators.WOT_OT_VoxelBrush,
        panel.WOT_PT_VoxelToolPanel,
    )

    # 1. Register properties
    bpy.utils.register_class(property_class)
    bpy.types.Scene.wot_tool_props = bpy.props.PointerProperty(type=property_class)

    # 2. Register operators and panels
    for cls in other_classes:
        bpy.utils.register_class(cls)

    # 3. Register the tool
    bpy.utils.register_tool(tool_class, separator=True, group=True)

def unregister():
    # Define what to unregister (in reverse order)
    property_class = properties.WOT_VoxelToolProperties
    tool_class = tool.WOT_VoxelBrushTool
    other_classes = (
        operators.WOT_OT_SelectVoxelColor,
        operators.WOT_OT_VoxelBrush,
        panel.WOT_PT_VoxelToolPanel,
    )

    # 1. Unregister the tool
    bpy.utils.unregister_tool(tool_class)

    # 2. Unregister operators and panels
    for cls in reversed(other_classes):
        bpy.utils.unregister_class(cls)

    # 3. Unregister properties
    del bpy.types.Scene.wot_tool_props
    bpy.utils.unregister_class(property_class)

if __name__ == "__main__":
    register() 