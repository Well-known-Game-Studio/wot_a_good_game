# ---------- terrain, water, stairs/ladders, NPCs, dialogue added on top of the city specs ----------
LADDER_BP="/Game/Wot/GeometryScripting/BP_Ladder"; POTION="/Game/Wot/Actors/HealthPotion"
WF_TOP="/Game/Wot/Effects/NS_WaterfallTop"; WF_FOAM="/Game/Wot/Effects/NS_WaterfallFoam"
def t_rect(label,x0,y0,x1,y1,z,mat,base=-300):
    h=z-base; return ("ground",CUBE,label,((x0+x1)/2,(y0+y1)/2,base+h/2),0,((x1-x0)/100.0,(y1-y0)/100.0,h/100.0),[mat])
def t_circle(label,cx,cy,r,z,mat,base=-300):
    h=z-base; return ("ground",CYL,label,(cx,cy,base+h/2),0,(2*r/100.0,2*r/100.0,h/100.0),[mat])
def water_plane(label,cx,cy,size,z=-150):
    return ("mesh",CUBE,label,(cx,cy,z-1),0,(size/100.0,size/100.0,0.02),[WATER])
def stair_prop(label,x,y,yaw,rise,z=0,mats=None):
    """Stairs whose foot is at (x,y,z) climbing in direction yaw by rise."""
    return ("mesh",kit_mesh(f"SM_Stairs{rise}"),label,(x,y,z),yaw,1.0,mats)
def stairs_circle(prefix,cx,cy,r,rise,angles,z=0,mats=None):
    """Stairs up a circular terrace of radius r at the given angles (deg), foot outside climbing inward."""
    out=[]
    for k,a in enumerate(angles):
        th=math.radians(a); fx=cx+(r+2*rise+20)*math.cos(th); fy=cy+(r+2*rise+20)*math.sin(th)
        out.append(stair_prop(f"{prefix}{k}",fx,fy,a+180,rise,z,mats))
    return out
def ladder(label,x,y,yaw,h,z=0):
    return ("actor",LADDER_BP,label,(x,y,z),yaw,1.0,{"LadderSize":{"x":0,"y":0,"z":h}})
def ring_excl(cx,cy,r,n=24,w=420):
    return [(cx+r*math.cos(2*math.pi*k/n),cy+r*math.sin(2*math.pi*k/n),w,w) for k in range(n)]
def rect_excl(x0,y0,x1,y1,w=380):
    return [((x0+x1)/2,y0,(x1-x0)/2+w,w),((x0+x1)/2,y1,(x1-x0)/2+w,w),(x0,(y0+y1)/2,w,(y1-y0)/2+w),(x1,(y0+y1)/2,w,(y1-y0)/2+w)]
def npc(label,name,row,x,y,yaw=0,radius=900):
    return ("npc",label,(x,y),yaw,{"Name":name,"CurrentRowName":row,"WanderRadius":float(radius)})
def lines(prefix,texts):
    d={}
    for i,t in enumerate(texts):
        d[f"{prefix}_{i+1}"]=(t, f"{prefix}_{i+2}" if i+1<len(texts) else "")
    return d
def potion(label,x,y,z=0): return ("mesh",POTION,label,(x,y,z),0,1.0,None)
def chest(label,x,y,z=0,yaw=0): return ("mesh",CHEST,label,(x,y,z),yaw,1.0,None)
def waterfall(label,x,y,yaw,z_top,z_bottom):
    return [("mesh",WF_TOP,label+"_top",(x,y,z_top),yaw,1.0,None),("mesh",WF_FOAM,label+"_foam",(x,y,z_bottom+20),yaw,1.0,None)]
def no_ground(spec,names):
    for r in spec["roads"]:
        if gv(r,"name","") in names: r["no_ground"]=True


# ---- deterministic terrain heights ----
def _in_circle(x,y,c): return (x-c[0])**2+(y-c[1])**2<=c[2]**2
def _in_rect(x,y,r): return r[0]<=x<=r[2] and r[1]<=y<=r[3]
def terrain_height(s,x,y):
    z=gv(s,"base_z",0)
    for tr in gv(s,"terraces") or []:
        sh=tr["shape"]
        inside=_in_circle(x,y,sh[1]) if sh[0]=="circle" else _in_rect(x,y,sh[1])
        if inside and tr["z"]>z: z=tr["z"]
    return z
