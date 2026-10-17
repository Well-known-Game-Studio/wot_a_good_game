def make_city(spec,folder_name,extra_clean):
    out={}
    src="/Game/Wot/Maps/EmondsField/EmondsField"; dst=f"/Game/Wot/Maps/{folder_name}/{folder_name}"
    if not T(AS+"exists",{"path":dst}):
        T(AS+"create_folder",{"path":f"/Game/Wot/Maps/{folder_name}"})
        out["dup"]=T(AS+"duplicate",{"path":src,"new_path":dst}); save([dst])
    cur=T(SC+"get_current_level",{})
    if cur!=dst: save([cur]); T(SC+"load_level",{"level_path":dst})
    out["build"]=build_level(spec,extra_clean=extra_clean)
    save([dst]); out["level"]=dst
    return out
def run():
    out={}
    for town in TOWNS:
        t0=time.time()
        try:
            spec=CITY_SPECS[town]()
            if town=="EmondsField":
                cur=T(SC+"get_current_level",{}); dst="/Game/Wot/Maps/EmondsField/EmondsField"
                if cur!=dst: save([cur]); T(SC+"load_level",{"level_path":dst})
                b=build_level(spec,extra_clean=["TheGreen_OldTree","VillageWell","WinespringInn","Campfire","Chest_Inn","Chest_West","Crate1","Crate2","SheepPen","Sheep0","Sheep1","Sheep2","InnHorse","Fireflies0","Fireflies1","Fireflies2"]); save([dst]); r={"build":b}
            else:
                r=make_city(spec,town,["Tam_NPC","Lan_NPC","Ground"])
            out[town]={"ok":True,"nodes":r["build"]["info"]["nodes"],"msgs":str(r["build"]["msgs"])[:300],"placed":r["build"]["placed"],"boxes":len(spec["terrain"]["boxes"]),"lattice":len(spec["lattice"]),"s":round(time.time()-t0)}
        except Exception as e:
            # the level may be half rebuilt (old actors removed, new ones missing): leave it open and unsaved,
            # and stop so the next town's level switch cannot save it
            out[town]={"ok":False,"err":str(e)[:600],"s":round(time.time()-t0)}
            break
    return out
