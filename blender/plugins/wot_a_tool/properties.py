import bpy

class WOT_VoxelToolProperties(bpy.types.PropertyGroup):
    voxel_size: bpy.props.FloatProperty(
        name="Voxel Size",
        description="The size of each voxel cube",
        default=1.0,
        min=0.01
    )
    
    voxel_color: bpy.props.FloatVectorProperty(
        name="Voxel Color",
        description="The color to apply to the next voxel",
        subtype='COLOR',
        default=(1.0, 1.0, 1.0, 1.0), # RGBA, default to white
        min=0.0,
        max=1.0,
        size=4
    ) 