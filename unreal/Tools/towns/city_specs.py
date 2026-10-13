# ---------------- shared helpers for the city specs ----------------
H=MESH["house"]
def WALLM(n): return MAT(n)
PORTAL_BP="/Game/Wot/Actors/Portal"; CHEST="/Game/Wot/Actors/Props/Chest"; CRATE="/Game/Wot/Actors/Props/BP_BreakableCrate"; CAMP="/Game/Wot/Actors/Props/Campfire"; FIRE="/Game/Wot/Actors/Fireflies"
HORSE="/Game/Wot/NPCs/animals/horse/HorseBP"; SHEEP="/Game/Wot/NPCs/animals/sheep/Sheep_BP"; PEN="/Game/Wot/GeometryScripting/BP_AnimalPen"
def grid_streets(x0,x1,y0,y1,nx,ny,width=5,houses=True,spacing=1300,setback=850,set_=None,prefix="st"):
    """Rectilinear street grid between (x0,y0)-(x1,y1): nx vertical and ny horizontal streets."""
    out=[]
    for i in range(nx):
        x=x0+(x1-x0)*i/(nx-1) if nx>1 else (x0+x1)/2
        r={"name":f"{prefix} v{i}","pts":[(x,y0),(x,(y0+y1)/2),(x,y1)],"width":width,"houses":houses,"spacing":spacing,"setback":setback,"linear":True}
        if set_: r["set"]=set_
        out.append(r)
    for j in range(ny):
        y=y0+(y1-y0)*j/(ny-1) if ny>1 else (y0+y1)/2
        r={"name":f"{prefix} h{j}","pts":[(x0,y),((x0+x1)/2,y),(x1,y)],"width":width,"houses":houses,"spacing":spacing,"setback":setback,"linear":True}
        if set_: r["set"]=set_
        out.append(r)
    return out
def radial_streets(cx,cy,r0,r1,n,width=5,houses=True,spacing=1400,setback=900,start=0.0,set_=None):
    out=[]
    for k in range(n):
        th=start+2*math.pi*k/n; t=(math.cos(th),math.sin(th))
        r={"name":f"radial {k}","pts":[(cx+r0*t[0],cy+r0*t[1]),(cx+(r0+r1)/2*t[0],cy+(r0+r1)/2*t[1]),(cx+r1*t[0],cy+r1*t[1])],"width":width,"houses":houses,"spacing":spacing,"setback":setback}
        if set_: r["set"]=set_
        out.append(r)
    return out
def lamp_line(step=1400,offset=None,mat="P_Gold"):
    return {"mesh":kit_mesh("SM_LampPost"),"step":step,"offset":offset or 380,"mat":None,"scale":(1,1,1)}
def tree_line(step=1200,offset=420,mesh=None):
    return {"mesh":mesh or MESH["tree3"],"step":step,"offset":offset,"mat":None,"scale":(0.9,0.9,0.9)}
def base_foliage(trees,forest_ratio=0.2,edge_ratio=0.05,town_ratio=0.008,bush=0.02,grass=0.3,mush=0.0,extent=5000,fr=None,clip=1.0,town_trees=None,extra=None,bands=None):
    fr=fr or extent-400
    b=bands or {"forest":(0.84,1.0),"edge":(0.6,0.84),"town":(0.2,0.6)}
    return {"extent":extent,"forest_radius":fr,"clip_density":clip,"trees":trees,
            "forest_band":b["forest"],"forest_ratio":forest_ratio,"edge_band":b["edge"],"edge_ratio":edge_ratio,"town_band":b["town"],"town_ratio":town_ratio,
            "town_trees":town_trees or trees,"bush_ratio":bush,"grass_ratio":grass,"mush_ratio":mush,"extra":extra or []}