def add_terrace(s,kind,shape,z,n=32):
    """Registers a terrace for height lookups and foliage lattice. shape: circle (cx,cy,r) or rect (x0,y0,x1,y1)."""
    s.setdefault("terraces",[])
    if kind=="circle": pts=circle(shape[0],shape[1],shape[2],n)
    else: pts=rect(*shape)
    s["terraces"].append({"pts":pts,"z":z,"linear":kind=="rect","shape":(kind,shape)})
def lift_roads(s,skip=()):
    """Gives every road point a height from the terraces; inserts vertical jumps where a road crosses a terrace edge."""
    for rd in s["roads"]:
        if gv(rd,"name","") in skip or gv(rd,"flat",False): continue
        pts=rd["pts"]; out=[]
        closed=gv(rd,"closed",False)
        seq=list(pts)+([pts[0]] if closed else [])
        for i in range(len(seq)):
            x,y=seq[i][0],seq[i][1]; z=terrain_height(s,x,y)+(seq[i][2] if len(seq[i])>2 else 0)
            if i>0:
                px,py=seq[i-1][0],seq[i-1][1]; L=math.hypot(x-px,y-py); n=max(2,int(L/60))
                prev_z=terrain_height(s,px,py)
                for k in range(1,n):
                    sx=px+(x-px)*k/n; sy=py+(y-py)*k/n; sz=terrain_height(s,sx,sy)
                    if sz!=prev_z:
                        bx=px+(x-px)*(k-1)/n; by=py+(y-py)*(k-1)/n
                        out.append((bx,by,prev_z)); out.append((sx,sy,sz)); prev_z=sz
            if not (closed and i==len(seq)-1): out.append((x,y,z))
        rd["pts"]=out; rd["linear"]=True
def lift_plazas(s):
    for pz in gv(s,"plazas") or []:
        if "z" in pz and pz["z"]<0:   # water pools etc: offset relative to the terrace under the first point
            x,y=pz["pts"][0][0],pz["pts"][0][1]; pz["z"]=terrain_height(s,x,y)+pz["z"]
        elif "z" not in pz:
            x,y=pz["pts"][0][0],pz["pts"][0][1]; pz["z"]=terrain_height(s,x,y)
def lift_house_pts(s):
    if gv(s,"house_pts"):
        s["house_pts"]=[(x,y,yaw) for (x,y,yaw) in s["house_pts"]]

# ---------------------------------------------------------------------------------------------
def terrain_EmondsField(s):
    g=GRASSMAT
    # the Winespring: a rocky knoll with the spring pool on top, waterfall feeding the brook
    s["props"]+=[t_circle("SpringKnoll",3100,2500,1150,300,MAT("MI_Wall_Stone"),base=-300),
                 t_circle("SpringKnollTop",3100,2500,1100,320,g,base=250)]
    s["plazas"].append({"pts":circle(3100,2500,520,14),"mat":WATER,"z":-2})
    s["props"]+=waterfall("Winespring",3100,1330,-90,330,0)
    s["props"]+=stairs_circle("SpringStairs",3100,2500,1150,300,[200])
    s["exclusions"]+=ring_excl(3100,2500,1150,16,380)+[(3100,2500,700,700)]
    s["props"]+=[chest("SpringChest",3300,2800,330,40),potion("Potion_Spring",2800,2300,330),potion("Potion_Green",-600,900,0)]
    s["props"]+=[npc("Villager_Cenn","Cenn Buie",  "EF_Cenn",     -1600, 900, 0,1000),
                 npc("Villager_Bran","Bran al'Vere","EF_Bran",     1300,1000,-120,700),
                 npc("Villager_Egwene","Egwene",   "EF_Egwene",    400,-1700, 90,1200),
                 npc("Villager_Mat","Mat Cauthon",  "EF_Mat",     -2200,-2200, 30,1500)]
    s["dialogue"]={**lines("EF_Cenn",["Thatching, thatching, always thatching. The Winespring Inn wants a tile roof, can you believe it?","Mark my words, lad, those Coplin boys will be trouble before Bel Tine."]),
                   **lines("EF_Bran",["Welcome to the Winespring Inn! Finest ale this side of the Mountains of Mist.","The spring up on the knoll never runs dry. Sweetest water in the Two Rivers."]),
                   **lines("EF_Egwene",["Rand! Have you seen Nynaeve? She's looking for anyone who skipped their chores.","I'm going to see the world one day. Caemlyn, Tar Valon... all of it."]),
                   **lines("EF_Mat",["Psst. Want to see something? I found a badger down by the Old Road.","Don't tell Nynaeve. She'd have my hide with that switch of hers."])}
    add_terrace(s,"circle",(3100,2500,1150),320)
    lift_roads(s); lift_plazas(s)

