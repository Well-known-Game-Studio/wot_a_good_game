def run():
    out={}
    for name,fn in KIT.items():
        try: out[name]=build_kit_mesh(name,fn())
        except Exception as e: out[name]="ERR "+str(e)[:160]
    # ground material variants
    F="/Game/Wot/Towns/Common/Materials"; G="/Game/Wot/Materials/MI_VoxelGrass.MI_VoxelGrass"
    for n,c in {"MI_Ground_Sand":(0.86,0.76,0.52),"MI_Ground_Dirt":(0.40,0.30,0.18),"MI_Ground_Ash":(0.33,0.31,0.30),"MI_Ground_Tundra":(0.58,0.66,0.44),"MI_Ground_Lush":(0.30,0.60,0.26)}.items():
        if not T(AS+"exists",{"path":f"{F}/{n}"}):
            T("editor_toolset.toolsets.material_instance.MaterialInstanceTools.create",{"folder_path":F,"asset_name":n,"parent":ref(G)})
        T("editor_toolset.toolsets.material_instance.MaterialInstanceTools.set_vector_parameter",{"instance":ref(f"{F}/{n}.{n}"),"name":"Color","value":{"r":c[0],"g":c[1],"b":c[2],"a":1}})
        out[n]="ok"
    found=T(AS+"find_assets",{"folder_path":"/Game/Wot/Towns/Common/Buildings","name":"","asset_type":"/Script/Engine.StaticMesh","recursive":True})
    paths=[(x if isinstance(x,str) else x["refPath"]) for x in found]
    info={}
    for p in paths:
        n=p.split("/")[-1]
        b=T("editor_toolset.toolsets.static_mesh.StaticMeshTools.get_bounds",{"mesh":ref(p+"."+n)}); mn=b["min"]; mx=b["max"]
        info[n]={"size":[round(mx["x"]-mn["x"]),round(mx["y"]-mn["y"]),round(mx["z"]-mn["z"])],"zmin":round(mn["z"]),"slots":T("editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots",{"mesh":ref(p+"."+n)})}
    save(paths+[f"{F}/{n}" for n in ("MI_Ground_Sand","MI_Ground_Dirt","MI_Ground_Ash","MI_Ground_Tundra","MI_Ground_Lush")])
    return {"built":out,"info":info}
