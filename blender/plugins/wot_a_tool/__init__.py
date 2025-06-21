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

classes = [
    properties.WOT_VoxelToolProperties,
    panel.WOT_PT_VoxelToolPanel,
    operators.WOT_OT_VoxelBrush,
]

def register():
    # Register properties and operators first
    for cls in classes:
        bpy.utils.register_class(cls)
        
    bpy.types.Scene.wot_tool_props = bpy.props.PointerProperty(type=properties.WOT_VoxelToolProperties)

    # Then, register the tool itself
    tool.register()

def unregister():
    # Unregister in the reverse order of registration
    tool.unregister()
    
    del bpy.types.Scene.wot_tool_props
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register() 