def terrain_Caemlyn(s):
    g=MAT("MI_Ground_Lush"); st=MAT("MI_Tile_WhiteStone")
    s["props"]+=[t_circle("InnerCityHill",0,0,4400,400,g),t_circle("InnerCityWall",0,0,4400,120,MAT("MI_Wall_Stone"),base=-300),
                 t_circle("PalaceHill",0,0,2350,700,st,base=350)]
    s["props"]+=stairs_circle("InnerStairs",0,0,4400,400,[k*30 for k in range(12)],0,kit_mats("SM_Stairs400",{"P_LightGrey":"P_White"}))
    s["props"]+=stairs_circle("PalaceStairs",0,0,2350,300,[0,90,180,270],400,kit_mats("SM_Stairs300",{"P_LightGrey":"P_White"}))
    s["exclusions"]+=ring_excl(0,0,4400,36,420)+ring_excl(0,0,2350,24,380)
    for p in s["props"]:
        if p[0]=="mesh" and p[2] in ("RoyalPalace","SunSpire_N","SunSpire_S","PalaceFountain"):
            i=s["props"].index(p); l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],700),p[4],p[5],p[6])
    s["props"]+=[chest("PalaceChest",-1500,1200,700,90),potion("Potion_Palace",1300,-900,700),potion("Potion_Market",8000,-1200,0),chest("HillChest",3600,-2500,400,0)]
    s["props"]+=[npc("Guard_Tallanvor","Guardsman Tallanvor","CM_Guard",600,-1200,90,600),
                 npc("Merchant_Gill","Master Gill","CM_Gill",8300,-1500,-90,800),
                 npc("Lady_Elayne","Elayne","CM_Elayne",-900,1500,-45,700),
                 npc("Boy_Bayle","Bayle Domon","CM_Domon",-8200,3500,0,1200),
                 npc("Wanderer_Thom","Thom Merrilin","CM_Thom",5000,5200,0,1500)]
    s["dialogue"]={**lines("CM_Guard",["Halt! State your business in the Inner City.","...Very well. Mind the Queen's Guards, and keep off the palace steps."]),
                   **lines("CM_Gill",["The Queen's Blessing has the best room in the New City, if you ask me.","Trouble in the streets lately. Red cloths, white cloths... stay out of it."]),
                   **lines("CM_Elayne",["Mother says a Daughter-Heir should not talk to strangers in the gardens.","But you look like you have seen more of the world than I have. Tell me of it sometime."]),
                   **lines("CM_Domon",["I do be a river man, not a city man. Caemlyn be too far from the water.","If you ever need passage down the Erinin, look for the Spray in Whitebridge."]),
                   **lines("CM_Thom",["A gleeman's tales are worth a coin or two, young friend.","Did you know the topless towers of Cairhien were never finished? Now there is a story."])}
    add_terrace(s,"circle",(0,0,4400),400,40); add_terrace(s,"circle",(0,0,2350),700,28)
    lift_roads(s); lift_plazas(s)

