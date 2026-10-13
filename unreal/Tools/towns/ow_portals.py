# Places (or moves) the Overworld portals / player starts / landmark meshes at the positions in ow_sites.json (see ow_fit.py).
SITES=__SITES__
LANDMARK_MESH=M+"small/small_prop_big_building.small_prop_big_building"
USER_LANDMARKS={"TarValon":"small_white_tower","Tear":"small_stone_of_tear"}
def run():
    out={}
    cur=T(SC+"get_current_level",{}); ow="/Game/Wot/Maps/Overworld/Overworld"
    if cur!=ow: save([cur]); T(SC+"load_level",{"level_path":ow})
    for town,s in SITES.items():
        x,y,yaw=s["x"],s["y"],s["yaw"]; dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw))
        ps=find_by_label(f"{town}Portal")
        if ps: move_actor(ps[0],(x,y,100),yaw); out[town]="moved"
        else:
            p=place_mesh("/Game/Wot/Actors/Portal",f"{town}Portal",(x,y,100),yaw,0.5,None,tag="OverworldPortal")
            T(OB+"set_properties",{"instance":ref(p),"values":json.dumps({"ToLevel":town,"PortalName":f"Overworld{town}","Enabled":True})}); out[town]="placed"
        st=find_by_label(f"{town}PlayerStart")
        if st: move_actor(st[0],(x+dx*80,y+dy*80,100),yaw)
        else:
            a=place_class("/Script/Engine.PlayerStart",f"{town}PlayerStart",(x+dx*80,y+dy*80,100),yaw,1.0,tag="OverworldPortal")
            T(OB+"set_properties",{"instance":ref(a),"values":json.dumps({"PlayerStartTag":f"{town}Overworld"})})
        lm=find_by_label(f"{town}Landmark")
        if lm: move_actor(lm[0],(x-dx*220,y-dy*220,10),yaw+180)
        elif town not in USER_LANDMARKS: place_mesh(LANDMARK_MESH,f"{town}Landmark",(x-dx*220,y-dy*220,10),yaw+180,1.2,None,tag="OverworldPortal")
    for town,lab in USER_LANDMARKS.items():
        s=SITES[town]; dx=math.cos(math.radians(s["yaw"])); dy=math.sin(math.radians(s["yaw"]))
        for p in find_by_label(lab): move_actor(p,(s["x"]-dx*320,s["y"]-dy*320,10))
    s=SITES["TarValon"]
    for p in find_by_label("small_dragonmount"): move_actor(p,(s["x"]-900,s["y"]+300,10))
    save([ow]); out["dirty"]=T(AS+"is_dirty",{"asset_path":ow})
    return out
