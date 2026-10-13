def make_city(spec,folder_name,extra_clean):
    out={}
    src="/Game/Wot/Maps/EmondsField/EmondsField"; dst=f"/Game/Wot/Maps/{folder_name}/{folder_name}"
    if not T(AS+"exists",{"path":dst}):
        T(AS+"create_folder",{"path":f"/Game/Wot/Maps/{folder_name}"})
        out["dup"]=T(AS+"duplicate",{"path":src,"new_path":dst}); save([dst])
    cur=T(SC+"get_current_level",{})
    if cur!=dst:
        save([cur]); T(SC+"load_level",{"level_path":dst})
    out["build"]=build_level(spec,extra_clean=extra_clean)
    save([dst]); out["level"]=dst
    return out