def terrain_Cairhien(s):
    st=MAT("MI_Tile_WhiteStone")
    s["props"]+=[t_rect("Acropolis",-2700,-2700,2700,2700,300,st)]
    s["props"]+=[stair_prop("AcroStairE",2700+620,0,180,300,0),stair_prop("AcroStairW",-2700-620,0,0,300,0),stair_prop("AcroStairN",0,2700+620,-90,300,0),stair_prop("AcroStairS",0,-2700-620,90,300,0)]
    for k in range(4):
        a=math.radians(45+90*k); cx,cy=math.cos(a)*3400,math.sin(a)*3400
        s["props"].append(t_circle(f"TowerPlinth{k}",cx,cy,700,600,MAT("MI_Wall_Stone")))
        s["props"].append(ladder(f"TowerLadder{k}",cx-700*math.cos(a)-40,cy-700*math.sin(a)-40,math.degrees(a)+180,600))
    s["exclusions"]+=rect_excl(-2700,-2700,2700,2700,380)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2] in ("SunPalace","SunFountain"): l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],300),p[4],p[5],p[6])
        if p[0]=="mesh" and p[2].startswith("ToplessTower"): l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],600),p[4],p[5],p[6])
    s["props"]+=[chest("TowerChest0",math.cos(math.radians(45))*3400,math.sin(math.radians(45))*3400,600,0),potion("Potion_Acro",1800,1800,300),potion("Potion_Market",6500,6900,0)]
    s["props"]+=[npc("Noble_Barthanes","Lord Barthanes","CH_Barthanes",1200,-1200,135,500),npc("Beggar_Aldrin","Aldrin","CH_Beggar",-6000,2100,0,900),
                 npc("Clerk_Meilan","Meilan","CH_Clerk",2100,2100,-135,600),npc("Guard_Dalresin","Tower Guard","CH_Guard",6400,6400,-90,500)]
    s["dialogue"]={**lines("CH_Barthanes",["Daes Dae'mar, the Game of Houses. Every word you say here has three meanings.","Choose your friends in Cairhien carefully, outlander. Choose your enemies more carefully still."]),
                   **lines("CH_Beggar",["Spare a coin? The Foregate burned and the nobles never noticed.","They say the King hoards grain in the Sun Palace. I say the towers are hollow."]),
                   **lines("CH_Clerk",["The grid of the city was laid out by Ogier masons. Every street exactly so.","Please do not walk on the grass. It was imported from Andor at great expense."]),
                   **lines("CH_Guard",["The topless towers have stood half-built since the Aiel War.","Climb the ladders if you like. Mind the drop."])}
    add_terrace(s,"rect",(-2700,-2700,2700,2700),300)
    for k in range(4):
        a=math.radians(45+90*k); add_terrace(s,"circle",(math.cos(a)*3400,math.sin(a)*3400,700),600,16)
    lift_roads(s); lift_plazas(s)

def terrain_Tear(s):
    g=MAT("MI_Ground_Lush")
    s["props"]+=[t_rect("StoneRock",-5300,-2700,-200,2700,600,MAT("MI_Wall_DarkStone"))]
    s["props"]+=[stair_prop("StoneStairs",-200+1220,0,180,600,0,kit_mats("SM_Stairs600",{"P_LightGrey":"P_DarkGrey"})),ladder("StoneLadderN",-2700,2740,90,600),ladder("StoneLadderS",-2700,-2740,-90,600)]
    s["exclusions"]+=rect_excl(-5300,-2700,-200,2700,400)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2] in ("StoneOfTear","StoneGate"): l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],600),p[4],p[5],p[6])
        if p[0]=="mesh" and p[2]=="StoneKeep": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],3500),p[4],p[5],p[6])
    s["props"]+=[chest("StoneChest",-4800,2200,600,0),potion("Potion_Stone",-1000,-2000,600),potion("Potion_Docks",4800,3000,0),chest("DockChest",6900,4400,10,90)]
    no_ground(s,[f"Pier {k}" for k in range(6)])
    s["props"]+=[npc("Defender_Weiramon","Defender Weiramon","TR_Defender",-900,900,-135,500),npc("Docker_Nalesean","Nalesean","TR_Docker",4200,-1200,90,900),
                 npc("Fisher_Alanna","Mistress Alanna","TR_Fisher",4800,5200,-90,900),npc("Lord_Darlin","Lord Darlin","TR_Darlin",-6500,-4300,45,600)]
    s["dialogue"]={**lines("TR_Defender",["The Stone of Tear has never fallen. Not once in three thousand years.","Keep your hands where I can see them, outlander."]),
                   **lines("TR_Docker",["The Erinin brings the grain and the Erinin takes the coin.","Watch your step on the piers, the planks are rotten and the river is deep."]),
                   **lines("TR_Fisher",["Fresh eels! Caught this morning below the Stone.","No channeling in Tear, mind. The Defenders do not take kindly to it."]),
                   **lines("TR_Darlin",["The Maule stinks of fish and the High Lords stink of politics.","If you have a strong arm, there is always work with the Defenders."])}
    add_terrace(s,"rect",(-5300,-2700,-200,2700),600)
    lift_roads(s,skip=[f"Pier {k}" for k in range(6)]+["Dock fence"]); lift_plazas(s)

