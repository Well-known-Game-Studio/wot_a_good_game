# ---------------- Caemlyn, Cairhien, Tear, Tar Valon, Tanchico, Illian ----------------
def caemlyn_spec():
    E=13000
    COB=MAT("MI_Tile_CobbleMid"); WS=MAT("MI_Tile_WhiteStone"); PL=MAT("MI_Tile_Plaza")
    houses=rand_house_entries(["MI_Wall_White","MI_Wall_Plaster"],["MI_Roof_RedTile","MI_Roof_Slate"])+andor_entries()+[
        kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_RedBrick"},2),kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_Slate"},2),
        kit_entry("SM_GableHouse",{"P_Plaster":"P_White","P_Terracotta":"P_RedBrick"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_LightGrey","P_Terracotta":"P_Slate"},1)]
    inner=rand_house_entries(["MI_Wall_White"],["MI_Roof_RedTile","MI_Roof_Gold"],w=2)+stone_town_entries(wall="P_White",roof="P_RedBrick",quoin="P_Gold")+[kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_Gold"},3),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_RedBrick"},2)]
    poor=adobe_entries(wall="P_Brown",trim="P_DarkGrey")+[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_DarkGrey"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Ochre","P_Terracotta":"P_Slate"},2),kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_DarkGrey"},2),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_DarkGrey"},1)]
    stalls=[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None),(kit_mesh("SM_MarketStall"),0.03,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Blue"}))]
    s=town_base("Caemlyn",E,road_mat=COB)
    roads=[
      {"name":"Inner wall","kind":"wall","pts":circle(0,0,4450,40),"closed":True,"width":2,"tile_scale":(1,1,3.6),"tile_z":180,"mat":WS,"towers":{"step":1700,"scale":(2.8,2.8,5.6),"z":280,"mat":MAT("MI_Wall_Stone")}},
      {"name":"Outer wall","kind":"wall","pts":circle(0,0,10750,72),"closed":True,"width":2,"tile_scale":(1,1,3.0),"tile_z":150,"mat":MAT("MI_Wall_RedStone"),"towers":{"step":2200,"scale":(2.6,2.6,4.8),"z":240,"mat":MAT("MI_Wall_DarkStone")}},
      {"name":"Palace ring","pts":circle(0,0,2500,24),"closed":True,"width":5,"mat":WS,"houses":True,"sides":"right","spacing":1700,"setback":950,"set":"inner","lines":[lamp_line(1300,330)]},
      {"name":"Garden ring","pts":circle(0,0,3450,30),"closed":True,"width":5,"houses":False,"lines":[tree_line(900,-380)]},
      {"name":"Inner ring","pts":circle(0,0,4150,36),"closed":True,"width":5,"houses":False},
      {"name":"Middle ring","pts":circle(0,0,6100,40),"closed":True,"width":7,"houses":True,"spacing":1300,"setback":900},
      {"name":"Outer ring","pts":circle(0,0,8200,48),"closed":True,"width":7,"houses":True,"spacing":1300,"setback":900},
      {"name":"Lane ring","pts":circle(0,0,10200,56),"closed":True,"width":5,"houses":True,"sides":"left","spacing":1200,"setback":850},
      {"name":"Low ring","pts":circle(0,0,11800,64),"closed":True,"width":5,"houses":True,"sides":"left","spacing":1000,"setback":700,"set":"low"},
    ]+radial_streets(0,0,2500,12600,12,width=7,spacing=1400,setback=950)
    for r in roads[6:]: r["lines"]=[lamp_line(1400,480)]
    # cramped quarters between the middle and outer rings (NW and SE) and the shanties of Low Caemlyn beyond the wall
    nw=grid_streets(-8700,-4900,-8700,-4900,4,4,width=3,spacing=850,setback=600,set_="alley",prefix="nw alley",wig=80,seed=3)
    se=grid_streets(4900,8700,4900,8700,4,4,width=3,spacing=850,setback=600,set_="alley",prefix="se alley",wig=80,seed=5)
    roads+=nw+se+[{"name":"Market lane","pts":[(6400,-1600),(7600,-1500),(8300,-900)],"width":5,"houses":True,"spacing":1300,"setback":850}]
    s["roads"]=roads
    s["plazas"]=[{"pts":circle(0,0,2200,28),"mat":PL,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None)]},
               {"pts":rect(8600,-2400,10000,-700),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":stalls},
               {"pts":rect(-9600,-3000,-8100,-1400),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":stalls},
               {"pts":circle(5000,5000,1300,16),"mat":GRASS,"scatter":[(MESH["tree3"],0.03,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.05,0.9,1.3,None)]},
               {"pts":circle(-5000,-5000,1300,16),"mat":GRASS,"scatter":[(MESH["tree4"],0.03,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.05,0.9,1.3,None)]}]
    s["exclusions"]=[(0,0,1500,1500),(12500,0,600,800)]
    s["houses"]={"spacing":1300,"setback":900,"scale":(0.8,1.0),"keep":0.94,"footprint":(-660,-380,520,480),"entries":houses}
    s["house_sets"]={"inner":{"spacing":1700,"setback":950,"scale":(0.9,1.15),"keep":0.95,"entries":inner},"alley":dense_set(houses,850,600,(0.55,0.7),0.97),"low":dense_set(poor,1000,700,(0.6,0.8),0.95)}
    s["foliage"]=base_foliage(TREES_TEMPERATE,forest_ratio=0.1,edge_ratio=0.02,town_ratio=0.004,bush=0.012,grass=0.22,extent=E,town_trees=TREES_FORMAL,bands={"forest":(0.95,1.0),"edge":(0.9,0.95),"town":(0.3,0.9)})
    s["pads"]=[(("circle",0,0,2300),None),(("rect",8600,-2400,10000,-700),None),(("rect",-9600,-3000,-8100,-1400),None)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"RoyalPalace",(-600,0,0),180,2.3,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_LightGrey","P_Slate":"P_Gold"})),
       ("mesh",kit_mesh("SM_Obelisk"),"SunSpire_N",(900,-1500,0),0,1.3,None),("mesh",kit_mesh("SM_Obelisk"),"SunSpire_S",(900,1500,0),0,1.3,None),
       ("mesh",kit_mesh("SM_StoneWell"),"PalaceFountain",(1200,0,0),0,1.6,kit_mats("SM_StoneWell",{"P_Grey":"P_White"})),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(10750,0,0),0,1.2,kit_mats("SM_GateHouse",{"P_Grey":"P_RedBrick"})),("mesh",kit_mesh("SM_GateHouse"),"WestGate",(-10750,0,0),0,1.2,kit_mats("SM_GateHouse",{"P_Grey":"P_RedBrick"})),
       ("mesh",kit_mesh("SM_GateHouse"),"InnerGateE",(4450,0,0),0,1.0,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),
       ("mesh",CHEST,"Chest_Market",(9300,-1800,0),90,1.0,None),("mesh",HORSE,"MarketHorse",(9000,-1000,80),-120,1.0,None),
       ("mesh",FIRE,"Fireflies0",(5000,5000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-5000,-5000,150),0,1.0,None),
       ladder("PalaceLadder",0,-2560,-90,600),ladder("InnerLadder",-4460,0,180,600),
       chest("PalaceChest",-1500,1200,0,90),potion("Potion_Palace",1300,-900,0),potion("Potion_Market",9300,-1200,0),chest("HillChest",3600,-2500,0,0),chest("LowChest",11500,3000,0,45),potion("Potion_Low",-11500,-2500,0),
       npc("Guard_Tallanvor","Guardsman Tallanvor","CM_Guard",1700,-1400,120,600),npc("Merchant_Gill","Master Gill","CM_Gill",9300,-1500,-90,800),
       npc("Lady_Elayne","Elayne","CM_Elayne",-1300,2000,-45,700),npc("Boy_Bayle","Bayle Domon","CM_Domon",-8800,-2200,0,1200),npc("Wanderer_Thom","Thom Merrilin","CM_Thom",5000,5200,0,1500),
       npc("Beggar_Low","Low Caemlyn beggar","CM_Beggar",11300,2400,120,900),npc("Alley_Urchin","Street urchin","CM_Urchin",-6800,-6800,30,900)]
    s["dialogue"]={**lines("CM_Guard",["Halt! State your business in the Inner City.","...Very well. Mind the Queen's Guards, and keep off the palace steps."]),
                   **lines("CM_Gill",["The Queen's Blessing has the best room in the New City, if you ask me.","Trouble in the streets lately. Red cloths, white cloths... stay out of it."]),
                   **lines("CM_Elayne",["Mother says a Daughter-Heir should not talk to strangers in the gardens.","But you look like you have seen more of the world than I have. Tell me of it sometime."]),
                   **lines("CM_Domon",["I do be a river man, not a city man. Caemlyn be too far from the water.","If you ever need passage down the Erinin, look for the Spray in Whitebridge."]),
                   **lines("CM_Thom",["A gleeman's tales are worth a coin or two, young friend.","Did you know the topless towers of Cairhien were never finished? Now there is a story."]),
                   **lines("CM_Beggar",["Low Caemlyn, they call it. Outside the walls, outside the law.","The Queen's Guards never come this far. Neither does the Queen's bread."]),
                   **lines("CM_Urchin",["Lost? Everyone gets lost in the alleys. Two coppers and I'll show you the way.","Don't go in the Inner City. They make you wash."])}
    s["props"]+=[hero("Moiraine","Moiraine","CM_Moiraine",-3000,100,0,0,tags=("QuestName=Moiraine",))]
    s["dialogue"].update(lines("CM_Moiraine",["Caemlyn, at last. The Queen's Blessing has been watching for you.","Rest here. The road ahead goes to Tar Valon, and from there to the Eye of the World. But not tonight."]))
    s["moves"]=portal_moves(12500,0,180)
    Rl=Relief(E,base=0,water=None,seed=31)
    Rl.hill(0,0,9000,420); Rl.hill(-9500,-8000,4500,320); Rl.hill(9000,9000,5000,380); Rl.hill(-10000,9000,4000,260)
    Rl.plateau(("circle",0,0,4600),560,0); Rl.plateau(("circle",0,0,3650),760,0); Rl.plateau(("circle",0,0,2650),1000,0)
    Rl.noise(45,900,seed=4); Rl.noise(18,300,seed=8)
    Rl.rock_z=None
    relief_apply(s,Rl,{"grass":GRASS,"rock":ROCK,"sand":SAND,"snow":SNOW})
    return s

