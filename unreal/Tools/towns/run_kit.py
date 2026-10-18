def run():
    out={}
    cur=T(SC+"get_current_level",{}); dst="/Game/Wot/Maps/EmondsField/EmondsField"
    if cur!=dst: save([cur]); T(SC+"load_level",{"level_path":dst})
    names=KIT_ONLY if KIT_ONLY else [f"SM_Stairs{r}" for r in (200,240,280,320,360,400,480,600)]+["SM_WhiteBridge"]
    for name in names:
        try: out[name]=build_kit_mesh(name,KIT[name]())
        except Exception as e: out[name]="ERR "+str(e)[:160]
    for old in ("SM_Stairs300",):
        if T(AS+"exists",{"path":f"{KIT_FOLDER}/{old}"}): T(AS+"delete",{"path":f"{KIT_FOLDER}/{old}"}); out[old]="deleted"
    # stairs collide with their real steps: no convex hull (it would be a wedge with a wall at the foot, and a merged mesh's
    # nav collision only ever exports that hull), complex-as-simple so both physics and the navmesh use the step triangles
    SMT="editor_toolset.toolsets.static_mesh.StaticMeshTools."
    for name in out:
        if name.startswith("SM_Stairs") and "ERR" not in str(out[name]):
            T(SMT+"remove_collisions",{"mesh":ref(f"{KIT_FOLDER}/{name}.{name}")})
            T(OB+"set_properties",{"instance":ref(f"{KIT_FOLDER}/{name}.{name}:BodySetup_0"),"values":json.dumps({"CollisionTraceFlag":"CTF_UseComplexAsSimple"})})
    save([f"{KIT_FOLDER}/{n}" for n in out if "ERR" not in str(out[n]) and out[n]!="deleted"])
    return out