def terrain_TarValon(s):
    st=MAT("MI_Tile_WhiteStone")
    s["props"]+=[t_circle("TowerPlinth",0,0,2350,400,st,base=-100)]
    s["props"]+=stairs_circle("TowerStairs",0,0,2350,400,[0,90,180,270],0,kit_mats("SM_Stairs400",{"P_LightGrey":"P_White"}))
    s["exclusions"]+=ring_excl(0,0,2350,24,400)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2] in ("WhiteTower","Spire_NE","Spire_SW","Fountain_N"): l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],400),p[4],p[5],p[6])
    no_ground(s,[f"Bridge {k}" for k in range(4)])
    s["props"]+=[chest("TowerChest",-1500,1500,400,45),potion("Potion_Tower",1500,-1500,400),potion("Potion_Park",5000,2100,0)]
    s["props"]+=[npc("Novice_Else","Novice Else","TV_Novice",1200,1200,-135,600),npc("Warder_Gaidin","Warder","TV_Warder",2900,0,180,400),
                 npc("Ogier_Loial","Loial","TV_Loial",-5000,5000,45,1200),npc("Trader_Hob","Master Hob","TV_Hob",7400,-800,90,800)]
    s["dialogue"]={**lines("TV_Novice",["Novices are not supposed to talk to visitors. Please do not tell the Mistress of Novices.","The Tower has a hundred levels and I have scrubbed every one of them."]),
                   **lines("TV_Warder",["The Tower grounds are closed to the public. Admire it from the steps.","An Aes Sedai is not to be disturbed. Move along."]),
                   **lines("TV_Loial",["Ogier built this city, you know. Every bridge, every street. Hasty work, only three hundred years.","I would very much like to see a stedding again. The trees here are so young."]),
                   **lines("TV_Hob",["Four bridges in and four bridges out, and a toll on every one of them.","Silk from Arafel, ice peppers from Shienar. Have a look, friend."])}
    add_terrace(s,"circle",(0,0,2350),400,28)
    lift_roads(s,skip=[f"Bridge {k}" for k in range(4)]); lift_plazas(s)

def terrain_FalDara(s):
    g=MAT("MI_Ground_Tundra")
    s["props"]+=[t_circle("KeepRock",0,0,2050,500,MAT("MI_Wall_DarkStone")),t_circle("KeepRockTop",0,0,2000,520,g,base=450)]
    s["props"]+=[stair_prop("KeepStairE",2050+820,0,180,400,100),stair_prop("KeepStairW",-2050-820,0,0,400,100),
                 t_rect("KeepLandingE",2050,-300,2900,300,100,MAT("MI_Tile_Cobble")),t_rect("KeepLandingW",-2900,-300,-2050,300,100,MAT("MI_Tile_Cobble")),
                 ladder("KeepLadderN",0,2060,90,500),ladder("KeepLadderS",0,-2060,-90,500)]
    s["exclusions"]+=ring_excl(0,0,2050,20,420)
    s["roads"].append({"name":"Moat","kind":"water","pts":circle(0,0,6900,48),"closed":True,"width":5,"houses":False,"mat":WATER,"tile_z":-3})
    s["roads"].append({"name":"Moat bridge","pts":[(6400,0),(7400,0)],"width":7,"houses":False,"mat":WOODMAT,"tile_z":10})
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2]=="FalDaraKeep": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],520),p[4],p[5],p[6])
    s["props"]+=[chest("KeepChest",1200,1200,520,45),potion("Potion_Keep",-1200,-1200,520),potion("Potion_Gate",5600,-700,0),chest("WallChest",-5700,5600,0,135)]
    s["props"]+=[npc("Lord_Agelmar","Lord Agelmar","FD_Agelmar",900,-900,135,500),npc("Soldier_Ingtar","Ingtar","FD_Ingtar",3800,2600,-90,900),
                 npc("Smith_Ragan","Ragan","FD_Ragan",-3800,-2600,45,800),npc("Child_Uno","Uno","FD_Uno",5200,-3900,180,1000)]
    s["dialogue"]={**lines("FD_Agelmar",["Peace favor your sword, traveler. Fal Dara is the last light before the Blight.","The keep has stood against Trollocs for a thousand years. It will stand tonight."]),
                   **lines("FD_Ingtar",["Every Shienaran man carries a sword. Every woman knows how to use one.","The Gap is quiet. Too quiet. Keep your lantern lit."]),
                   **lines("FD_Ragan",["Good steel comes from the forges here, hammered in the cold.","Snow in the streets till Sunday. Mind the ice on the keep stairs."]),
                   **lines("FD_Uno",["Bloody flaming Trollocs took my goat last winter.","If you see a goat with one horn, that one's mine."])}
    add_terrace(s,"circle",(0,0,2050),520,24); add_terrace(s,"rect",(2050,-300,2900,300),100); add_terrace(s,"rect",(-2900,-300,-2050,300),100)
    lift_roads(s,skip=["Moat","Moat bridge"]); lift_plazas(s)