def cairhien_spec():
    E=12000; WL=-150
    COB=MAT("MI_Tile_CobbleMid"); WS=MAT("MI_Tile_WhiteStone")
    tall=lambda rc,w: kit_entry("SM_TowerHouse",rc,w)
    houses=stone_town_entries(wall="P_LightGrey",roof="P_Slate",quoin="P_White")+[tall({"P_Plaster":"P_LightGrey","P_Slate":"P_Slate","P_DarkWood":"P_DarkGrey"},3),tall({"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_DarkGrey"},3),
            tall({"P_Plaster":"P_Plaster","P_Slate":"P_DarkGrey","P_DarkWood":"P_DarkGrey"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_LightGrey","P_Terracotta":"P_Slate"},2),
            kit_entry("SM_CourtyardHouse",{"P_Sand":"P_LightGrey","P_Terracotta":"P_Slate"},1)]+rand_house_entries(["MI_Wall_Stone","MI_Wall_White"],["MI_Roof_Slate"],w=1)
    foregate=[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Ochre"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Ochre","P_Terracotta":"P_DarkGrey"},2),kit_entry("SM_MarketStall",None,2),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_Terracotta"},1)]
    s=town_base("Cairhien",E,road_mat=COB)
    roads=grid_streets(-8500,8500,-8500,8500,9,9,width=5,spacing=1150,setback=800)
    for r in roads: r["lines"]=[lamp_line(1150,400,"P_Gold")]
    fg=grid_streets(-7000,7000,9800,11400,8,2,width=3,spacing=800,setback=560,set_="foregate",prefix="foregate",wig=120,seed=7)
    roads+=[{"name":"City wall","kind":"wall","pts":rect(-9200,-9200,9200,9200),"closed":True,"linear":True,"width":2,"tile_scale":(1,1,3.4),"tile_z":170,"mat":MAT("MI_Wall_Stone"),"towers":{"step":2300,"scale":(2.8,2.8,5.6),"z":280,"mat":MAT("MI_Wall_DarkStone")}},
            {"name":"Sun avenue","pts":[(-9800,0),(0,0),(9800,0),(10800,0)],"width":11,"houses":False,"mat":WS,"lines":[tree_line(1000,700,MESH["tree3"]),lamp_line(1000,560)]},
            {"name":"South avenue","pts":[(0,9200),(0,10500),(0,11600)],"width":7,"houses":False},
            {"name":"River quay","pts":[(10900,-9000),(10900,-4000),(10900,4000),(10900,9000)],"width":5,"houses":False,"lines":[{"mesh":MESH["fence"],"step":250,"offset":280,"mat":None,"scale":(1,1,1),"sides":"right"}]}]+fg
    roads+=pier_roads("Pier",11100,(-6000,-2000,2000,6000),1400,WL)
    s["roads"]=roads
    topless=[("mesh",kit_mesh("SM_Obelisk"),f"ToplessTower{k}",(math.cos(math.radians(45+90*k))*3400,math.sin(math.radians(45+90*k))*3400,0),0,3.2,kit_mats("SM_Obelisk",{"P_White":"P_LightGrey","P_Gold":"P_LightGrey"})) for k in range(4)]
    s["plazas"]=[{"pts":rect(-2600,-2600,2600,2600),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_LampPost"),0.008,1,1,None)]},
               {"pts":rect(5200,5200,8000,8000),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]}]
    s["exclusions"]=[(0,0,1900,1900)]+[(math.cos(math.radians(45+90*k))*3400,math.sin(math.radians(45+90*k))*3400,700,700) for k in range(4)]+[(10400,0,500,800)]
    s["houses"]={"spacing":1150,"setback":800,"scale":(0.9,1.2),"keep":0.96,"footprint":(-460,-380,380,380),"jitter":40,"entries":houses}
    s["house_sets"]={"foregate":dense_set(foregate,800,560,(0.5,0.68),0.97)}
    s["foliage"]=base_foliage(TREES_FORMAL+TREES_PINE()[:1],forest_ratio=0.12,edge_ratio=0.03,town_ratio=0.003,bush=0.008,grass=0.18,extent=E,bands={"forest":(0.93,1.0),"edge":(0.88,0.93),"town":(0.3,0.88)},
                              extra=[boulders_extra((0.9,1.0),0.02)])
    s["pads"]=[(("rect",-2700,-2700,2700,2700),None),(("rect",5200,5200,8000,8000),None)]+[(("circle",math.cos(math.radians(45+90*k))*3400,math.sin(math.radians(45+90*k))*3400,750),None) for k in range(4)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"SunPalace",(-700,0,0),180,2.6,kit_mats("SM_Keep",{"P_DarkGrey":"P_LightGrey","P_Grey":"P_White","P_Slate":"P_Gold"})),
       ("mesh",kit_mesh("SM_StoneWell"),"SunFountain",(1500,0,0),0,2.0,kit_mats("SM_StoneWell",{"P_Grey":"P_LightGrey","P_Terracotta":"P_Gold"})),
       ("mesh",kit_mesh("SM_GateHouse"),"JangaiGate",(9200,0,0),0,1.4,None),("mesh",kit_mesh("SM_GateHouse"),"WestGate",(-9200,0,0),0,1.4,None),("mesh",kit_mesh("SM_GateHouse"),"SouthGate",(0,9200,0),90,1.4,None),
       ("mesh",CHEST,"Chest_Market",(6500,6500,0),45,1.0,None),("mesh",HORSE,"Horse",(7000,5600,80),-120,1.0,None),("mesh",CAMP,"ForegateFire",(2500,10500,0),0,1.0,None)]+topless
    s["props"]+=[ladder(f"TowerLadder{k}",math.cos(math.radians(45+90*k))*3400-760*math.cos(math.radians(45+90*k)),math.sin(math.radians(45+90*k))*3400-760*math.sin(math.radians(45+90*k)),45+90*k+180,600) for k in range(4)]
    s["props"]+=[chest("TowerChest0",math.cos(math.radians(45))*3400+200,math.sin(math.radians(45))*3400+200,0,0),potion("Potion_Acro",1800,1800,0),potion("Potion_Market",6500,6900,0),chest("QuayChest",11000,-7000,0,90),potion("Potion_Foregate",-5000,10600,0),
       npc("Noble_Barthanes","Lord Barthanes","CH_Barthanes",1200,-1200,135,500),npc("Beggar_Aldrin","Aldrin","CH_Beggar",-6000,2100,0,900),
       npc("Clerk_Meilan","Meilan","CH_Clerk",2100,2100,-135,600),npc("Guard_Dalresin","Tower Guard","CH_Guard",6400,6400,-90,500),
       npc("Foregate_Juggler","Foregate juggler","CH_Juggler",1500,10500,-90,1200),npc("Bargeman_Alguenya","Riverman","CH_River",10800,-3000,180,800)]
    s["dialogue"]={**lines("CH_Barthanes",["Daes Dae'mar, the Game of Houses. Every word you say here has three meanings.","Choose your friends in Cairhien carefully, outlander. Choose your enemies more carefully still."]),
                   **lines("CH_Beggar",["Spare a coin? The Foregate burned and the nobles never noticed.","They say the King hoards grain in the Sun Palace. I say the towers are hollow."]),
                   **lines("CH_Clerk",["The grid of the city was laid out by Ogier masons. Every street exactly so.","Please do not walk on the grass. It was imported from Andor at great expense."]),
                   **lines("CH_Guard",["The topless towers have stood half-built since the Aiel War.","Climb the ladders if you like. Mind the drop."]),
                   **lines("CH_Juggler",["Welcome to the Foregate! Gleemen, jugglers, and the best fried bread outside Andor.","The nobles inside the walls think we are rabble. We think they are boring."]),
                   **lines("CH_River",["The Alguenya carries grain south to the Erinin and gossip north to the Tower.","Mind the quay stairs. The river is higher than it looks."])}
    s["moves"]=portal_moves(10400,0,180)
    Rl=Relief(E,base=0,water=WL,seed=41)
    Rl.slope(11000,0,-12000,0,0,560)
    peaks(Rl,[(-13000,-12500),(-11500,-7000),(-12800,0),(-11800,7000),(-13000,12500)],4200,1800,n=6,seed=41)
    Rl.hill(-5000,-11000,4000,360); Rl.hill(6000,-11500,3500,300)
    Rl.plateau(("rect",-9300,-9300,9300,9300),240,400)
    Rl.plateau(("rect",-2750,-2750,2750,2750),600,0)
    for k in range(4):
        a=math.radians(45+90*k); Rl.plateau(("circle",math.cos(a)*3400,math.sin(a)*3400,800),840,0)
    Rl.plateau(("rect",-7500,9500,7500,11600),120,300)
    Rl.valley([(12400,-13000),(12300,0),(12400,13000)],2800,620,flat=1200)
    Rl.noise(30,900,seed=2); Rl.noise(12,300,seed=5)
    Rl.rock_z=1500; Rl.snow_z=2100
    relief_apply(s,Rl,{"grass":TUNDRA,"rock":ROCK,"sand":DIRT,"snow":SNOW})
    return s

