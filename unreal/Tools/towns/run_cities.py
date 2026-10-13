import time
def run():
    towns=TOWNS
    out={}
    for town in towns:
        t0=time.time()
        try:
            if town=="EmondsField":
                spec=ef_spec(); spec["town"]="EmondsField"; TERRAIN[town](spec)
                cur=T(SC+"get_current_level",{}); dst="/Game/Wot/Maps/EmondsField/EmondsField"
                if cur!=dst: save([cur]); T(SC+"load_level",{"level_path":dst})
                b=build_level(spec); save([dst]); r={"build":b}
            else:
                spec=CITY_SPECS[town]()
                if town in TERRAIN: TERRAIN[town](spec)
                r=make_city(spec,town,["Tam_NPC","Lan_NPC","Ground"])
            out[town]={"ok":True,"nodes":r["build"]["info"]["nodes"],"msgs":r["build"]["msgs"],"placed":r["build"]["placed"],"s":round(time.time()-t0)}
        except Exception as e:
            out[town]={"ok":False,"err":str(e)[:400],"s":round(time.time()-t0)}
    return out