TREES_TEMPERATE=[(MESH["tree1"],3,None),(MESH["tree2"],3,None),(MESH["tree3"],2,None),(MESH["tree4"],2,None),(MESH["oldtree"],1,None)]
TREES_FORMAL=[(MESH["tree3"],3,None),(MESH["tree4"],3,None),(kit_mesh("SM_Birch"),2,None)]
def TREES_PINE(snow=False):
    e=[kit_entry("SM_Pine",{"P_DarkGreen":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_DarkGreen":"P_Olive"},1),(MESH["tree4"],1,None)]
    if snow: e=[kit_entry("SM_Pine",{"P_DarkGreen":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_DarkGreen":"P_Ice"},1),(MESH["tree4"],1,None)]
    return e
TREES_DESERT=[kit_entry("SM_Palm",None,3),kit_entry("SM_Palm",{"P_Green":"P_Olive"},2),(MESH["tree3"],1,None)]
TREES_DEAD=[kit_entry("SM_DeadTree",None,4),kit_entry("SM_DeadTree",{"P_DarkGrey":"P_Black"},2)]
def ground_props(town,size,mat,extra=()):
    """Standard level props: ground plane, plus optional extras."""
    return [("mesh",CUBE,"Ground",(0,0,-1),0,(size/100.0,size/100.0,0.02),[mat])]+list(extra)
def level_scale(size): return (size/200.0,size/200.0,10)
def portal_moves(x,y,yaw):
    """Portal and player start at the town entrance, facing yaw (player looks along yaw into town)."""
    dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw))
    return [("OverworldPortal",(x-dx*250,y-dy*250,120),yaw),("OverworldPlayerStart",(x+dx*200,y+dy*200,93),yaw)]
def rand_house_entries(walls,roofs,woods=("MI_Wood_Dark","MI_Wood_Grey"),w=1):
    e=[]
    for i,wl in enumerate(walls):
        for j,rf in enumerate(roofs):
            e.append((H,w,[MAT(rf),MAT(woods[(i+j)%len(woods)]),MAT(wl),None,None]))
    return e

# ====================================================================================
def caemlyn_spec():
    COB=MAT("MI_Tile_Cobble"); WS=MAT("MI_Tile_WhiteStone"); PL=MAT("MI_Tile_Plaza")
    houses=rand_house_entries(["MI_Wall_White","MI_Wall_Plaster","MI_Wall_Stone"],["MI_Roof_RedTile","MI_Roof_Slate"])+[
        kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_RedBrick"},2),kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_Slate"},2),
        kit_entry("SM_GableHouse",{"P_Plaster":"P_White","P_Terracotta":"P_RedBrick"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_LightGrey","P_Terracotta":"P_Slate"},1)]
    inner=rand_house_entries(["MI_Wall_White"],["MI_Roof_RedTile","MI_Roof_Gold"],w=2)+[kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_Gold"},3),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_RedBrick"},2)]
    stalls=[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None),(kit_mesh("SM_MarketStall"),0.03,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Blue"}))]
    roads=[
      {"name":"Inner wall","kind":"wall","pts":circle(0,0,4300,36),"closed":True,"width":2,"tile_scale":(1,1,3.6),"tile_z":180,"mat":WS,"towers":{"step":1700,"scale":(2.8,2.8,5.6),"z":280,"mat":MAT("MI_Wall_Stone")}},
      {"name":"Outer wall","kind":"wall","pts":circle(0,0,8600,56),"closed":True,"width":2,"tile_scale":(1,1,3.0),"tile_z":150,"mat":MAT("MI_Wall_RedStone"),"towers":{"step":2200,"scale":(2.6,2.6,4.8),"z":240,"mat":MAT("MI_Wall_DarkStone")}},
      {"name":"Palace ring","pts":circle(0,0,2600,24),"closed":True,"width":5,"mat":WS,"houses":True,"sides":"right","spacing":1700,"setback":950,"set":"inner","lines":[lamp_line(1300,330)]},
      {"name":"Inner ring","pts":circle(0,0,3600,30),"closed":True,"width":5,"houses":False,"lines":[tree_line(900,-380)]},
      {"name":"Middle ring","pts":circle(0,0,5500,40),"closed":True,"width":7,"houses":True,"spacing":1300,"setback":900},
      {"name":"Outer ring","pts":circle(0,0,7400,48),"closed":True,"width":7,"houses":True,"spacing":1300,"setback":900},
      {"name":"Lane ring","pts":circle(0,0,9300,56),"closed":True,"width":5,"houses":True,"sides":"left","spacing":1200,"setback":850},
    ]+radial_streets(0,0,2600,9800,12,width=7,spacing=1400,setback=950)
    for r in roads[6:]: r["lines"]=[lamp_line(1400,480)]
    roads.append({"name":"Market lane","pts":[(6400,-1600),(7600,-1500),(8300,-900)],"width":5,"houses":True,"spacing":1300,"setback":850})
    return {
     "town":"Caemlyn","level_scale":LEVEL_SIZES["Caemlyn"],"graph_name":"PCG_Caemlyn","folder":"/Game/Wot/Towns/Caemlyn","center":(0,0),"road_mat":COB,
     "plazas":[{"pts":circle(0,0,2300,28),"mat":PL,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None)]},
               {"pts":rect(7300,-2400,9200,-500),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":stalls},
               {"pts":rect(-9200,2600,-7400,4400),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":stalls},
               {"pts":circle(5000,5000,1300,16),"mat":MAT("MI_Ground_Lush"),"scatter":[(MESH["tree3"],0.03,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.05,0.9,1.3,None)]},
               {"pts":circle(-5000,-5000,1300,16),"mat":MAT("MI_Ground_Lush"),"scatter":[(MESH["tree4"],0.03,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.05,0.9,1.3,None)]}],
     "exclusions":[(0,0,1500,1500),(9500,0,600,800)],
     "roads":roads,
     "houses":{"spacing":1300,"setback":900,"scale":(0.8,1.0),"keep":0.94,"footprint":(-520,-480,660,380),"entries":houses},
     "house_sets":{"inner":{"spacing":1700,"setback":950,"scale":(0.9,1.15),"keep":0.95,"entries":inner}},
     "foliage":base_foliage(TREES_TEMPERATE,forest_ratio=0.1,edge_ratio=0.02,town_ratio=0.004,bush=0.012,grass=0.22,extent=10000,town_trees=TREES_FORMAL,bands={"forest":(0.93,1.0),"edge":(0.86,0.93),"town":(0.3,0.86)}),
     "props":ground_props("Caemlyn",20000,MAT("MI_Ground_Lush"))+[
       ("mesh",kit_mesh("SM_Keep"),"RoyalPalace",(-600,0,0),180,2.3,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_LightGrey","P_Slate":"P_Gold"})),
       ("mesh",kit_mesh("SM_Obelisk"),"SunSpire_N",(900,-1500,0),0,1.3,None),("mesh",kit_mesh("SM_Obelisk"),"SunSpire_S",(900,1500,0),0,1.3,None),
       ("mesh",kit_mesh("SM_StoneWell"),"PalaceFountain",(1200,0,0),0,1.6,kit_mats("SM_StoneWell",{"P_Grey":"P_White"})),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(8600,0,0),0,1.2,kit_mats("SM_GateHouse",{"P_Grey":"P_RedBrick"})),
       ("mesh",kit_mesh("SM_GateHouse"),"WestGate",(-8600,0,0),0,1.2,kit_mats("SM_GateHouse",{"P_Grey":"P_RedBrick"})),
       ("mesh",CHEST,"Chest_Market",(8000,-1800,0),90,1.0,None),("mesh",HORSE,"MarketHorse",(7700,-700,80),-120,1.0,None),
       ("mesh",FIRE,"Fireflies0",(5000,5000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-5000,-5000,150),0,1.0,None)],
     "moves":portal_moves(9400,0,180),
    }

def cairhien_spec():
    COB=MAT("MI_Tile_Cobble"); WS=MAT("MI_Tile_WhiteStone")
    tall=lambda rc,w: kit_entry("SM_TowerHouse",rc,w)
    houses=[tall({"P_Plaster":"P_LightGrey","P_Slate":"P_Slate","P_DarkWood":"P_DarkGrey"},3),tall({"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_DarkGrey"},3),
            tall({"P_Plaster":"P_Plaster","P_Slate":"P_DarkGrey","P_DarkWood":"P_DarkGrey"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_LightGrey","P_Terracotta":"P_Slate"},2),
            kit_entry("SM_CourtyardHouse",{"P_Sand":"P_LightGrey","P_Terracotta":"P_Slate"},1)]+rand_house_entries(["MI_Wall_Stone","MI_Wall_White"],["MI_Roof_Slate"],w=1)
    roads=grid_streets(-8500,8500,-8500,8500,9,9,width=5,spacing=1150,setback=800)
    for r in roads: r["lines"]=[lamp_line(1150,400,"P_Gold")]
    roads+=[{"name":"City wall","kind":"wall","pts":rect(-9200,-9200,9200,9200),"closed":True,"linear":True,"width":2,"tile_scale":(1,1,3.4),"tile_z":170,"mat":MAT("MI_Wall_Stone"),"towers":{"step":2300,"scale":(2.8,2.8,5.6),"z":280,"mat":MAT("MI_Wall_DarkStone")}},
            {"name":"Sun avenue","pts":[(-9800,0),(0,0),(9800,0)],"width":11,"houses":False,"mat":WS,"lines":[tree_line(1000,700,MESH["tree3"]),lamp_line(1000,560)]}]
    topless=[("mesh",kit_mesh("SM_Obelisk"),f"ToplessTower{k}",(math.cos(math.radians(45+90*k))*3400,math.sin(math.radians(45+90*k))*3400,0),0,3.2,kit_mats("SM_Obelisk",{"P_White":"P_LightGrey","P_Gold":"P_LightGrey"})) for k in range(4)]
    return {
     "town":"Cairhien","level_scale":LEVEL_SIZES["Cairhien"],"graph_name":"PCG_Cairhien","folder":"/Game/Wot/Towns/Cairhien","center":(0,0),"road_mat":COB,
     "plazas":[{"pts":rect(-2600,-2600,2600,2600),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_LampPost"),0.008,1,1,None)]},
               {"pts":rect(5200,5200,8000,8000),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]}],
     "exclusions":[(0,0,1900,1900)]+[(math.cos(math.radians(45+90*k))*3400,math.sin(math.radians(45+90*k))*3400,600,600) for k in range(4)]+[(9400,0,500,800)],
     "roads":roads,
     "houses":{"spacing":1150,"setback":800,"scale":(0.9,1.2),"keep":0.96,"footprint":(-380,-380,460,380),"jitter":40,"entries":houses},
     "foliage":base_foliage(TREES_FORMAL,forest_ratio=0.0,edge_ratio=0.0,town_ratio=0.003,bush=0.006,grass=0.18,extent=10000,bands={"forest":(0.99,1.0),"edge":(0.97,0.99),"town":(0.3,0.97)}),
     "props":ground_props("Cairhien",20000,MAT("MI_Ground_Tundra"))+[
       ("mesh",kit_mesh("SM_Keep"),"SunPalace",(-700,0,0),180,2.6,kit_mats("SM_Keep",{"P_DarkGrey":"P_LightGrey","P_Grey":"P_White","P_Slate":"P_Gold"})),
       ("mesh",kit_mesh("SM_StoneWell"),"SunFountain",(1500,0,0),0,2.0,kit_mats("SM_StoneWell",{"P_Grey":"P_LightGrey","P_Terracotta":"P_Gold"})),
       ("mesh",kit_mesh("SM_GateHouse"),"JangaiGate",(9200,0,0),0,1.4,None),("mesh",kit_mesh("SM_GateHouse"),"WestGate",(-9200,0,0),0,1.4,None),
       ("mesh",CHEST,"Chest_Market",(6500,6500,0),45,1.0,None),("mesh",HORSE,"Horse",(7000,5600,80),-120,1.0,None)]+topless,
     "moves":portal_moves(9400,0,180),
    }

def tear_spec():
    COB=MAT("MI_Tile_Cobble")
    rich=rand_house_entries(["MI_Wall_Stone","MI_Wall_Plaster"],["MI_Roof_Slate","MI_Roof_RedTile"])+[kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_Slate"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_Plaster","P_Terracotta":"P_Slate"},2)]
    poor=[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_DarkGrey"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Ochre","P_Terracotta":"P_Slate"},2),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_DarkGrey"},1)]
    # docks district: tight alleys in the strip x in [1500,5500] (river at x>5600), y in [-8500,8500]
    alleys=grid_streets(1800,5000,-8000,8000,3,9,width=3,spacing=900,setback=620,set_="docks",prefix="alley")
    hi=grid_streets(-8500,-1200,-8500,8500,5,6,width=7,spacing=1400,setback=950,prefix="ave")
    for r in hi: r["lines"]=[lamp_line(1600,520)]
    roads=[{"name":"Stone wall","kind":"wall","pts":rect(-5200,-2600,-300,2600),"closed":True,"linear":True,"width":2,"tile_scale":(1,1,2.8),"tile_z":140,"mat":MAT("MI_Wall_DarkStone"),"towers":{"step":2000,"scale":(2.6,2.6,4.6),"z":230,"mat":MAT("MI_Wall_Stone")}},
           {"name":"Gate road","pts":[(-300,0),(900,0),(1800,0)],"width":7,"houses":False},
           {"name":"Riverside avenue","pts":[(5300,-9500),(5300,-4000),(5300,0),(5300,4000),(5300,9500)],"width":7,"houses":True,"sides":"left","spacing":1100,"setback":700,"set":"docks","lines":[{"mesh":kit_mesh("SM_Reeds"),"step":260,"offset":-430,"mat":None,"scale":(1,1,1),"sides":"right"}]},
           {"name":"Dock fence","kind":"wall","pts":[(5700,-9400),(5700,9400)],"width":1,"mesh":MESH["fence"],"mat":None,"tile_step":250,"tile_scale":(1,1,1),"align":True}]
    for k,yy in enumerate((-6500,-3300,-900,1800,4400,7200)):
        roads.append({"name":f"Pier {k}","pts":[(5400,yy),(6300,yy),(7200+ (k%2)*500,yy)],"width":3,"houses":False,"mat":WOODMAT,"tile_z":8,"tile_scale":(1,1,0.1)})
    roads+=alleys+hi
    return {
     "town":"Tear","level_scale":LEVEL_SIZES["Tear"],"graph_name":"PCG_Tear","folder":"/Game/Wot/Towns/Tear","center":(-1500,0),"road_mat":COB,
     "plazas":[{"pts":rect(-200,-1200,1700,1200),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.04,0.9,1.1,None)]},
               {"pts":rect(2200,-9000,4800,-6800),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.06,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Olive"}))]}],
     "exclusions":[(-2750,0,2600,2600),(7600,0,2200,10000),(5300,-9000,500,700)],
     "roads":roads,
     "houses":{"spacing":1400,"setback":950,"scale":(0.85,1.05),"keep":0.94,"footprint":(-520,-480,660,380),"entries":rich},
     "house_sets":{"docks":{"spacing":900,"setback":620,"scale":(0.55,0.72),"keep":0.95,"footprint":(-420,-380,520,330),"jitter":60,"entries":poor}},
     "foliage":base_foliage(TREES_TEMPERATE,forest_ratio=0.08,edge_ratio=0.02,town_ratio=0.004,bush=0.012,grass=0.22,extent=10000,bands={"forest":(0.94,1.0),"edge":(0.85,0.94),"town":(0.3,0.85)},
                            extra=[{"name":"Reeds","band":(0.0,1.0),"ratio":0.0,"entries":[],"scale":(1,1)}]),
     "props":ground_props("Tear",20000,MAT("MI_Ground_Lush"))+[
       ("mesh",CUBE,"RiverErinin",(8000,0,-1),0,(44,200,0.06),[WATER]),
       ("mesh",MESH["stone"],"StoneOfTear",(-2750,0,0),0,20.0,None),("mesh",MESH["tower"],"StoneKeep",(-2750,0,2900),0,5.0,None),
       ("mesh",kit_mesh("SM_GateHouse"),"StoneGate",(-300,0,0),0,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey"})),
       ("mesh",CHEST,"Chest_Pier",(6900,-900,10),0,1.0,None),("mesh",CHEST,"Chest_Market",(3500,-8000,0),90,1.0,None),
       ("mesh",CAMP,"DockFire",(4600,2600,0),0,1.0,None),("mesh",HORSE,"DockHorse",(2900,-7700,80),-30,1.0,None),
       ("mesh",FIRE,"Fireflies0",(5000,3000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-6000,-4000,150),0,1.0,None)],
     "moves":portal_moves(5300,-9000,90),
    }

def tarvalon_spec():
    WS=MAT("MI_Tile_WhiteStone")
    white=[kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_Gold"},3),kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Blue","P_Ochre":"P_Gold"},3),
           kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Gold","P_Ochre":"P_LightGrey"},1),kit_entry("SM_GableHouse",{"P_Plaster":"P_White","P_Terracotta":"P_Slate","P_DarkWood":"P_Gold"},2),
           kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Blue"},2)]+rand_house_entries(["MI_Wall_TarValon"],["MI_Roof_Blue","MI_Roof_Gold"],woods=("MI_Trim_Gold",))
    radials=radial_streets(0,0,2600,8200,12,width=5,houses=True,spacing=1400,setback=900)
    for r in radials: r["lines"]=[lamp_line(1500,520,"P_Gold")]
    posts={"step":500,"scale":(1.2,1.2,2.6),"z":-130,"mat":MAT("MI_Wall_Stone")}
    bridges=[{"name":f"Bridge {k}","pts":[(math.cos(th)*8200,math.sin(th)*8200),(math.cos(th)*9400,math.sin(th)*9400),(math.cos(th)*10600,math.sin(th)*10600)],"width":5,"houses":False,"mat":WS,"tile_z":6,"towers":posts} for k,th in enumerate([0,math.pi/2,math.pi,3*math.pi/2])]
    parks=[{"pts":circle(math.cos(th)*5400,math.sin(th)*5400,1100,14),"mat":MAT("MI_Ground_Lush"),"scatter":[(kit_mesh("SM_Birch"),0.04,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.06,0.9,1.3,None)]} for th in [math.pi/8+k*math.pi/2 for k in range(4)]]
    return {
     "town":"TarValon","level_scale":LEVEL_SIZES["TarValon"],"graph_name":"PCG_TarValon","folder":"/Game/Wot/Towns/TarValon","center":(0,0),"road_mat":WS,
     "plazas":[{"pts":circle(0,0,2300,32),"mat":WS,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None),(kit_mesh("SM_Obelisk"),0.004,0.6,0.8,None)]}]+parks,
     "exclusions":[(0,0,1500,1500)]+[(math.cos(th)*8700,math.sin(th)*8700,500,700) for th in [0,math.pi/2,math.pi,3*math.pi/2]],
     "roads":[{"name":"Tower ring","pts":circle(0,0,2600,32),"closed":True,"width":5,"houses":False,"lines":[tree_line(900,-380,MESH["tree3"])]},
              {"name":"Inner ring","pts":circle(0,0,4300,36),"closed":True,"width":5,"houses":True,"sides":"both","spacing":1500,"setback":900},
              {"name":"Outer ring","pts":circle(0,0,6000,44),"closed":True,"width":5,"houses":True,"sides":"both","spacing":1400,"setback":900},
              {"name":"Shore ring","pts":circle(0,0,7700,52),"closed":True,"width":5,"houses":True,"sides":"left","spacing":1400,"setback":900}]+radials+bridges,
     "houses":{"spacing":1400,"setback":900,"scale":(0.8,1.0),"keep":0.93,"footprint":(-520,-480,660,380),"entries":white},
     "foliage":base_foliage(TREES_FORMAL,forest_ratio=0.0,edge_ratio=0.03,town_ratio=0.008,bush=0.018,grass=0.28,extent=10000,fr=9000,clip=0.93,bands={"forest":(0.99,1.0),"edge":(0.82,0.93),"town":(0.3,0.82)}),
     "props":[("mesh",CUBE,"RiverErinin",(0,0,-170),0,(300,300,0.3),[WATER]),("mesh",CYL,"Island",(0,0,-100),0,(170,170,2),[MAT("MI_Ground_Lush")]),
       ("mesh",MESH["tower"],"WhiteTower",(0,0,0),0,24.0,None),
       ("mesh",kit_mesh("SM_Obelisk"),"Spire_NE",(1600,-1600,0),0,1.6,None),("mesh",kit_mesh("SM_Obelisk"),"Spire_SW",(-1600,1600,0),0,1.6,None),
       ("mesh",kit_mesh("SM_StoneWell"),"Fountain_N",(0,1800,0),0,1.8,kit_mats("SM_StoneWell",{"P_Grey":"P_White","P_Terracotta":"P_Blue"})),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(8200,0,0),0,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),
       ("mesh",CHEST,"Chest_Bridge",(7700,500,0),0,1.0,None),("mesh",HORSE,"BridgeHorse",(7400,900,80),200,1.0,None),
       ("mesh",FIRE,"Fireflies0",(0,-1200,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-5400,5400,150),0,1.0,None)],
     "moves":portal_moves(8700,0,180),
    }

