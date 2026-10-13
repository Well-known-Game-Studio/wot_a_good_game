SITES=[ # (town map name, x, y, facing yaw, landmark mesh, landmark scale, landmark mats)
 ("Baerlon",1500,-1800,-90,M+"small/small_prop_house.small_prop_house",1.0,None),
 ("Whitebridge",-500,3200,90,M+"small/small_prop_big_building.small_prop_big_building",1.0,None),
 ("ShadarLogoth",2600,2600,180,M+"small/small_prop_big_building.small_prop_big_building",1.0,[PM("P_DarkGrey"),PM("P_Black"),PM("P_DarkGrey"),PM("P_DarkGrey"),PM("P_DarkGrey")]),
 ("Caemlyn",1200,6800,90,M+"small/small_prop_big_building.small_prop_big_building",1.3,None),
 ("Cairhien",6000,8300,90,M+"small/small_prop_big_building.small_prop_big_building",1.3,None),
 ("TarValon",9100,-5900,-90,None,1.0,None),
 ("FalDara",14300,-1200,0,M+"small/small_prop_big_building.small_prop_big_building",1.2,None),
 ("Tear",11200,8000,90,None,1.0,None),
 ("Illian",-4800,2800,180,M+"small/small_prop_big_building.small_prop_big_building",1.2,None),
 ("Tanchico",-2500,-7200,-90,M+"small/small_prop_big_building.small_prop_big_building",1.2,None),
]
def run():
    out=[]
    for (town,x,y,yaw,lm,ls,lmats) in SITES:
        # skip if already present
        if find_by_label(f"{town}Portal"): continue
        p=place_mesh("/Game/Wot/Actors/Portal",f"{town}Portal",(x,y,100),yaw,0.5,None,tag="OverworldPortal")
        T(OB+"set_properties",{"instance":ref(p),"values":json.dumps({"ToLevel":town,"PortalName":f"Overworld{town}","Enabled":True})})
        dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw))
        ps=place_class("/Script/Engine.PlayerStart",f"{town}PlayerStart",(x+dx*80,y+dy*80,100),yaw,1.0,tag="OverworldPortal")
        T(OB+"set_properties",{"instance":ref(ps),"values":json.dumps({"PlayerStartTag":f"{town}Overworld"})})
        if lm: place_mesh(lm,f"{town}Landmark",(x-dx*220,y-dy*220,10),yaw+180,ls,lmats,tag="OverworldPortal")
        out.append(town)
    # verify a couple of properties
    chk={}
    for lab in ("CaemlynPortal","CaemlynPlayerStart"):
        for a in find_by_label(lab): chk[lab]=T(OB+"get_properties",{"instance":ref(a),"properties":["ToLevel","PortalName","PlayerStartTag"]})
    save(["/Game/Wot/Maps/Overworld/Overworld"])
    return {"placed":out,"check":chk}
