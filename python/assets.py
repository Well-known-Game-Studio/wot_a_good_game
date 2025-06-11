import bpy

def copy_asset(parent_obj, new_location):
    # Deselect all
    bpy.ops.object.select_all(action='DESELECT')
    # Select parent and all children
    parent_obj.select_set(True)
    for child in parent_obj.children:
        child.select_set(True)
    bpy.context.view_layer.objects.active = parent_obj
    # Duplicate
    bpy.ops.object.duplicate()
    # Move the new parent to the new location
    new_parent = bpy.context.selected_objects[0]
    new_parent.location = new_location
    # Deselect all
    bpy.ops.object.select_all(action='DESELECT')
    return new_parent