def faldara_spec():
    COB=MAT("MI_Tile_Cobble")
    houses=[kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_Slate"},3),kit_entry("SM_Longhouse",{"P_Brown":"P_Brown","P_Olive":"P_DarkGrey"},2),
            kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Slate","P_DarkWood":"P_Black"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_DarkGrey","P_Terracotta":"P_Snow","P_DarkWood":"P_DarkWood"},2),
            kit_entry("SM_TowerHouse",{"P_Plaster":"P_DarkGrey","P_Slate":"P_Slate","P_DarkWood":"P_Black"},1)]
    streets=grid_streets(-5200,5200,-5200,5200,5,5,width=5,spacing=1100,setback=760,prefix="st")
    for r in streets: r["lines"]=[lamp_line(1800,330,"P_Copper")]
    roads=[{"name":"Fortress wall","kind":"wall","pts":rect(-6200,-6200,6200,6200),"closed":True,"linear":True,"width":3,"tile_scale":(1,1,4.0),"tile_z":200,"mat":MAT("MI_Wall_DarkStone"),"towers":{"step":2050,"scale":(3.2,3.2,6.4),"z":320,"mat":MAT("MI_Wall_Stone")}},
           {"name":"Keep ring","pts":circle(0,0,2300,20),"closed":True,"width":5,"houses":False},
           {"name":"East road","pts":[(6200,0),(7100,0)],"width":7,"houses":False}]+streets
    return {
     "town":"FalDara","level_scale":LEVEL_SIZES["FalDara"],"graph_name":"PCG_FalDara","folder":"/Game/Wot/Towns/FalDara","center":(0,0),"road_mat":COB,
     "plazas":[{"pts":rect(-1900,-1900,1900,1900),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.03,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_DarkGrey"}))]}],
     "exclusions":[(-400,0,1700,1500),(6700,0,500,700)],
     "roads":roads,
     "houses":{"spacing":1100,"setback":760,"scale":(0.7,0.9),"keep":0.95,"footprint":(-520,-420,560,380),"jitter":60,"entries":houses},
     "foliage":base_foliage(TREES_PINE(snow=True),forest_ratio=0.28,edge_ratio=0.06,town_ratio=0.004,bush=0.008,grass=0.12,extent=7500,bands={"forest":(0.9,1.0),"edge":(0.84,0.9),"town":(0.3,0.84)},
                            extra=[{"name":"Boulders","band":(0.8,1.0),"ratio":0.02,"entries":[kit_entry("SM_Boulder",None,3),kit_entry("SM_Boulder",{"P_Grey":"P_Ice"},1)],"scale":(0.8,1.6),"prune":200},
                                   {"name":"Snow","band":(0.0,1.0),"ratio":0.05,"entries":[kit_entry("SM_SnowPatch")],"scale":(0.8,1.8)}]),
     "props":ground_props("FalDara",15000,MAT("MI_Ground_Tundra"))+[
       ("mesh",kit_mesh("SM_Keep"),"FalDaraKeep",(-400,0,0),180,1.9,None),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(6200,0,0),0,1.5,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_Grey"})),
       ("mesh",CHEST,"Chest_Keep",(1200,-1200,0),45,1.0,None),("mesh",HORSE,"WarHorse",(1500,1300,80),-100,1.0,None),
       ("mesh",CAMP,"GateFire",(5600,700,0),0,1.0,None)],
     "moves":portal_moves(6700,0,180),
    }

def tanchico_spec():
    SAND=MAT("MI_Tile_Plaza"); WS=MAT("MI_Tile_WhiteStone")
    houses=[kit_entry("SM_DomeHouse",{"P_Cream":"P_Cream","P_Teal":"P_Teal","P_Ochre":"P_Ochre"},3),kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Copper","P_Ochre":"P_Terracotta"},2),
            kit_entry("SM_DomeHouse",{"P_Cream":"P_Sand","P_Teal":"P_Blue","P_Ochre":"P_White"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_Sand","P_Terracotta":"P_Terracotta"},2),
            kit_entry("SM_TowerHouse",{"P_Plaster":"P_Cream","P_Slate":"P_Copper","P_DarkWood":"P_Teal"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Teal"},1)]
    medina=grid_streets(-8000,2000,-8000,-1200,6,5,width=3,spacing=850,setback=600,set_="medina",prefix="alley")
    for r in medina: r["linear"]=False   # slightly wandering lanes
    grand=grid_streets(-8000,2000,1200,8000,3,3,width=7,spacing=1700,setback=1050,prefix="ave")
    for r in grand: r["lines"]=[{"mesh":kit_mesh("SM_Palm"),"step":2600,"offset":560,"mat":None,"scale":(0.6,0.6,0.6)},lamp_line(1700,380,"P_Copper")]
    roads=[{"name":"Harbour road","pts":[(2600,-9500),(2600,-4000),(2600,0),(2600,4000),(2600,9500)],"width":7,"houses":True,"sides":"left","spacing":1300,"setback":850,"lines":[{"mesh":kit_mesh("SM_Palm"),"step":2200,"offset":-460,"mat":None,"scale":(0.6,0.6,0.6),"sides":"right"}]},
           {"name":"Sea wall","kind":"wall","pts":[(3300,-9400),(3300,9400)],"width":1,"tile_scale":(1,1,1.2),"tile_z":60,"mat":MAT("MI_Tile_WhiteStone")}]
    for k,yy in enumerate((-7000,-3500,500,4500,8000)):
        roads.append({"name":f"Pier {k}","pts":[(3000,yy),(4200,yy),(5600,yy)],"width":3,"houses":False,"mat":WOODMAT,"tile_z":8,"tile_scale":(1,1,0.1)})
    roads+=medina+grand
    return {
     "town":"Tanchico","level_scale":LEVEL_SIZES["Tanchico"],"graph_name":"PCG_Tanchico","folder":"/Game/Wot/Towns/Tanchico","center":(-3000,0),"road_mat":SAND,
     "plazas":[{"pts":rect(-2400,-1000,1400,1000),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.07,0.9,1.1,None),(kit_mesh("SM_MarketStall"),0.04,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Teal"}))]},
               {"pts":rect(-8800,4600,-5600,7600),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_Palm"),0.012,0.5,0.7,None)]}],
     "exclusions":[(-7200,6100,1700,1600),(2600,-9000,500,700)],
     "roads":roads,
     "houses":{"spacing":1300,"setback":850,"scale":(0.85,1.05),"keep":0.92,"entries":houses},
     "house_sets":{"medina":{"spacing":850,"setback":600,"scale":(0.55,0.72),"keep":0.97,"footprint":(-400,-380,480,330),"jitter":40,"entries":houses}},
     "foliage":dict(tree_scale=(0.55,0.8),**base_foliage(TREES_DESERT,forest_ratio=0.012,edge_ratio=0.006,town_ratio=0.002,bush=0.004,grass=0.03,extent=10000,bands={"forest":(0.9,1.0),"edge":(0.8,0.9),"town":(0.3,0.8)},
                            extra=[{"name":"Cacti","band":(0.3,1.0),"ratio":0.012,"entries":[kit_entry("SM_Cactus",None,2),kit_entry("SM_Cactus",{"P_Olive":"P_Green"},1)],"scale":(0.7,1.4)},
                                   {"name":"Rocks","band":(0.6,1.0),"ratio":0.01,"entries":[kit_entry("SM_Boulder",{"P_Grey":"P_Sand","P_DarkGrey":"P_Ochre"})],"scale":(0.6,1.3),"prune":150}])),
     "props":ground_props("Tanchico",20000,MAT("MI_Ground_Sand"))+[
       ("mesh",CUBE,"Sea",(7500,0,-1),0,(54,200,0.06),[WATER]),
       ("mesh",kit_mesh("SM_Keep"),"PanarchPalace",(-7200,6100,0),-90,1.8,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_Gold","P_Slate":"P_Teal"})),
       ("mesh",kit_mesh("SM_DomeHouse"),"GreatDome",(-500,0,0),0,2.2,kit_mats("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Gold","P_Ochre":"P_Teal"})),
       ("mesh",kit_mesh("SM_Obelisk"),"HarbourLight",(3600,-9000,0),0,1.2,kit_mats("SM_Obelisk",{"P_White":"P_Cream"})),
       ("mesh",CHEST,"Chest_Pier",(5200,500,10),0,1.0,None),("mesh",CHEST,"Chest_Souk",(-1800,-700,0),0,1.0,None),
       ("mesh",HORSE,"SoukHorse",(-2200,800,80),-30,1.0,None),("mesh",FIRE,"Fireflies0",(-7000,2500,150),0,1.0,None)],
     "moves":portal_moves(2600,-9000,90),
    }

def illian_spec():
    COB=MAT("MI_Tile_Cobble"); WS=MAT("MI_Tile_WhiteStone")
    houses=rand_house_entries(["MI_Wall_White","MI_Wall_Plaster","MI_Wall_Cream"],["MI_Roof_Green","MI_Roof_Gold","MI_Roof_Slate"])+[
        kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Green","P_DarkWood":"P_Gold"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Green"},2),
        kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Gold"},1)]
    streets=grid_streets(-8500,8500,-8500,8500,7,7,width=5,spacing=1250,setback=850,prefix="st")
    for r in streets: r["lines"]=[lamp_line(1800,330,"P_Gold")]
    canals=[]
    for k,c in enumerate((-6400,-2100,2100,6400)):
        canals.append({"name":f"Canal v{k}","kind":"water","pts":[(c,-9800),(c,0),(c,9800)],"width":4,"houses":False,"mat":WATER,"tile_z":-3,"linear":True,
                       "lines":[{"mesh":kit_mesh("SM_Reeds"),"step":300,"offset":260,"mat":None,"scale":(1,1,1)}]})
    canals.append({"name":"Grand canal","kind":"water","pts":[(-9800,0),(0,0),(9800,0)],"width":6,"houses":False,"mat":WATER,"tile_z":-3,"linear":True,"lines":[{"mesh":kit_mesh("SM_Reeds"),"step":300,"offset":360,"mat":None,"scale":(1,1,1)}]})
    bridges=[]
    for c in (-6400,-2100,2100,6400):
        for sy in (-8500,-4250,0,4250,8500):
            bridges.append(("mesh",kit_mesh("SM_BridgeArch"),f"Bridge_{c}_{sy}",(c,sy,0),0,0.7,kit_mats("SM_BridgeArch",{"P_LightGrey":"P_White","P_White":"P_Gold"})))
    for sx in (-8500,-4250,4250,8500):
        bridges.append(("mesh",kit_mesh("SM_BridgeArch"),f"GBridge_{sx}",(sx,0,0),90,0.9,kit_mats("SM_BridgeArch",{"P_LightGrey":"P_White","P_White":"P_Gold"})))
    return {
     "town":"Illian","level_scale":LEVEL_SIZES["Illian"],"graph_name":"PCG_Illian","folder":"/Game/Wot/Towns/Illian","center":(0,0),"road_mat":COB,
     "plazas":[{"pts":rect(-1500,-1500,1500,-150),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None)]},
               {"pts":rect(-1500,150,1500,1500),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Green"}))]}],
     "exclusions":[(0,-800,1000,600),(9400,0,500,800)],
     "roads":streets+canals,
     "houses":{"spacing":1250,"setback":850,"scale":(0.8,1.0),"keep":0.94,"footprint":(-500,-460,640,370),"entries":houses},
     "foliage":base_foliage(TREES_TEMPERATE,forest_ratio=0.1,edge_ratio=0.02,town_ratio=0.006,bush=0.015,grass=0.26,extent=10000,bands={"forest":(0.94,1.0),"edge":(0.86,0.94),"town":(0.3,0.86)},
                            town_trees=[(MESH["tree3"],2,None),(kit_mesh("SM_Palm"),1,None),(MESH["tree4"],1,None)]),
     "props":ground_props("Illian",20000,MAT("MI_Ground_Lush"))+[
       ("mesh",kit_mesh("SM_Keep"),"KingsPalace",(-200,-800,0),180,1.9,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_Gold","P_Slate":"P_Green"})),
       ("mesh",kit_mesh("SM_StoneWell"),"Fountain",(0,900,0),0,1.6,kit_mats("SM_StoneWell",{"P_Grey":"P_White","P_Terracotta":"P_Green"})),
       ("mesh",CHEST,"Chest_Market",(1200,1200,0),45,1.0,None),("mesh",HORSE,"Horse",(-1200,1300,80),-100,1.0,None),
       ("mesh",FIRE,"Fireflies0",(4000,4000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-4000,-4000,150),0,1.0,None)]+bridges,
     "moves":portal_moves(9400,0,180),
    }

def shadarlogoth_spec():
    ruins=[kit_entry("SM_TowerHouse",{"P_Plaster":"P_DarkGrey","P_Slate":"P_Black","P_DarkWood":"P_Black","P_Glass":"P_Black"},3),
           kit_entry("SM_GableHouse",{"P_Plaster":"P_Grey","P_Terracotta":"P_DarkGrey","P_DarkWood":"P_Black","P_Glass":"P_Black"},2),
           kit_entry("SM_RuinWall",None,4),kit_entry("SM_RuinWall",{"P_DarkGrey":"P_Black"},2),kit_entry("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_Black"},1)]
    alleys=grid_streets(-6000,6000,-6000,6000,7,7,width=3,spacing=900,setback=620,prefix="alley")
    roads=[{"name":"Broken wall","kind":"wall","pts":circle(0,0,6900,40),"closed":True,"width":2,"tile_scale":(1,1,3.0),"tile_z":150,"mat":MAT("MI_Wall_DarkStone"),"keep":0.55,"towers":{"step":2600,"scale":(2.8,2.8,5.0),"z":250,"mat":MAT("MI_Wall_DarkStone")}},
           {"name":"Entry road","pts":[(6900,0),(7400,0)],"width":5,"houses":False}]+alleys
    return {
     "town":"ShadarLogoth","level_scale":LEVEL_SIZES["ShadarLogoth"],"graph_name":"PCG_ShadarLogoth","folder":"/Game/Wot/Towns/ShadarLogoth","center":(0,0),"road_mat":MAT("MI_Tile_PlazaDark"),
     "plazas":[{"pts":rect(-1600,-1600,1600,1600),"mat":MAT("MI_Ground_Ash"),"linear":True,"scatter":[(kit_mesh("SM_RuinWall"),0.04,0.8,1.3,None),(kit_mesh("SM_Boulder"),0.03,0.6,1.2,kit_mats("SM_Boulder",{"P_Grey":"P_DarkGrey","P_DarkGrey":"P_Black"}))]}],
     "exclusions":[(0,0,700,700),(7100,0,500,700)],
     "roads":roads,
     "houses":{"spacing":900,"setback":620,"scale":(0.65,0.95),"keep":0.75,"footprint":(-420,-380,520,330),"jitter":120,"entries":ruins},
     "foliage":base_foliage(TREES_DEAD,forest_ratio=0.12,edge_ratio=0.05,town_ratio=0.012,bush=0.0,grass=0.0,extent=7500,bands={"forest":(0.88,1.0),"edge":(0.6,0.88),"town":(0.2,0.6)},
                            extra=[{"name":"Rubble","band":(0.0,1.0),"ratio":0.03,"entries":[kit_entry("SM_Boulder",{"P_Grey":"P_DarkGrey","P_DarkGrey":"P_Black"},3),kit_entry("SM_RuinWall",None,1)],"scale":(0.5,1.2),"prune":120}]),
     "props":ground_props("ShadarLogoth",15000,MAT("MI_Ground_Ash"))+[
       ("mesh",kit_mesh("SM_Keep"),"RuinedCitadel",(0,0,0),180,1.6,kit_mats("SM_Keep",{"P_DarkGrey":"P_Black","P_Grey":"P_DarkGrey","P_Slate":"P_Black"})),
       ("mesh",kit_mesh("SM_Obelisk"),"CursedSpire",(-1800,1900,0),0,1.1,kit_mats("SM_Obelisk",{"P_White":"P_Black","P_LightGrey":"P_DarkGrey","P_Gold":"P_Rust"})),
       ("mesh",CHEST,"Chest_Treasure",(300,-1200,0),30,1.0,None),("mesh",CHEST,"Chest_Dagger",(-4000,3500,0),130,1.0,None)],
     "moves":portal_moves(7100,0,180),
    }

def whitebridge_spec():
    R=MAT("MI_Roof_Thatch"); R2=MAT("MI_Roof_ThatchPale")
    houses=rand_house_entries(["MI_Wall_Cream","MI_Wall_Tan","MI_Wall_Plaster"],["MI_Roof_Thatch","MI_Roof_ThatchPale"])+[kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Ochre"},2),kit_entry("SM_Longhouse",None,1)]
    return {
     "town":"Whitebridge","level_scale":LEVEL_SIZES["Whitebridge"],"graph_name":"PCG_Whitebridge","folder":"/Game/Wot/Towns/Whitebridge","center":(-1500,0),"road_mat":MAT("MI_Tile_DirtRoad"),
     "plazas":[{"pts":rect(-3300,-1100,-1300,1100),"mat":MAT("MI_Tile_Plaza"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]}],
     "exclusions":[(-2300,0,1100,1200),(-5300,0,500,700),(2200,0,900,6100)],
     "roads":[{"name":"Main street","pts":[(-5800,0),(-3400,0),(-1200,0),(1200,0)],"width":7,"houses":True,"spacing":1300,"setback":900,"lines":[lamp_line(1600,480,"P_Copper")]},
              {"name":"The White Bridge","pts":[(1200,0),(2200,0),(3300,0)],"width":7,"houses":False,"mat":MAT("MI_Tile_WhiteStone"),"tile_z":8},
              {"name":"East road","pts":[(3300,0),(4500,100),(5900,0)],"width":7,"houses":True,"spacing":1300,"setback":900},
              {"name":"River street N","pts":[(600,600),(500,2500),(700,5800)],"width":5,"houses":True,"sides":"left","spacing":1200,"setback":850},
              {"name":"River street S","pts":[(600,-600),(500,-2500),(700,-5800)],"width":5,"houses":True,"sides":"right","spacing":1200,"setback":850},
              {"name":"North lane","pts":[(-3000,1100),(-3100,3000),(-2800,5800)],"width":5,"houses":True,"spacing":1300,"setback":880},
              {"name":"South lane","pts":[(-3000,-1100),(-3100,-3000),(-2800,-5800)],"width":5,"houses":True,"spacing":1300,"setback":880},
              {"name":"Arinelle","kind":"water","pts":[(2200,-6200),(2250,-2000),(2200,2000),(2150,6200)],"width":14,"houses":False,"mat":WATER,"tile_z":-3,"lines":[{"mesh":kit_mesh("SM_Reeds"),"step":280,"offset":760,"mat":None,"scale":(1,1,1)}]},
              {"name":"Dock fence","kind":"wall","pts":[(1400,-5800),(1400,-800)],"width":1,"mesh":MESH["fence"],"mat":None,"tile_step":250,"tile_scale":(1,1,1),"align":True}],
     "houses":{"spacing":1300,"setback":900,"scale":(0.75,0.95),"keep":0.9,"entries":houses},
     "foliage":base_foliage(TREES_TEMPERATE,forest_ratio=0.22,edge_ratio=0.05,town_ratio=0.008,bush=0.02,grass=0.3,mush=0.01,extent=6000),
     "props":ground_props("Whitebridge",12000,GRASSMAT)+[
       ("mesh",kit_mesh("SM_BridgeArch"),"WhiteBridgeArch",(2200,0,0),0,(1.9,1.8,2.2),kit_mats("SM_BridgeArch",{"P_LightGrey":"P_White","P_White":"P_White"})),
       ("mesh",H,"WayfarersRest",(-2300,1500,0),-90,1.3,[MAT("MI_Roof_RedTile"),MAT("MI_Wood_Dark"),MAT("MI_Wall_Plaster"),None,None]),
       ("mesh",kit_mesh("SM_StoneWell"),"Well",(-2300,-500,0),0,1.2,None),("mesh",CHEST,"Chest_Inn",(-1600,2100,0),0,1.0,None),
       ("mesh",HORSE,"InnHorse",(-1500,2600,80),-60,1.0,None),("mesh",FIRE,"Fireflies0",(-2300,0,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(2200,-3000,150),0,1.0,None)],
     "moves":portal_moves(-5300,0,0),
    }

def baerlon_spec():
    houses=rand_house_entries(["MI_Wall_Cream","MI_Wall_Tan","MI_Wall_White"],["MI_Roof_Thatch","MI_Roof_Slate"])+[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Slate"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Ochre"},2),kit_entry("SM_Longhouse",None,1)]
    streets=grid_streets(-3600,3600,-3600,3600,4,4,width=5,spacing=1200,setback=820,prefix="st")
    roads=[{"name":"Palisade","kind":"wall","pts":circle(0,0,4800,32),"closed":True,"width":1,"tile_scale":(1,1,2.6),"tile_z":130,"mat":MAT("MI_Wood_Dark"),"towers":{"step":2400,"scale":(2.2,2.2,4.0),"z":200,"mat":MAT("MI_Wood_Grey")}},
           {"name":"South road","pts":[(0,4800),(0,5900)],"width":7,"houses":False},{"name":"North road","pts":[(0,-4800),(0,-5900)],"width":7,"houses":False}]+streets
    return {
     "town":"Baerlon","level_scale":LEVEL_SIZES["Baerlon"],"graph_name":"PCG_Baerlon","folder":"/Game/Wot/Towns/Baerlon","center":(0,0),"road_mat":MAT("MI_Tile_DirtRoad"),
     "plazas":[{"pts":rect(-1300,-1300,1300,1300),"mat":MAT("MI_Tile_Plaza"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]}],
     "exclusions":[(0,5400,500,700),(-4000,4000,800,800)],
     "roads":roads,
     "houses":{"spacing":1200,"setback":820,"scale":(0.7,0.92),"keep":0.92,"entries":houses},
     "foliage":base_foliage(TREES_PINE()+TREES_TEMPERATE[:2],forest_ratio=0.26,edge_ratio=0.06,town_ratio=0.006,bush=0.02,grass=0.3,mush=0.012,extent=6000,bands={"forest":(0.86,1.0),"edge":(0.78,0.86),"town":(0.25,0.78)},
                            extra=[{"name":"Boulders","band":(0.7,1.0),"ratio":0.02,"entries":[kit_entry("SM_Boulder")],"scale":(0.8,1.5),"prune":200}]),
     "props":ground_props("Baerlon",12000,GRASSMAT)+[
       ("mesh",H,"StagAndLion",(-900,-2200,0),90,1.35,[MAT("MI_Roof_Slate"),MAT("MI_Wood_Dark"),MAT("MI_Wall_Plaster"),None,None]),
       ("mesh",kit_mesh("SM_StoneWell"),"Well",(0,0,0),0,1.3,None),("mesh",kit_mesh("SM_GateHouse"),"SouthGate",(0,4800,0),90,1.1,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkWood","P_LightGrey":"P_Brown"})),
       ("mesh",PEN,"Pen",(-4000,4000,0),0,1.0,None),("mesh",SHEEP,"Sheep0",(-4200,3800,80),0,1.0,None),("mesh",SHEEP,"Sheep1",(-3850,4150,80),90,1.0,None),
       ("mesh",CHEST,"Chest_Inn",(-300,-2700,0),0,1.0,None),("mesh",HORSE,"InnHorse",(-1700,-2500,80),-60,1.0,None),("mesh",FIRE,"Fireflies0",(2500,2500,150),0,1.0,None)],
     "moves":portal_moves(0,5400,-90),
    }

LEVEL_SIZES={"Caemlyn":level_scale(20000),"Cairhien":level_scale(20000),"Tear":level_scale(20000),"TarValon":level_scale(22000),"FalDara":level_scale(15000),"Tanchico":level_scale(20000),"Illian":level_scale(20000),"ShadarLogoth":level_scale(15000),"Whitebridge":level_scale(12000),"Baerlon":level_scale(12000)}
CITY_SPECS={"Caemlyn":caemlyn_spec,"Cairhien":cairhien_spec,"Tear":tear_spec,"TarValon":tarvalon_spec,"FalDara":faldara_spec,"Tanchico":tanchico_spec,"Illian":illian_spec,"ShadarLogoth":shadarlogoth_spec,"Whitebridge":whitebridge_spec,"Baerlon":baerlon_spec}
