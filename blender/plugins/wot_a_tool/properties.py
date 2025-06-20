import bpy

def update_wall_dimensions(self, context):
    """
    This function is called when wall_dimensions is changed in the UI.
    It updates the dimensions of the active 'InteractiveWall' object,
    while preserving the world-transform of its children (the cutouts).
    """
    if not (context.active_object and context.active_object.name.startswith("InteractiveWall")):
        return
        
    wall = context.active_object
    
    # This check prevents the function from running on the property itself at startup
    if hasattr(self, "wall_dimensions"):
        
        # Store children and their world matrices to preserve their transform
        children = list(wall.children)
        child_world_matrices = {child: child.matrix_world.copy() for child in children}

        # Unparent children temporarily
        for child in children:
            # Keep matrix in world space
            child.matrix_world = child.matrix_world
            child.parent = None

        # Resize wall and apply scale
        wall.dimensions = self.wall_dimensions
        with context.temp_override(object=wall, selected_objects=[wall]):
             bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

        # Re-parent children and restore their world transforms
        for child in children:
            child.parent = wall
            child.matrix_world = child_world_matrices[child]


class WOT_Properties(bpy.types.PropertyGroup):
    wall_dimensions: bpy.props.FloatVectorProperty(
        name="Dimensions",
        default=(4.0, 0.2, 3.0),
        subtype='XYZ',
        unit='LENGTH',
        description="Dimensions of the wall",
        update=update_wall_dimensions
    )
    wall_voxel_size: bpy.props.FloatProperty(
        name="Voxel Size",
        default=0.1,
        min=0.01,
        max=1.0,
        description="The size of the voxels"
    )

    # Door properties
    has_door: bpy.props.BoolProperty(
        name="Add Door",
        default=False,
        description="Add a door cutout to the wall"
    )
    door_dimensions: bpy.props.FloatVectorProperty(
        name="Door Dimensions",
        default=(0.9, 0.2, 2.0),
        subtype='XYZ',
        unit='LENGTH',
        description="Dimensions of the door"
    )
    door_position: bpy.props.FloatVectorProperty(
        name="Door Position",
        default=(0.0, 0.0, 1.0),
        subtype='XYZ',
        unit='LENGTH',
        description="Position of the door"
    )

    # Window properties
    has_window: bpy.props.BoolProperty(
        name="Add Window",
        default=False,
        description="Add a window cutout to the wall"
    )
    window_dimensions: bpy.props.FloatVectorProperty(
        name="Window Dimensions",
        default=(1.2, 0.2, 1.0),
        subtype='XYZ',
        unit='LENGTH',
        description="Dimensions of the window"
    )
    window_position: bpy.props.FloatVectorProperty(
        name="Window Position",
        default=(0.0, 0.0, 1.5),
        subtype='XYZ',
        unit='LENGTH',
        description="Position of the window"
    ) 