def tear_spec():
    E=12000; WL=-120
    COB=MAT("MI_Tile_CobbleMid")
    rich=rand_house_entries(["MI_Wall_Stone","MI_Wall_Plaster"],["MI_Roof_Slate","MI_Roof_RedTile"])+stone_town_entries(roof="P_Slate")+[kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_Slate"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_Plaster","P_Terracotta":"P_Slate"},2)]
    poor=[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_DarkGrey"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Ochre","P_Terracotta":"P_Slate"},2),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_DarkGrey"},1)]
    s=town_base("Tear",E,center=(-1500,0),road_mat=COB)
    alleys=grid_streets(1800,5000,-9000,9000,3,11,width=3,spacing=900,setback=620,set_="docks",prefix="alley",wig=70,seed=9)
    hi=grid_streets(-10500,-1200,-10500,10500,6,7,width=7,spacing=1400,setback=950,prefix="ave")
    for r in hi: r["lines"]=[lamp_line(1600,520)]
    roads=[{"name":"Stone wall","kind":"wall","pts":rect(-5200,-2600,-300,2600),"closed":True,"linear":True,"width":2,"tile_scale":(1,1,2.8),"tile_z":140,"mat":MAT("MI_Wall_DarkStone"),"towers":{"step":2000,"scale":(2.6,2.6,4.6),"z":230,"mat":MAT("MI_Wall_Stone")}},
           {"name":"Gate road","pts":[(-300,0),(900,0),(1800,0)],"width":7,"houses":False},
           {"name":"Riverside avenue","pts":[(5300,-11500),(5300,-4000),(5300,0),(5300,4000),(5300,11500)],"width":7,"houses":True,"sides":"left","spacing":1100,"setback":700,"set":"docks","lines":[{"mesh":kit_mesh("SM_Reeds"),"step":260,"offset":-430,"mat":None,"scale":(1,1,1),"sides":"right"}]},
           {"name":"Dock fence","kind":"wall","pts":[(5700,-11400),(5700,11400)],"width":1,"mesh":MESH["fence"],"mat":None,"tile_step":250,"tile_scale":(1,1,1),"align":True}]
    roads+=pier_roads("Pier",5400,(-6500,-3300,-900,1800,4400,7200),1800,WL)
    roads+=alleys+hi
    s["roads"]=roads
    s["plazas"]=[{"pts":rect(-200,-1200,1700,1200),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.04,0.9,1.1,None)]+market_extras(0.015)},
               {"pts":rect(2200,-9000,4800,-6800),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.06,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Olive"}))]}]
    s["exclusions"]=[(-2750,0,2600,2600),(8000,0,2600,12000),(5300,-11000,500,700)]
    s["houses"]={"spacing":1400,"setback":950,"scale":(0.85,1.05),"keep":0.94,"footprint":(-660,-380,520,480),"entries":rich}
    s["house_sets"]={"docks":dense_set(poor,900,620,(0.55,0.72),0.95)}
    s["foliage"]=base_foliage(TREES_TEMPERATE,forest_ratio=0.08,edge_ratio=0.02,town_ratio=0.004,bush=0.012,grass=0.22,extent=E,bands={"forest":(0.95,1.0),"edge":(0.88,0.95),"town":(0.3,0.88)},
                              extra=[{"name":"Reeds","band":(0.0,1.0),"ratio":0.0,"entries":[],"scale":(1,1)}])
    s["pads"]=[(("rect",-200,-1200,1700,1200),None),(("rect",2200,-9000,4800,-6800),None)]
    s["props"]=[("mesh",MESH["stone"],"StoneOfTear",(-2750,0,0),0,20.0,None),("mesh",MESH["tower"],"StoneKeep",(-2750,0,2900),0,5.0,None),
       ("mesh",kit_mesh("SM_GateHouse"),"StoneGate",(-300,0,0),0,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey"})),
       ("mesh",CHEST,"Chest_Pier",(6900,-900,10),0,1.0,None,"abs"),("mesh",CHEST,"Chest_Market",(3500,-8000,0),90,1.0,None),
       ("mesh",CAMP,"DockFire",(4600,2600,0),0,1.0,None),("mesh",HORSE,"DockHorse",(2900,-7700,80),-30,1.0,None),
       ("mesh",FIRE,"Fireflies0",(5000,3000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-6000,-4000,150),0,1.0,None),
       ladder("StoneLadderN",-2750,-2660,-90,900),ladder("StoneLadderS",-2750,2660,90,900),
       chest("StoneChest",-4800,2200,0,0),potion("Potion_Stone",-1000,-2000,0),potion("Potion_Docks",4800,5200,0),chest("DockChest",6900,4400,10,90),potion("Potion_Hill",-9500,-9500,0),
       npc("Defender_Weiramon","Defender Weiramon","TR_Defender",-900,900,-135,500),npc("Docker_Nalesean","Nalesean","TR_Docker",4200,-1200,90,900),
       npc("Fisher_Alanna","Mistress Alanna","TR_Fisher",4800,5200,-90,900),npc("Lord_Darlin","Lord Darlin","TR_Darlin",-6500,-4300,45,600),npc("Maule_Child","Maule child","TR_Child",3000,6500,0,1200)]
    s["dialogue"]={**lines("TR_Defender",["The Stone of Tear has never fallen. Not once in three thousand years.","Keep your hands where I can see them, outlander."]),
                   **lines("TR_Docker",["The Erinin brings the grain and the Erinin takes the coin.","Watch your step on the piers, the planks are rotten and the river is deep."]),
                   **lines("TR_Fisher",["Fresh eels! Caught this morning below the Stone.","No channeling in Tear, mind. The Defenders do not take kindly to it."]),
                   **lines("TR_Darlin",["The Maule stinks of fish and the High Lords stink of politics.","If you have a strong arm, there is always work with the Defenders."]),
                   **lines("TR_Child",["The Maule floods every spring. We put the chickens on the roof.","Up on the hill the High Lords have gardens. Down here we have mud."])}
    s["moves"]=portal_moves(5300,-11000,90)
    Rl=Relief(E,base=0,water=WL,seed=51)
    Rl.slope(6000,0,-12000,0,0,380); Rl.hill(-9000,-3000,7000,220)
    Rl.plateau(("rect",-5300,-2700,-200,2700),900,0)
    Rl.plateau(("rect",1500,-12000,5600,12000),0,250)
    Rl.valley([(8500,-13000),(8600,0),(8500,13000)],5600,700,flat=2600)
    Rl.valley([(-12000,7500),(-7000,8200),(-2000,8800),(1500,9200)],900,300,flat=200)
    Rl.noise(30,900,seed=3); Rl.noise(12,300,seed=4)
    Rl.sand_h=100
    relief_apply(s,Rl,{"grass":GRASS,"rock":DARKROCK,"sand":DIRT,"snow":SNOW})
    return s

def tarvalon_spec():
    E=13500; WL=-200
    WS=MAT("MI_Tile_WhiteStone")
    white=stone_town_entries(wall="P_White",roof="P_Blue",quoin="P_Gold")+[kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_Gold"},3),kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Blue","P_Ochre":"P_Gold"},3),
           kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Gold","P_Ochre":"P_LightGrey"},1),kit_entry("SM_GableHouse",{"P_Plaster":"P_White","P_Terracotta":"P_Slate","P_DarkWood":"P_Gold"},2),
           kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Blue"},2)]+rand_house_entries(["MI_Wall_TarValon"],["MI_Roof_Blue","MI_Roof_Gold"],woods=("MI_Trim_Gold",))
    tight=[kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Blue","P_DarkWood":"P_Gold"},3),kit_entry("SM_TowerHouse",{"P_Plaster":"P_Cream","P_Slate":"P_Slate","P_DarkWood":"P_Gold"},2),kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Blue","P_Ochre":"P_Gold"},2)]
    s=town_base("TarValon",E,road_mat=WS)
    radials=radial_streets(0,0,2700,8900,12,width=5,houses=True,spacing=1400,setback=900)
    for r in radials: r["lines"]=[lamp_line(1500,520,"P_Gold")]
    posts={"step":500,"scale":(1.2,1.2,2.6),"z":-130,"mat":MAT("MI_Wall_Stone")}
    bridges=[{"name":f"Bridge {k}","pts":[(math.cos(th)*8900,math.sin(th)*8900,6),(math.cos(th)*10100,math.sin(th)*10100,6),(math.cos(th)*11400,math.sin(th)*11400,6)],"width":5,"houses":False,"mat":WS,"tile_z":0,"towers":posts,"nocarve":True,"linear":True} for k,th in enumerate([0,math.pi/2,math.pi,3*math.pi/2])]
    parks=[{"pts":circle(math.cos(th)*5600,math.sin(th)*5600,1100,14),"mat":GRASS,"scatter":[(kit_mesh("SM_Birch"),0.04,0.9,1.2,None),(kit_mesh("SM_FlowerShrub"),0.06,0.9,1.3,None)]} for th in [math.pi/8+k*math.pi/2 for k in range(4)]]
    q1=grid_streets(3200,7800,-7800,-3200,4,4,width=3,spacing=850,setback=600,set_="tight",prefix="ne alley",wig=60,seed=13)
    q2=grid_streets(-7800,-3200,3200,7800,4,4,width=3,spacing=850,setback=600,set_="tight",prefix="sw alley",wig=60,seed=17)
    s["roads"]=[{"name":"Tower ring","pts":circle(0,0,2700,32),"closed":True,"width":5,"houses":False,"lines":[tree_line(900,-380,MESH["tree3"])]},
              {"name":"Inner ring","pts":circle(0,0,4500,36),"closed":True,"width":5,"houses":True,"sides":"both","spacing":1500,"setback":900},
              {"name":"Outer ring","pts":circle(0,0,6300,44),"closed":True,"width":5,"houses":True,"sides":"both","spacing":1400,"setback":900},
              {"name":"Shore ring","pts":circle(0,0,8200,56),"closed":True,"width":5,"houses":True,"sides":"left","spacing":1400,"setback":900},
              {"name":"East bank road","pts":[(11400,0),(12400,0),(13200,0)],"width":5,"houses":False},
              {"name":"West bank road","pts":[(-11400,0),(-12400,0)],"width":5,"houses":False}]+radials+bridges+q1+q2
    s["plazas"]=[{"pts":circle(0,0,2400,32),"mat":WS,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None),(kit_mesh("SM_Obelisk"),0.004,0.6,0.8,None)]}]+parks
    s["exclusions"]=[(0,0,1500,1500)]+[(math.cos(th)*9400,math.sin(th)*9400,500,700) for th in [0,math.pi/2,math.pi,3*math.pi/2]]+[(12700,0,500,700)]
    s["houses"]={"spacing":1400,"setback":900,"scale":(0.8,1.0),"keep":0.93,"footprint":(-660,-380,520,480),"entries":white}
    s["house_sets"]={"tight":dense_set(tight,850,600,(0.55,0.72),0.97)}
    s["foliage"]=base_foliage(TREES_FORMAL+TREES_PINE()[:1],forest_ratio=0.1,edge_ratio=0.03,town_ratio=0.008,bush=0.018,grass=0.28,extent=E,fr=12000,clip=1.0,bands={"forest":(0.9,1.0),"edge":(0.78,0.9),"town":(0.3,0.78)},
                              extra=[boulders_extra((0.85,1.0),0.03,{"P_Grey":"P_DarkGrey"})])
    s["pads"]=[(("circle",0,0,2500),None)]+[(("circle",math.cos(th)*5600,math.sin(th)*5600,1200),None) for th in [math.pi/8+k*math.pi/2 for k in range(4)]]
    s["props"]=[("mesh",MESH["tower"],"WhiteTower",(0,0,0),0,24.0,None),
       ("mesh",kit_mesh("SM_Obelisk"),"Spire_NE",(1600,-1600,0),0,1.6,None),("mesh",kit_mesh("SM_Obelisk"),"Spire_SW",(-1600,1600,0),0,1.6,None),
       ("mesh",kit_mesh("SM_StoneWell"),"Fountain_N",(0,1800,0),0,1.8,kit_mats("SM_StoneWell",{"P_Grey":"P_White","P_Terracotta":"P_Blue"})),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(8300,0,0),0,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),("mesh",kit_mesh("SM_GateHouse"),"WestGate",(-8300,0,0),0,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),
       ("mesh",kit_mesh("SM_GateHouse"),"NorthGate",(0,-8300,0),90,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),("mesh",kit_mesh("SM_GateHouse"),"SouthGate",(0,8300,0),90,1.3,kit_mats("SM_GateHouse",{"P_Grey":"P_White","P_LightGrey":"P_Gold"})),
       ("mesh",CHEST,"Chest_Bridge",(7700,500,0),0,1.0,None),("mesh",HORSE,"BridgeHorse",(7400,900,80),200,1.0,None),
       ("mesh",FIRE,"Fireflies0",(0,-1200,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-5400,5400,150),0,1.0,None),
       ladder("TowerLadder",0,-2760,-90,600),ladder("TerraceLadder",-5160,0,180,600),
       chest("TowerChest",-1500,1500,0,45),potion("Potion_Tower",1500,-1500,0),potion("Potion_Park",5200,2100,0),chest("AlleyChest",5500,-5500,0,30),potion("Potion_Alley",-5500,5500,0),chest("BankChest",12800,600,0,90),
       npc("Novice_Else","Novice Else","TV_Novice",1200,1200,-135,600),npc("Warder_Gaidin","Warder","TV_Warder",2900,0,180,400),
       npc("Ogier_Loial","Loial","TV_Loial",-5000,5000,45,1200),npc("Trader_Hob","Master Hob","TV_Hob",7400,-800,90,800),npc("Alley_Seamstress","Seamstress","TV_Seam",5400,-5400,45,800),npc("Bank_Farmer","Farmer","TV_Farmer",12300,-500,0,900)]
    s["dialogue"]={**lines("TV_Novice",["Novices are not supposed to talk to visitors. Please do not tell the Mistress of Novices.","The Tower has a hundred levels and I have scrubbed every one of them."]),
                   **lines("TV_Warder",["The Tower grounds are closed to the public. Admire it from the steps.","An Aes Sedai is not to be disturbed. Move along."]),
                   **lines("TV_Loial",["Ogier built this city, you know. Every bridge, every street. Hasty work, only three hundred years.","I would very much like to see a stedding again. The trees here are so young."]),
                   **lines("TV_Hob",["Four bridges in and four bridges out, and a toll on every one of them.","Silk from Arafel, ice peppers from Shienar. Have a look, friend."]),
                   **lines("TV_Seam",["The old quarter, dear. Houses leaning on houses, and every roof a different colour.","An Aes Sedai ordered a shawl with a blue fringe. I have not slept in three days."]),
                   **lines("TV_Farmer",["That mountain is Dragonmount. Folk say it still smokes some nights.","Nobody farms on its slopes. Nobody sensible, anyway."])}
    s["moves"]=portal_moves(12700,0,180)
    Rl=Relief(E,base=-700,water=WL,seed=61)
    Rl.plateau(("circle",0,0,9000),0,500)
    Rl.plateau(("rect",11300,-15000,15000,15000),0,500,"max"); Rl.plateau(("rect",-15000,-15000,-11300,15000),0,500,"max")
    Rl.plateau(("rect",-15000,11300,15000,15000),0,500,"max"); Rl.plateau(("rect",-15000,-15000,15000,-11300),0,500,"max")
    Rl.hill(-17000,-2500,7800,4800); Rl.hill(-15500,-5500,3200,1200); Rl.hill(-14000,9000,3600,700); Rl.hill(14000,-9000,3600,600)
    Rl.plateau(("circle",0,0,5300),320,0); Rl.plateau(("circle",0,0,2800),720,0)
    Rl.noise(30,900,seed=1); Rl.noise(14,300,seed=2)
    Rl.rock_z=900; Rl.snow_z=2500
    relief_apply(s,Rl,{"grass":GRASS,"rock":ROCK,"sand":SAND,"snow":SNOW})
    return s

