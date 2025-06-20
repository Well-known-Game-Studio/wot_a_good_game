bl_info = {
    "name": "Wot A Tool",
    "author": "Gemini",
    "version": (1, 0),
    "blender": (2, 80, 0),
    "location": "View3D > Sidebar > Wot A Tool",
    "description": "A set of tools for generating voxelized meshes.",
    "warning": "",
    "doc_url": "",
    "category": "Development",
}

import bpy
import importlib

from . import panel
from . import operators
from . import properties

# When developing, this allows you to reload the addon scripts in Blender
# without having to restart Blender.
if "bpy" in locals():
    importlib.reload(panel)
    importlib.reload(operators)
    importlib.reload(properties)

classes = [
    properties.WOT_Properties,
    panel.WOT_PT_MainPanel,
    operators.WOT_OT_GenerateWall,
    operators.WOT_OT_FinalizeObject,
]

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.wot_tool_props = bpy.props.PointerProperty(type=properties.WOT_Properties)

def unregister():
    del bpy.types.Scene.wot_tool_props
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register() 