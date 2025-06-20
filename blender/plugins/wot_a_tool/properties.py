import bpy

class WOT_Properties(bpy.types.PropertyGroup):
    wall_dimensions: bpy.props.FloatVectorProperty(
        name="Dimensions",
        default=(4.0, 0.2, 3.0),
        subtype='XYZ',
        unit='LENGTH',
        description="Dimensions of the wall"
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