def terrain_Tanchico(s):
    sand=MAT("MI_Ground_Sand")
    s["props"]+=[t_rect("UpperTerrace",-10000,-10000,-3000,10000,600,sand),t_rect("MidTerrace",-3000,-10000,600,10000,300,sand)]
    ys_med=[-8000+(-1200+8000)*j/4 for j in range(5)]; ys_grand=[1200+(8000-1200)*j/2 for j in range(3)]
    for k,yy in enumerate(ys_med+ys_grand):
        s["props"].append(stair_prop(f"UpStairs{k}",-3000+620,yy,180,300,300)); s["props"].append(stair_prop(f"MidStairs{k}",600+620,yy,180,300,0))
    s["props"]+=[ladder("CliffLadder0",-2990,-9600,0,300,300),ladder("CliffLadder1",610,9600,0,300,0)]
    s["exclusions"]+=[(-3000,0,380,10000),(600,0,380,10000)]
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2]=="PanarchPalace": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],600),p[4],p[5],p[6])
        if p[0]=="mesh" and p[2]=="GreatDome": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],300),p[4],p[5],p[6])
    no_ground(s,[f"Pier {k}" for k in range(5)])
    s["props"]+=[chest("PalaceChest",-6500,5200,600,90),potion("Potion_Medina",-5500,-5000,600),potion("Potion_Souk",-500,600,300),chest("PierChest",5100,4600,10,0)]
    s["props"]+=[npc("Panarch_Guard","Panarch's Guard","TC_Guard",-5800,4600,-90,500),npc("Souk_Trader","Souk Trader","TC_Trader",-600,-400,45,700),
                 npc("Sailor_Bayle","Sailor","TC_Sailor",2200,2000,180,900),npc("Child_Seluca","Seluca","TC_Child",-6200,-4200,0,1200)]
    s["dialogue"]={**lines("TC_Guard",["The Panarch's Palace is closed. The Panarch's Palace is always closed.","Tanchico has three hills and three kinds of trouble. Pick one."]),
                   **lines("TC_Trader",["Silk, dyes, honey from the Mountains of Mist! Best prices in the Souk.","Mind the stairs down to the harbour. The steps are older than the city."]),
                   **lines("TC_Sailor",["Ships from Illian, ships from the Sea Folk, ships from places with no names.","The Great Dome? The Panarch's folk say it is older than the Breaking."]),
                   **lines("TC_Child",["Race you to the top of the medina! Last one up is a Whitecloak.","I know a secret way down the cliff. It's a ladder. Don't tell."])}
    add_terrace(s,"rect",(-10000,-10000,-3000,10000),600); add_terrace(s,"rect",(-3000,-10000,600,10000),300)
    lift_roads(s,skip=[f"Pier {k}" for k in range(5)]+["Sea wall"]); lift_plazas(s)

