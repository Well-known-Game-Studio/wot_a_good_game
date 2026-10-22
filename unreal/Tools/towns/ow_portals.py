# Places (or moves) the Overworld portals / player starts / landmark meshes at the positions in ow_sites.json (see ow_fit.py).
SITES=json.loads(r"""__SITES__""")   # JSON (null is not Python)
LANDMARK_MESH=M+"small/small_prop_big_building.small_prop_big_building"
USER_LANDMARKS={"TarValon":"small_white_tower","Tear":"small_stone_of_tear"}
def run():
    out={}
    cur=T(SC+"get_current_level",{}); ow="/Game/Wot/Maps/Overworld/Overworld"
    if cur!=ow: save([cur]); T(SC+"load_level",{"level_path":ow})
    for town,s in SITES.items():
        x,y,yaw=s["x"],s["y"],s["yaw"]; dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw)); has_level=s.get("level",True)
        ps=find_by_label(f"{town}Portal")
        if ps: p=ps[0]; move_actor(p,(x,y,100),yaw); out[town]="moved"
        else:
            p=place_mesh("/Game/Wot/Actors/Portal",f"{town}Portal",(x,y,100),yaw,0.5,None,tag="OverworldPortal"); out[town]="placed"
        # a city without a level yet keeps a disabled portal as its marker (ToLevel empty) until its level exists
        T(OB+"set_properties",{"instance":ref(p),"values":json.dumps({"ToLevel":town if has_level else "","PortalName":f"Overworld{town}","Enabled":bool(has_level)})})
        st=find_by_label(f"{town}PlayerStart")
        if has_level:
            if st: move_actor(st[0],(x+dx*420,y+dy*420,100),yaw)
            else:
                a=place_class("/Script/Engine.PlayerStart",f"{town}PlayerStart",(x+dx*420,y+dy*420,100),yaw,1.0,tag="OverworldPortal")
                T(OB+"set_properties",{"instance":ref(a),"values":json.dumps({"PlayerStartTag":f"{town}Overworld"})})
        else:
            for a in st: remove(a)
        lm=find_by_label(f"{town}Landmark")
        if s.get("cluster"):
            # the hand-placed settlement stands here: the generic landmark building is not wanted
            for p in lm: remove(p)
        elif lm: move_actor(lm[0],(x-dx*220,y-dy*220,10),yaw+180)
        elif town not in USER_LANDMARKS: place_mesh(LANDMARK_MESH,f"{town}Landmark",(x-dx*220,y-dy*220,10),yaw+180,1.2,None,tag="OverworldPortal")
    for town,lab in USER_LANDMARKS.items():
        s=SITES[town]; dx=math.cos(math.radians(s["yaw"])); dy=math.sin(math.radians(s["yaw"]))
        for p in find_by_label(lab): move_actor(p,(s["x"]-dx*320,s["y"]-dy*320,10))
    s=SITES["TarValon"]
    for p in find_by_label("small_dragonmount"): move_actor(p,(s["x"]-2700,s["y"]+640,10))   # the mountain spur west of the city on the map texture
    save([ow]); out["dirty"]=T(AS+"is_dirty",{"asset_path":ow})
    return out