def tanchico_spec():
    E=12000; WL=-150
    SANDT=MAT("MI_Tile_Plaza"); WS=MAT("MI_Tile_WhiteStone")
    houses=adobe_entries()+[kit_entry("SM_DomeHouse",{"P_Cream":"P_Cream","P_Teal":"P_Teal","P_Ochre":"P_Ochre"},3),kit_entry("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Copper","P_Ochre":"P_Terracotta"},2),
            kit_entry("SM_DomeHouse",{"P_Cream":"P_Sand","P_Teal":"P_Blue","P_Ochre":"P_White"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_Sand","P_Terracotta":"P_Terracotta"},2),
            kit_entry("SM_TowerHouse",{"P_Plaster":"P_Cream","P_Slate":"P_Copper","P_DarkWood":"P_Teal"},2),kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Teal"},1)]
    s=town_base("Tanchico",E,center=(-3500,0),road_mat=SANDT)
    medina=grid_streets(-9500,1500,-9500,-1500,7,7,width=3,spacing=850,setback=600,set_="medina",prefix="alley",wig=120,seed=19)
    grand=grid_streets(-9500,1500,1500,9500,3,3,width=7,spacing=1700,setback=1050,prefix="ave")
    for r in grand: r["lines"]=[{"mesh":kit_mesh("SM_Palm"),"step":2600,"offset":560,"mat":None,"scale":(0.6,0.6,0.6)},lamp_line(1700,380,"P_Copper")]
    hillside=grid_streets(-9500,-3500,1500,9500,4,4,width=3,spacing=850,setback=600,set_="medina",prefix="verana",wig=120,seed=23)
    roads=[{"name":"Harbour road","pts":[(2600,-11500),(2600,-4000),(2600,0),(2600,4000),(2600,11500)],"width":7,"houses":True,"sides":"left","spacing":1300,"setback":850,"lines":[{"mesh":kit_mesh("SM_Palm"),"step":2200,"offset":-460,"mat":None,"scale":(0.6,0.6,0.6),"sides":"right"}]},
           {"name":"Sea wall","kind":"wall","pts":[(3300,-11400),(3300,11400)],"width":1,"tile_scale":(1,1,1.2),"tile_z":60,"mat":WS}]
    roads+=pier_roads("Pier",3100,(-7000,-3500,500,4500,8000),2500,WL)
    roads+=medina+grand+hillside
    s["roads"]=roads
    s["plazas"]=[{"pts":rect(-2400,-1000,1400,1000),"mat":WS,"linear":True,"scatter":market_extras(0.015)+[(kit_mesh("SM_MarketStall"),0.07,0.9,1.1,None),(kit_mesh("SM_MarketStall"),0.04,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Teal"}))]},
               {"pts":rect(-8800,4600,-5600,7600),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_Palm"),0.012,0.5,0.7,None)]}]
    s["exclusions"]=[(-7200,6100,1700,1600),(2600,-11000,500,700)]
    s["houses"]={"spacing":1300,"setback":850,"scale":(0.85,1.05),"keep":0.92,"entries":houses}
    s["house_sets"]={"medina":dense_set(houses,850,600,(0.55,0.72),0.97)}
    s["foliage"]=dict(tree_scale=(0.55,0.8),**base_foliage(TREES_DESERT,forest_ratio=0.012,edge_ratio=0.006,town_ratio=0.002,bush=0.004,grass=0.03,extent=E,bands={"forest":(0.9,1.0),"edge":(0.8,0.9),"town":(0.3,0.8)},
                            extra=[{"name":"Cacti","band":(0.3,1.0),"ratio":0.012,"entries":[kit_entry("SM_Cactus",None,2),kit_entry("SM_Cactus",{"P_LeafOlive":"P_Green"},1)],"scale":(0.7,1.4)},
                                   {"name":"Rocks","band":(0.6,1.0),"ratio":0.01,"entries":[kit_entry("SM_Boulder",{"P_Grey":"P_Sand","P_DarkGrey":"P_Ochre"})],"scale":(0.6,1.3),"prune":150}]))
    s["pads"]=[(("rect",-2500,-1100,1500,1100),None),(("rect",-8900,4500,-5500,7700),None)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"PanarchPalace",(-7200,6100,0),-90,1.8,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_Gold","P_Slate":"P_Teal"})),
       ("mesh",kit_mesh("SM_DomeHouse"),"GreatDome",(-500,0,0),0,2.2,kit_mats("SM_DomeHouse",{"P_Cream":"P_White","P_Teal":"P_Gold","P_Ochre":"P_Teal"})),
       ("mesh",kit_mesh("SM_Obelisk"),"HarbourLight",(3600,-11000,0),0,1.2,kit_mats("SM_Obelisk",{"P_White":"P_Cream"})),
       ("mesh",CHEST,"Chest_Pier",(5200,500,10),0,1.0,None,"abs"),("mesh",CHEST,"Chest_Souk",(-1800,-700,0),0,1.0,None),
       ("mesh",HORSE,"SoukHorse",(-2200,800,80),-30,1.0,None),("mesh",FIRE,"Fireflies0",(-7000,2500,150),0,1.0,None),
       ladder("CliffLadder0",-9300,-9300,45,600),ladder("PalaceLadder",-7200,6100-2360,-90,600),
       chest("PalaceChest",-6500,5200,0,90),potion("Potion_Medina",-5500,-5000,0),potion("Potion_Souk",-500,600,0),chest("PierChest",5100,4600,10,0),potion("Potion_Hill",-8500,-8500,0),
       npc("Panarch_Guard","Panarch's Guard","TC_Guard",-5800,4600,-90,500),npc("Souk_Trader","Souk Trader","TC_Trader",-600,-400,45,700),
       npc("Sailor_Bayle","Sailor","TC_Sailor",2200,2000,180,900),npc("Child_Seluca","Seluca","TC_Child",-6200,-4200,0,1200),npc("Medina_Dyer","Dyer","TC_Dyer",-4000,-7000,90,900)]
    s["dialogue"]={**lines("TC_Guard",["The Panarch's Palace is closed. The Panarch's Palace is always closed.","Tanchico has three hills and three kinds of trouble. Pick one."]),
                   **lines("TC_Trader",["Silk, dyes, honey from the Mountains of Mist! Best prices in the Souk.","Mind the stairs down to the harbour. The steps are older than the city."]),
                   **lines("TC_Sailor",["Ships from Illian, ships from the Sea Folk, ships from places with no names.","The Great Dome? The Panarch's folk say it is older than the Breaking."]),
                   **lines("TC_Child",["Race you to the top of the medina! Last one up is a Whitecloak.","I know a secret way down the cliff. It's a ladder. Don't tell."]),
                   **lines("TC_Dyer",["Blue from the Sea Folk, red from the hills. My hands have not been clean in twenty years.","The alleys up here were never planned. Every house just leans on the next."])}
    s["moves"]=portal_moves(2600,-11000,90)
    Rl=Relief(E,base=-600,water=WL,seed=71)
    Rl.plateau(("rect",-13000,-13000,3300,13000),0,700)
    Rl.hill(-7000,-6000,5200,760); Rl.hill(-6800,5500,5200,860); Rl.hill(-1000,0,4200,300); Rl.hill(-11000,-500,3500,500)
    Rl.plateau(("circle",-7200,6100,2300),1100,0)
    Rl.plateau(("rect",-2500,-1100,1500,1100),280,0)
    Rl.noise(40,800,seed=3); Rl.noise(16,300,seed=5)
    Rl.rock_z=None; Rl.sand_h=120
    relief_apply(s,Rl,{"grass":SAND,"rock":REDROCK,"sand":SAND,"snow":SNOW})
    return s

def illian_spec():
    E=11000; WL=-120
    COB=MAT("MI_Tile_CobbleMid"); WS=MAT("MI_Tile_WhiteStone")
    houses=rand_house_entries(["MI_Wall_White","MI_Wall_Plaster","MI_Wall_Cream"],["MI_Roof_Green","MI_Roof_Gold","MI_Roof_Slate"])+stone_town_entries(wall="P_White",roof="P_Green",quoin="P_Gold")+[kit_entry("SM_Shop",{"P_Terracotta":"P_Green","P_Cream":"P_Gold","P_Slate":"P_Green"},1)]+[
        kit_entry("SM_TowerHouse",{"P_Plaster":"P_White","P_Slate":"P_Green","P_DarkWood":"P_Gold"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Green"},2),
        kit_entry("SM_CourtyardHouse",{"P_Sand":"P_White","P_Terracotta":"P_Gold"},1)]
    bridge_=[kit_entry("SM_TowerHouse",{"P_Plaster":"P_Plaster","P_Slate":"P_DarkGrey","P_DarkWood":"P_DarkWood"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Green"},2)]
    s=town_base("Illian",E,road_mat=COB)
    streets=grid_streets(-8500,8500,-8500,8500,7,7,width=5,spacing=1250,setback=850,prefix="st")
    for r in streets: r["lines"]=[lamp_line(1800,330,"P_Gold")]
    perfumed=grid_streets(-8500,-4500,-8500,-4500,4,4,width=3,spacing=850,setback=600,set_="quarter",prefix="pq alley",wig=70,seed=29)
    canals=[]
    for k,c in enumerate((-6400,-2100,2100,6400)):
        canals.append({"name":f"Canal v{k}","kind":"water","pts":[(c,-9800),(c,0),(c,10600)],"width":4,"houses":False,"mat":WATER,"tile_z":-3,"linear":True,"depth":140,
                       "lines":[{"mesh":kit_mesh("SM_Reeds"),"step":300,"offset":260,"mat":None,"scale":(1,1,1)}]})
    canals.append({"name":"Grand canal","kind":"water","pts":[(-9800,0),(0,0),(9800,0)],"width":6,"houses":False,"mat":WATER,"tile_z":-3,"linear":True,"depth":160,"lines":[{"mesh":kit_mesh("SM_Reeds"),"step":300,"offset":360,"mat":None,"scale":(1,1,1)}]})
    harbour=[{"name":"Harbour quay","pts":[(-9000,9000),(0,9000),(9000,9000)],"width":5,"houses":False,"lines":[{"mesh":MESH["fence"],"step":250,"offset":-280,"mat":None,"scale":(1,1,1),"sides":"right"}]}]
    harbour+=pier_roads("Pier",9200,(-7000,-3500,0,3500,7000),1600,WL,along_x=False)
    bridges=[]
    for c in (-6400,-2100,2100,6400):
        for sy in (-8500,-4250,0,4250,8500):
            bridges.append(("mesh",kit_mesh("SM_BridgeArch"),f"Bridge_{c}_{sy}",(c,sy,0),0,0.7,kit_mats("SM_BridgeArch",{"P_LightGrey":"P_White","P_White":"P_Gold"})))
    for sx in (-8500,-4250,4250,8500):
        bridges.append(("mesh",kit_mesh("SM_BridgeArch"),f"GBridge_{sx}",(sx,0,0),90,0.9,kit_mats("SM_BridgeArch",{"P_LightGrey":"P_White","P_White":"P_Gold"})))
    s["roads"]=streets+perfumed+canals+harbour
    s["plazas"]=[{"pts":rect(-1500,-1500,1500,-150),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_LampPost"),0.01,1,1,None)]},
               {"pts":rect(-1500,150,1500,1500),"mat":WS,"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_Green"}))]}]
    s["exclusions"]=[(0,-800,1000,600),(10400,0,500,800)]
    s["houses"]={"spacing":1250,"setback":850,"scale":(0.8,1.0),"keep":0.94,"footprint":(-640,-370,500,460),"entries":houses}
    s["house_sets"]={"quarter":dense_set(bridge_,850,600,(0.55,0.72),0.97)}
    s["foliage"]=base_foliage(TREES_TEMPERATE,forest_ratio=0.1,edge_ratio=0.02,town_ratio=0.006,bush=0.015,grass=0.26,extent=E,bands={"forest":(0.95,1.0),"edge":(0.88,0.95),"town":(0.3,0.88)},
                              town_trees=[(MESH["tree3"],2,None),(kit_mesh("SM_Palm"),1,None),(MESH["tree4"],1,None)],
                              extra=[{"name":"Reeds","band":(0.0,1.0),"ratio":0.0,"entries":[],"scale":(1,1)}])
    s["pads"]=[(("rect",-1600,-1600,1600,-100),None)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"KingsPalace",(-200,-800,0),180,1.9,kit_mats("SM_Keep",{"P_DarkGrey":"P_White","P_Grey":"P_Gold","P_Slate":"P_Green"})),
       ("mesh",kit_mesh("SM_StoneWell"),"Fountain",(0,900,0),0,1.6,kit_mats("SM_StoneWell",{"P_Grey":"P_White","P_Terracotta":"P_Green"})),
       ("mesh",kit_mesh("SM_Obelisk"),"HarbourLight",(9600,9300,0),0,1.3,kit_mats("SM_Obelisk",{"P_White":"P_White","P_Gold":"P_Green"})),
       ("mesh",CHEST,"Chest_Market",(1200,1200,0),45,1.0,None),("mesh",HORSE,"Horse",(-1200,1300,80),-100,1.0,None),
       ("mesh",FIRE,"Fireflies0",(4000,4000,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(-4000,-4000,150),0,1.0,None),("mesh",CAMP,"QuayFire",(2000,9300,0),0,1.0,None),
       ladder("PalaceLadder",-1560,-800,180,600),
       chest("PalaceChest",-1200,-1200,0,45),potion("Potion_Palace",1200,-1200,0),potion("Potion_Canal",6400,3000,0),chest("QuayChest",-6000,9300,0,90),potion("Potion_Hill",9000,-9000,0),chest("QuarterChest",-6500,-6500,0,0)]+bridges
    s["props"]+=[npc("Councilor_Crane","Lord Crane","IL_Crane",900,-900,-135,500),npc("Boatman_Dilham","Dilham","IL_Boat",-6000,3000,0,900),
       npc("Hunter_Horn","Hunter of the Horn","IL_Hunter",5000,-5000,135,1200),npc("Flower_Girl","Nieda","IL_Nieda",2100,2100,-135,800),npc("Bridge_Docker","Harbour docker","IL_Docker",3000,8800,-90,900),npc("Perfumed_Lady","Lady of the Perfumed Quarter","IL_Lady",-6500,-6500,45,700)]
    s["dialogue"]={**lines("IL_Crane",["The Council of Nine do say the King rules Illian. The King do say otherwise.","Mind the canals after dark. The water be deeper than it looks."]),
                   **lines("IL_Boat",["A boat ride through the canals, two coppers. Bridges be free.","The reeds grow thick this year. Good for baskets, bad for boats."]),
                   **lines("IL_Hunter",["I have sworn the oath of the Hunters of the Horn. Have you heard it sounded?","Every road out of Illian leads to the Horn of Valere. I just do not know which."]),
                   **lines("IL_Nieda",["Flowers for your lady? The ones from the marsh never wilt.","The Bees of Illian, they call us. Busy, busy, and the honey all goes to the Nine."]),
                   **lines("IL_Docker",["The Sea of Storms do bring the ships and the ships do bring the coin.","Fish, olives, and lacquerware. Nothing leaves this harbour without a tally."]),
                   **lines("IL_Lady",["The Perfumed Quarter, dear. It do not smell of perfume. It never did.","Every alley here ends at a canal. Learn to swim or learn to ask directions."])}
    s["moves"]=portal_moves(10400,0,180)
    Rl=Relief(E,base=0,water=WL,seed=81)
    Rl.plateau(("rect",-13000,9400,13000,13000),-600,900)
    Rl.hill(-8500,-9500,5000,340); Rl.hill(8500,-9500,5500,420); Rl.hill(0,-11000,4000,260)
    Rl.plateau(("rect",-1600,-1600,1600,-100),440,0)
    Rl.noise(28,900,seed=4); Rl.noise(12,300,seed=6)
    Rl.sand_h=90
    relief_apply(s,Rl,{"grass":GRASS,"rock":ROCK,"sand":SAND,"snow":SNOW})
    return s

CITY_SPECS={"EmondsField":emondsfield_spec,"Baerlon":baerlon_spec,"Whitebridge":whitebridge_spec,"ShadarLogoth":shadarlogoth_spec,"FalDara":faldara_spec,
            "Caemlyn":caemlyn_spec,"Cairhien":cairhien_spec,"Tear":tear_spec,"TarValon":tarvalon_spec,"Tanchico":tanchico_spec,"Illian":illian_spec}