def terrain_Illian(s):
    st=MAT("MI_Tile_WhiteStone")
    s["props"]+=[t_rect("PalacePlinth",-1500,-1500,1500,-150,300,st)]
    s["props"]+=[stair_prop("PalaceStairsS",0,-1500-620,90,300,0),stair_prop("PalaceStairsN",0,-150+620,-90,300,0)]
    s["exclusions"]+=rect_excl(-1500,-1500,1500,-150,380)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2]=="KingsPalace": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],300),p[4],p[5],p[6])
    s["props"]+=[chest("PalaceChest",-1200,-1200,300,45),potion("Potion_Palace",1200,-1200,300),potion("Potion_Canal",6400,3000,0)]
    s["props"]+=[npc("Councilor_Crane","Lord Crane","IL_Crane",900,-900,-135,500),npc("Boatman_Dilham","Dilham","IL_Boat",-6000,3000,0,900),
                 npc("Hunter_Horn","Hunter of the Horn","IL_Hunter",5000,-5000,135,1200),npc("Flower_Girl","Nieda","IL_Nieda",2100,2100,-135,800)]
    s["dialogue"]={**lines("IL_Crane",["The Council of Nine do say the King rules Illian. The King do say otherwise.","Mind the canals after dark. The water be deeper than it looks."]),
                   **lines("IL_Boat",["A boat ride through the canals, two coppers. Bridges be free.","The reeds grow thick this year. Good for baskets, bad for boats."]),
                   **lines("IL_Hunter",["I have sworn the oath of the Hunters of the Horn. Have you heard it sounded?","Every road out of Illian leads to the Horn of Valere. I just do not know which."]),
                   **lines("IL_Nieda",["Flowers for your lady? The ones from the marsh never wilt.","The Bees of Illian, they call us. Busy, busy, and the honey all goes to the Nine."])}
    add_terrace(s,"rect",(-1500,-1500,1500,-150),300)
    lift_roads(s); lift_plazas(s)

def terrain_ShadarLogoth(s):
    ash=MAT("MI_Ground_Ash")
    s["props"]+=[t_circle("CitadelHill",0,0,2300,500,MAT("MI_Wall_DarkStone")),t_circle("CitadelHillTop",0,0,2250,520,ash,base=450)]
    s["props"]+=[stair_prop("CitadelStairs",2300+820,0,180,400,100,kit_mats("SM_Stairs400",{"P_LightGrey":"P_DarkGrey","P_Grey":"P_Black"})),t_rect("CitadelLanding",2300,-300,3200,300,100,ash),
                 ladder("CitadelLadder",-2310,0,180,500)]
    s["exclusions"]+=ring_excl(0,0,2300,20,420)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2] in ("RuinedCitadel","Chest_Treasure"): l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],520),p[4],p[5],p[6])
    s["props"]+=[potion("Potion_Hill",-1200,1200,520),potion("Potion_Gate",6400,700,0),chest("RuinChest",-4500,-4500,0,20)]
    s["props"]+=[npc("Ghost_Mordeth","Mordeth","SL_Mordeth",-600,-600,45,300)]
    s["dialogue"]=lines("SL_Mordeth",["Treasure... there is treasure here, friend. Take what you like.","No one leaves Shadar Logoth with more than they brought. Except me."])
    add_terrace(s,"circle",(0,0,2300),520,24); add_terrace(s,"rect",(2300,-300,3200,300),100)
    lift_roads(s); lift_plazas(s)

