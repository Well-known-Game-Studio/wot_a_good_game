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
import sys
import importlib

# This is a more robust way to handle reloading the addon's modules
# during development, preventing the need to restart Blender for every change.
addon_package_name = __name__
if addon_package_name in sys.modules:
    # The order of reloading matters. Reload in order of dependency.
    modules_to_reload = [
        "properties",
        "operators",
        "panel"
    ]
    for module_name in modules_to_reload:
        full_module_path = f"{addon_package_name}.{module_name}"
        if full_module_path in sys.modules:
            importlib.reload(sys.modules[full_module_path])

# Import modules after they've been potentially reloaded
from . import panel
from . import operators
from . import properties

classes = [
    properties.WOT_Properties,
    panel.WOT_PT_MainPanel,
    operators.WOT_OT_GenerateWall,
    operators.WOT_OT_FinalizeObject,
]

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    # Register the property group after the classes that might use it
    bpy.types.Scene.wot_tool_props = bpy.props.PointerProperty(type=properties.WOT_Properties)


def unregister():
    # Unregister in the reverse order of registration
    del bpy.types.Scene.wot_tool_props
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)

if __name__ == "__main__":
    register() 