def terrain_Whitebridge(s):
    g=GRASSMAT
    s["props"]=[p for p in s["props"] if p[2]!="Ground"]
    s["props"]+=[t_rect("RiverBed",-6100,-6100,6100,6100,-300,MAT("MI_Ground_Dirt"),base=-600),
                 t_rect("WestBank",-6100,-6100,1300,6100,0,g),t_rect("EastBank",3100,-6100,6100,6100,0,g),
                 water_plane("Arinelle",2200,0,12200,-150)]
    s["roads"]=[r for r in s["roads"] if gv(r,"name","") not in ("Arinelle",)]
    s["exclusions"]+=[(1300,0,400,6100),(3100,0,400,6100)]
    s["props"]+=[ladder("BankLadderW",1290,-1800,0,300,-300),ladder("BankLadderE",3110,1500,180,300,-300),
                 stair_prop("BankStairs",1300-620,2600,0,300,-300)]
    no_ground(s,["The White Bridge"])
    s["props"]+=[chest("RiverChest",2200,-4000,-300,90),potion("Potion_Bank",2200,3500,-300),potion("Potion_Inn",-2000,2300,0)]
    s["props"]+=[npc("Innkeeper_Fitch","Master Fitch","WB_Fitch",-2600,900,-45,500),npc("Ferryman","Ferryman","WB_Ferry",800,-1200,90,700),
                 npc("Gleeman_Kid","Olver","WB_Olver",4200,-800,135,1200)]
    s["dialogue"]={**lines("WB_Fitch",["The Wayfarer's Rest, best inn on the Arinelle. Mind the stairs, the cellar floods.","The White Bridge? Older than the town. Harder than steel. Nobody knows who built it."]),
                   **lines("WB_Ferry",["Nobody takes the ferry since the bridge. Cheaper to walk.","The water runs cold and fast below the bridge. I have fished men out of it."]),
                   **lines("WB_Olver",["Did you know the bridge looks like glass but you can't see through it?","Thom the gleeman said he'd teach me to juggle. Then he left."])}
    s["base_z"]=-300
    add_terrace(s,"rect",(-6100,-6100,1300,6100),0); add_terrace(s,"rect",(3100,-6100,6100,6100),0)
    lift_roads(s,skip=["The White Bridge"]); lift_plazas(s)

def terrain_Baerlon(s):
    g=GRASSMAT
    s["props"]+=[t_circle("TownHill",0,0,1850,300,MAT("MI_Wall_Stone")),t_circle("TownHillTop",0,0,1800,320,g,base=250)]
    s["props"]+=stairs_circle("HillStairs",0,0,1850,300,[90,270,0,180],0)
    s["exclusions"]+=ring_excl(0,0,1850,16,380)
    for i,p in enumerate(s["props"]):
        if p[0]=="mesh" and p[2]=="Well": l=p[3]; s["props"][i]=(p[0],p[1],p[2],(l[0],l[1],320),p[4],p[5],p[6])
    s["props"]+=[chest("HillChest",900,-900,320,45),potion("Potion_Hill",-900,900,320),potion("Potion_Gate",700,4200,0)]
    s["props"]+=[npc("Innkeeper_Fitch2","Master Fitch","BL_Fitch",-1000,-1700,90,500),npc("Miner_Jon","Jon Thane","BL_Miner",2800,-2800,135,900),
                 npc("Whitecloak","Child of the Light","BL_Whitecloak",-2800,2900,-45,700),npc("Wisdom_Nynaeve","Nynaeve","BL_Nynaeve",400,600,-120,600)]
    s["dialogue"]={**lines("BL_Fitch",["The Stag and Lion welcomes you. Rooms upstairs, stew downstairs, and no fighting.","The Whitecloaks have been asking questions. I did not tell them anything."]),
                   **lines("BL_Miner",["Baerlon is a mining town, whatever the fine folk say. Iron and silver.","The hills north of here are full of old shafts. Mind where you step."]),
                   **lines("BL_Whitecloak",["Walk in the Light, friend. The Children watch over Baerlon.","Darkfriends hide in plain sight. Report anything unusual to the Children."]),
                   **lines("BL_Nynaeve",["You came all this way with no cloak? You will catch your death.","When we get home, there will be words. Many words."])}
    add_terrace(s,"circle",(0,0,1850),320,20)
    lift_roads(s); lift_plazas(s)

TERRAIN={"EmondsField":terrain_EmondsField,"Caemlyn":terrain_Caemlyn,"Cairhien":terrain_Cairhien,"Tear":terrain_Tear,"TarValon":terrain_TarValon,"FalDara":terrain_FalDara,
         "Tanchico":terrain_Tanchico,"Illian":terrain_Illian,"ShadarLogoth":terrain_ShadarLogoth,"Whitebridge":terrain_Whitebridge,"Baerlon":terrain_Baerlon}
