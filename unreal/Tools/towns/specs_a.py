# ---------------- Emond's Field, Baerlon, Whitebridge, Shadar Logoth, Fal Dara ----------------
def emondsfield_spec():
    E=7000
    W=MAT("MI_Wall_Cream"); W2=MAT("MI_Wall_Tan"); W3=MAT("MI_Wall_Plaster"); W4=MAT("MI_Wall_White")
    R=MAT("MI_Roof_Thatch"); R2=MAT("MI_Roof_ThatchPale"); WD=MAT("MI_Wood_Dark"); WG=MAT("MI_Wood_Grey")
    s=town_base("EmondsField",E,road_mat=MAT("MI_Tile_DirtRoad")); s["graph_name"]="PCG_EmondsField"
    inn_yaw=math.degrees(math.atan2(-1250,-1750))+180
    s["plazas"]=[{"pts":circle(650,-450,330,12),"mat":MAT("MI_Tile_Plaza"),"scatter":market_extras(0.08)},{"pts":circle(3100,2500,520,14),"mat":WATER,"z":560}]
    s["exclusions"]=[(0,0,380,380),(1750,1250,1150,950),(650,-450,380,380),(-3000,2400,750,750),(3100,2500,760,760),(-5000,-200,650,700),(-3900,-4100,500,500),(3750,-4350,720,620),(2750,850,560,460)]
    s["roads"]=[
       {"name":"Green ring","pts":circle(0,0,1250,20),"closed":True,"width":5,"houses":True,"sides":"right","spacing":1700,"setback":820,"lines":[fence_line(300,400,"left","SM_Hedge"),lantern_line(1500,300,"left")]},
       {"name":"West road","pts":wiggle([(-1250,0),(-2600,-350),(-3900,-100),(-5300,-200)],260,3,2),"width":7,"houses":True,"spacing":1500,"lines":[fence_line(300,430,"both")]},
       {"name":"East road","pts":wiggle([(1250,0),(2600,300),(3900,100),(5400,350),(6800,200)],260,4,2),"width":7,"houses":True,"spacing":1500,"lines":[fence_line(300,430,"both")]},
       {"name":"North road","pts":wiggle([(0,1250),(300,2600),(-200,3900),(200,5300),(100,6800)],260,5,2),"width":7,"houses":True,"spacing":1500,"lines":[fence_line(300,430,"both")]},
       {"name":"South road","pts":wiggle([(0,-1250),(-300,-2600),(200,-3900),(-100,-5300),(0,-6800)],260,6,2),"width":7,"houses":True,"spacing":1500,"lines":[fence_line(300,430,"both")]},
       {"name":"Hill lane","pts":wiggle([(-900,900),(-2200,1800),(-3400,3100),(-4300,4300)],200,7,2),"width":5,"houses":True,"spacing":1500,"setback":900,"lines":[fence_line(300,400,"both","SM_StoneWall")]},
       {"name":"Mill lane","pts":wiggle([(900,-900),(2000,-2300),(3200,-3600),(4300,-4600)],200,8,2),"width":5,"houses":True,"spacing":1500,"setback":900,"lines":[fence_line(300,400,"both","SM_StoneWall")]},
       {"name":"Inn lane","pts":[(900,870),(1400,1250),(1900,1500)],"width":5,"houses":False},
       {"name":"Spring path","pts":[(2200,1700),(2700,2100),(3100,2500)],"width":3,"houses":False},
       {"name":"Quarry track","pts":[(-2600,-350),(-3300,-1500),(-3900,-3000),(-3900,-4100)],"width":3,"houses":False},
       {"name":"Winespring brook","kind":"water","pts":[(3100,1400),(2950,700),(2650,-300),(2900,-1300),(3300,-2400),(3200,-3500),(3600,-5000),(3700,-7000)],"width":3,"houses":False,"mat":WATER,"tile_z":-3,"depth":100},
    ]
    s["houses"]={"spacing":1500,"setback":960,"scale":(0.72,0.9),"keep":0.9,"footprint":YARD_FOOTPRINT,"entries":[(HK,1,[R,WD,W,None,None]),(HK,1,[R2,WD,W3,None,None])]+two_rivers_entries()}
    s["line_house_clear"]=760   # street fences stop short of every house front
    s["foliage"]=base_foliage(TREES_TEMPERATE+TREES_PINE()[:1],forest_ratio=0.28,edge_ratio=0.06,town_ratio=0.01,bush=0.02,grass=0.3,mush=0.01,extent=E,
                              extra=[boulders_extra((0.6,1.0),0.01)]+field_extras(0.02))
    s["pads"]=[(("rect",1050,500,2500,2000),None),(("circle",-3000,2400,900),None)]
    s["props"]=[
       ("mesh",MESH["oldtree"],"TheGreen_OldTree",(0,0,0),20,2.2,None),("mesh",MESH["well"],"VillageWell",(650,-450,0),15,1.5,None),
       ("mesh",kit_mesh("SM_Inn"),"WinespringInn",(1750,1250,0),inn_yaw,1.15,kit_mats("SM_Inn",{"P_Rose":"P_Gold"})),
       ("mesh",kit_mesh("SM_Smithy"),"LuhhansForge",(3650,-2950,0),-150,1.0,None),("mesh",kit_mesh("SM_Barn"),"AlVereBarn",(-2200,3050,0),20,0.95,None),
       ("mesh",kit_mesh("SM_Watermill"),"ThanesMill",(3750,-4350,0),90,1.0,None),("mesh",kit_mesh("SM_Stable"),"InnStable",(2750,850,0),200,0.9,None),
       ("mesh",kit_mesh("SM_LaundryLine"),"InnLaundry",(2300,1900,0),30,1.0,None),("mesh",kit_mesh("SM_WaterTrough"),"GreenTrough",(1050,-200,0),100,1.0,None),
       ("mesh",kit_mesh("SM_LogBench"),"GreenLog",(-500,-300,0),35,1.0,None),("mesh",kit_mesh("SM_Wheelbarrow"),"GreenBarrow",(800,300,0),-70,1.0,None),
       ("mesh",kit_mesh("SM_CropRows"),"InnGarden",(1100,2000,0),20,1.0,None),("mesh",kit_mesh("SM_Beehive"),"InnHive",(1450,2250,0),0,1.0,None),
       ("mesh",kit_mesh("SM_AppleTree"),"GreenApple",(-800,-200,0),0,1.1,None),("mesh",kit_mesh("SM_AppleTree"),"InnApple",(1300,650,0),40,1.0,None),
       ("mesh",kit_mesh("SM_FestivalPole"),"BelTinePole",(-250,-250,0),0,1.0,None),("mesh",kit_mesh("SM_Signpost"),"GreenSignpost",(-1000,-1400,0),30,1.0,None),
       ("mesh",kit_mesh("SM_Bench"),"GreenBench1",(350,-900,0),80,1.0,None),("mesh",kit_mesh("SM_Bench"),"GreenBench2",(-700,600,0),-40,1.0,None),
       ("mesh",kit_mesh("SM_Cart"),"InnCart",(2250,2050,0),140,1.0,None),("mesh",kit_mesh("SM_HayBale"),"InnHay",(2600,2150,0),20,1.0,None),("mesh",kit_mesh("SM_Barrel"),"InnBarrel",(1300,1700,0),0,1.0,None),
       ("mesh",kit_mesh("SM_Woodpile"),"ForgeWood",(3950,-3250,0),60,1.0,None),("mesh",kit_mesh("SM_FlowerBox"),"GreenFlowers",(-200,-1100,0),10,1.0,None),
       ("mesh",CAMP,"Campfire",(-480,380,0),0,1.0,None),("mesh",CHEST,"Chest_Inn",(2650,1900,0),inn_yaw+180,1.0,None),("mesh",CHEST,"Chest_West",(-3300,1450,0),90,1.0,None),
       ("mesh",CRATE,"Crate1",(2350,1750,0),10,1.0,None),("mesh",CRATE,"Crate2",(2480,1800,0),35,1.0,None),
       ("mesh",PEN,"SheepPen",(-3000,2400,0),0,1.0,None),("mesh",SHEEP,"Sheep0",(-3250,2200,80),0,1.0,None),("mesh",SHEEP,"Sheep1",(-2850,2500,80),70,1.0,None),("mesh",SHEEP,"Sheep2",(-3050,2650,80),140,1.0,None),
       ("mesh",HORSE,"InnHorse",(2500,2300,80),-60,1.0,None),("mesh",FIRE,"Fireflies0",(-700,-900,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(900,700,150),0,1.0,None),("mesh",FIRE,"Fireflies2",(3100,2500,150),0,1.0,None),
       ("mesh",kit_mesh("SM_GateHouse"),"QuarryMouth",(-3900,-4200,0),90,0.9,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_DarkWood"})),
       chest("QuarryChest",-4100,-4500,0,60),chest("SpringChest",3350,2850,0,40),potion("Potion_Spring",2800,2300,0),potion("Potion_Hill",-4300,4500,0),potion("Potion_Mill",4300,-4800,0),
       ladder("KnollLadder",3100+1160,2500,180,600),
       npc("Villager_Cenn","Cenn Buie","EF_Cenn",-1600,900,0,1000),npc("Villager_Bran","Bran al'Vere","EF_Bran",1300,1000,-120,700),
       hero("Egwene","Egwene al'Vere","EF_Egwene",400,-1700,90,500),hero("Mat","Mat Cauthon","EF_Mat",-2200,-2200,30,600),hero("Perrin","Perrin Aybara","EF_Perrin",3300,-3400,120,400),
       hero("Moiraine","Moiraine","EF_Moiraine",-350,-650,60,0,tags=("QuestName=Moiraine",)),hero("Nynaeve","Nynaeve al'Meara","EF_Nynaeve",-900,1500,-60,400),hero("Thom","Thom Merrilin","EF_Thom",2300,1550,-150,250)]
    s["props"]+=waterfall("Winespring",3100,1250,-90,480,-160,width=320)   # the lip of the spring pool (relief 480) down to the brook (-160)
    s["dialogue"]={**lines("EF_Cenn",["Thatching, thatching, always thatching. The Winespring Inn wants a tile roof, can you believe it?","Mark my words, lad, those Coplin boys will be trouble before Bel Tine."]),
                   **lines("EF_Bran",["Welcome to the Winespring Inn! Finest ale this side of the Mountains of Mist.","The spring up on the knoll never runs dry. Sweetest water in the Two Rivers."]),
                   **lines("EF_Egwene",["Rand! There is a lady at the inn in a blue dress, and a man with a sword who never smiles. Who are they?","Nynaeve says I am to be her apprentice. Wisdom of Emond's Field one day... unless I see the world first.","If you are leaving, I am coming with you. Don't argue."]),
                   **lines("EF_Mat",["Psst. Want to see something? I found a badger down by the Old Road. Thought we'd loose it on the Green for Bel Tine.","That black-cloaked rider on the road... you saw him too, didn't you? Perrin says I'm making it up.","Blood and ashes, Rand. If Moiraine says Trollocs are coming, I'm not waiting around to find out."]),
                   **lines("EF_Perrin",["Master Luhhan has me at the forge till dark. The whole village wants new hinges before winter.","I saw a rider on the road with a cloak that did not move in the wind. I have not told anyone but you.","I will bring the big axe. Someone should carry it."]),
                   **lines("EF_Moiraine",["You are Tam al'Thor's son. I am Moiraine, and the man watching the door is Lan. We have come a very long way to find you.","Listen carefully. Trollocs came to the Two Rivers for you and your friends. They will come again.","Gather Mat and Perrin, and Egwene if she insists. We leave before the Dark One's eyes find us. Tell no one else but the Wisdom.","The road north goes through Taren Ferry to Baerlon. Find me there when you arrive."]),
                   **lines("EF_Nynaeve",["Rand al'Thor. If you think I don't know what that Aes Sedai is planning, you are a bigger wool-head than Mat.","Leaving? With her? Light, you boys will be the death of me.","Go then. But I am Wisdom of this village, and where my people go, I follow. I will find you on the road."]),
                   **lines("EF_Thom",["Thom Merrilin, gleeman, at your service. Juggling, tales, and the harp, for the price of a bed and a meal.","Here is a tale: the Dragon Reborn, the Horn of Valere, and a shepherd who never asked for any of it. Hmm? Just a story.","If you are heading to Baerlon, a gleeman could use company on the road. The Aes Sedai may not like it. Good."])}
    s["moves"]=[("Tam_NPC",(1250,700,170),-140),("Lan_NPC",(1500,450,121),160)]+portal_moves(-5100,-200,0)
    Rl=Relief(E,base=0,water=None,seed=5)
    Rl.slope(7000,0,-7000,0,0,520)
    peaks(Rl,[(-7700,-7500),(-7400,-2000),(-7800,3000),(-7300,7500)],3400,2000,n=5,seed=5)
    Rl.hill(1400,2300,2000,220); Rl.hill(-2600,-2800,2600,340); Rl.hill(3600,-3400,2200,220); Rl.hill(-3600,3400,2300,320); Rl.hill(300,-5400,2300,260); Rl.hill(1200,5600,2600,300); Rl.hill(5600,2600,2200,180)
    Rl.noise(50,900,seed=3); Rl.noise(22,320,seed=9)
    Rl.plateau(("circle",0,0,1500),200,900)
    Rl.plateau(("circle",3100,2500,1150),600,0)
    Rl.plateau(("rect",-4600,-4800,-3300,-3500),1000,0)
    Rl.valley([(3100,1400),(2950,700),(2650,-300),(2900,-1300),(3300,-2400),(3200,-3500),(3600,-5000),(3700,-7000)],800,200,flat=160)
    Rl.rock_z=1500; Rl.snow_z=2300
    relief_apply(s,Rl,{"grass":GRASSMAT,"rock":ROCK,"sand":SAND,"snow":SNOW})
    return s

def baerlon_spec():
    E=8000
    houses=rand_house_entries(["MI_Wall_Cream","MI_Wall_Tan"],["MI_Roof_Thatch","MI_Roof_Slate"])+stone_town_entries()+[kit_entry("SM_StoneTimberHouseYard",None,3),kit_entry("SM_LHouse",{"P_Terracotta":"P_Slate"},1),kit_entry("SM_TimberHouseYard",{"P_Straw":"P_Shingle"},2),kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Slate"},1),kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Ochre"},2),kit_entry("SM_Longhouse",None,1)]
    miners=[kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_DarkGrey"},3),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_DarkGrey"},2)]
    s=town_base("Baerlon",E,road_mat=MAT("MI_Tile_DirtRoad"))
    streets=grid_streets(-3600,3600,-3600,3600,4,4,width=5,spacing=1200,setback=820,prefix="st",wig=160,seed=11)
    camp=grid_streets(-7000,-4600,-7000,-4400,2,3,width=3,spacing=900,setback=620,set_="miners",prefix="camp",wig=120,seed=21)
    s["roads"]=[{"name":"Palisade","kind":"wall","pts":circle(0,0,4800,36),"closed":True,"width":1,"tile_scale":(1,1,2.6),"tile_z":130,"mat":MAT("MI_Wood_Dark"),"towers":{"step":2400,"scale":(2.2,2.2,4.0),"z":200,"mat":MAT("MI_Wood_Grey")}},
           {"name":"South road","pts":wiggle([(0,4800),(100,6200),(0,7600)],200,2,2),"width":7,"houses":False},{"name":"North road","pts":wiggle([(0,-4800),(-200,-6200),(0,-7600)],200,3,2),"width":7,"houses":False},
           {"name":"Mine road","pts":wiggle([(-3600,-3600),(-4600,-4400),(-5600,-5400),(-6300,-6400)],200,4,2),"width":5,"houses":False},
           {"name":"Mine path","pts":[(-6300,-6400),(-6600,-6900)],"width":3,"houses":False},
           {"name":"Hill lane","pts":wiggle([(3600,3600),(4800,4800),(6200,5400)],200,5,2),"width":5,"houses":True,"spacing":1300,"setback":860},
           {"name":"Stream","kind":"water","pts":[(7800,-7800),(5200,-5200),(4700,-2400),(5600,500),(5000,3200),(5800,6000),(6200,7800)],"width":3,"houses":False,"mat":WATER,"tile_z":-3,"depth":100}]+streets+camp
    s["plazas"]=[{"pts":rect(-1300,-1300,1300,1300),"mat":MAT("MI_Tile_Plaza"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]+market_extras(0.02)}]
    s["exclusions"]=[(0,6900,500,700),(-4000,4000,800,800),(-6700,-7000,600,600)]
    s["houses"]={"spacing":1200,"setback":820,"scale":(0.7,0.92),"keep":0.92,"footprint":YARD_FOOTPRINT,"entries":houses}
    s["house_sets"]={"miners":dense_set(miners,900,620,(0.6,0.78),0.95)}
    s["foliage"]=base_foliage(TREES_PINE()+TREES_TEMPERATE[:2],forest_ratio=0.26,edge_ratio=0.06,town_ratio=0.006,bush=0.02,grass=0.3,mush=0.012,extent=E,bands={"forest":(0.86,1.0),"edge":(0.78,0.86),"town":(0.25,0.78)},
                              extra=[boulders_extra((0.5,1.0),0.025)])
    s["pads"]=[(("rect",-1800,-3000,0,-1500),None),(("circle",-4000,4000,900),None)]
    s["props"]=[("mesh",kit_mesh("SM_Inn"),"StagAndLion",(-900,-2200,0),90,1.1,kit_mats("SM_Inn",{"P_RedBrick":"P_Slate","P_Rose":"P_Gold"})),
       ("mesh",kit_mesh("SM_StoneWell"),"Well",(0,0,0),0,1.3,None),("mesh",kit_mesh("SM_GateHouse"),"SouthGate",(0,4800,0),90,1.1,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkWood","P_LightGrey":"P_Brown"})),
       ("mesh",kit_mesh("SM_GateHouse"),"NorthGate",(0,-4800,0),90,1.1,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkWood","P_LightGrey":"P_Brown"})),
       ("mesh",kit_mesh("SM_GateHouse"),"MineMouth",(-6700,-7000,0),45,0.9,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_DarkWood"})),
       ("mesh",PEN,"Pen",(-4000,4000,0),0,1.0,None),("mesh",SHEEP,"Sheep0",(-4200,3800,80),0,1.0,None),("mesh",SHEEP,"Sheep1",(-3850,4150,80),90,1.0,None),
       ("mesh",CHEST,"Chest_Inn",(-300,-2700,0),0,1.0,None),("mesh",HORSE,"InnHorse",(-1700,-2500,80),-60,1.0,None),("mesh",FIRE,"Fireflies0",(2500,2500,150),0,1.0,None),
       ("mesh",CAMP,"MinersFire",(-5800,-5700,0),0,1.0,None),chest("MineChest",-6900,-7200,0,45),potion("Potion_Mine",-6400,-6700,0),potion("Potion_Hill",6300,5600,0),chest("HillChest",6500,5300,0,200),
       npc("Innkeeper_Fitch","Master Fitch","BL_Fitch",-1000,-1700,90,500),npc("Miner_Jon","Jon Thane","BL_Miner",-5600,-5500,135,900),
       npc("Whitecloak","Child of the Light","BL_Whitecloak",-2800,2900,-45,700),npc("Wisdom_Nynaeve","Nynaeve","BL_Nynaeve",400,600,-120,600),npc("Min","Min Farshaw","BL_Min",-1200,-2900,60,500)]
    s["dialogue"]={**lines("BL_Fitch",["The Stag and Lion welcomes you. Rooms upstairs, stew downstairs, and no fighting.","The Whitecloaks have been asking questions. I did not tell them anything."]),
                   **lines("BL_Miner",["Baerlon is a mining town, whatever the fine folk say. Iron and silver.","The hills north of here are full of old shafts. Mind where you step."]),
                   **lines("BL_Whitecloak",["Walk in the Light, friend. The Children watch over Baerlon.","Darkfriends hide in plain sight. Report anything unusual to the Children."]),
                   **lines("BL_Nynaeve",["You came all this way with no cloak? You will catch your death.","When we get home, there will be words. Many words."]),
                   **lines("BL_Min",["I see things around people. Sparks, and a shadow. Don't ask.","The stable boy has a hole in his boot. See? Sometimes it's just boots."])}
    s["props"]+=[hero("Moiraine","Moiraine","BL_Moiraine",300,-900,180,0,tags=("QuestName=Moiraine",)),hero("Thom","Thom Merrilin","BL_Thom",-700,-500,60,250)]
    s["dialogue"].update(lines("BL_Moiraine",["You came. Good. Baerlon has eyes too; Whitecloaks in the streets and worse things in the alleys.","We ride for Caemlyn at first light. The road runs past a dead city, Shadar Logoth. Whatever happens there, take nothing from it."]))
    s["dialogue"].update(lines("BL_Thom",["The Stag and Lion. I have played worse rooms. The ale is honest.","Watch the Whitecloaks, boy. They see Darkfriends in every shadow, and they do not care to be wrong."]))
    s["moves"]=portal_moves(0,7400,-90)
    Rl=Relief(E,base=0,water=None,seed=8)
    Rl.slope(0,8000,0,-8000,0,420)
    peaks(Rl,[(-8800,-9000),(-6000,-8600),(-2000,-9200),(3000,-8800),(8800,-9000)],3800,1500,n=6,seed=8)
    Rl.hill(-5800,-5800,2600,420); Rl.hill(5200,5000,3200,420); Rl.hill(-5400,4600,2400,240); Rl.hill(6500,-2500,2600,300)
    Rl.plateau(("circle",0,0,2300),360,1400); Rl.plateau(("circle",0,0,4900),240,700)
    Rl.plateau(("circle",-6700,-7000,1100),1100,0)
    Rl.noise(50,800,seed=2); Rl.noise(20,300,seed=4)
    Rl.valley([(7800,-7800),(5200,-5200),(4700,-2400),(5600,500),(5000,3200),(5800,6000),(6200,7800)],700,180,flat=140)
    Rl.rock_z=1300; Rl.snow_z=2000
    relief_apply(s,Rl,{"grass":GRASSMAT,"rock":ROCK,"sand":SAND,"snow":SNOW})
    return s

def whitebridge_spec():
    E=8000; WL=-150
    houses=rand_house_entries(["MI_Wall_Cream","MI_Wall_Plaster"],["MI_Roof_Thatch","MI_Roof_ThatchPale"])+two_rivers_entries()+[kit_entry("SM_Shop",{"P_Slate":"P_Shingle","P_Terracotta":"P_Blue"},1),kit_entry("SM_GableHouse",{"P_Plaster":"P_Cream","P_Terracotta":"P_Ochre"},2),kit_entry("SM_Longhouse",None,1)]
    s=town_base("Whitebridge",E,center=(-2500,0),road_mat=MAT("MI_Tile_DirtRoad"))
    s["plazas"]=[{"pts":rect(-3300,-1100,-1300,1100),"mat":MAT("MI_Tile_Plaza"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.05,0.9,1.1,None)]+market_extras(0.02)}]
    s["exclusions"]=[(-2300,0,1100,1200),(-7200,0,500,700),(2600,0,2400,8000),(-300,0,500,800),(5400,0,500,800)]
    s["roads"]=[{"name":"Main street","pts":[(-7700,0),(-5400,0),(-3400,0),(-1200,0),(-500,0)],"width":7,"houses":True,"spacing":1300,"setback":900,"lines":[lamp_line(1600,480,"P_Copper")]},
              {"name":"East road","pts":wiggle([(5600,0),(6800,100),(7800,0)],150,2,2),"width":7,"houses":True,"spacing":1300,"setback":900},
              {"name":"River street N","pts":[(-700,700),(-800,2500),(-600,4500),(-900,7600)],"width":5,"houses":True,"sides":"left","spacing":1200,"setback":850},
              {"name":"River street S","pts":[(-700,-700),(-800,-2500),(-600,-4500),(-900,-7600)],"width":5,"houses":True,"sides":"right","spacing":1200,"setback":850},
              {"name":"Quay","pts":[(300,-5600),(300,-3000),(300,-1200)],"width":5,"houses":False,"lines":[{"mesh":MESH["fence"],"step":250,"offset":280,"mat":None,"scale":(1,1,1),"sides":"left"}]},
              {"name":"Quay stairs","pts":[(-700,-2500),(300,-2500)],"width":3,"houses":False},
              {"name":"North lane","pts":wiggle([(-3000,1100),(-3100,3000),(-2800,5800),(-2600,7600)],200,3,2),"width":5,"houses":True,"spacing":1300,"setback":880},
              {"name":"South lane","pts":wiggle([(-3000,-1100),(-3100,-3000),(-2800,-5800),(-2600,-7600)],200,4,2),"width":5,"houses":True,"spacing":1300,"setback":880},
              {"name":"Ferry lane","pts":[(5600,-400),(5200,-2200),(4800,-3600)],"width":3,"houses":False},
              {"name":"Upper lane","pts":wiggle([(-5400,0),(-5800,-2200),(-6400,-4400)],200,5,2),"width":5,"houses":True,"spacing":1300,"setback":880}]
    s["roads"]+=pier_roads("Pier",300,(-4800,-3400),1100,WL)+pier_roads("FerryPier",4700,(-3700,),-900,WL)
    s["houses"]={"spacing":1300,"setback":900,"scale":(0.75,0.95),"keep":0.9,"footprint":YARD_FOOTPRINT,"entries":houses}
    s["foliage"]=base_foliage(TREES_TEMPERATE,forest_ratio=0.22,edge_ratio=0.05,town_ratio=0.008,bush=0.02,grass=0.3,mush=0.01,extent=E,
                              extra=[{"name":"Reeds","band":(0.0,1.0),"ratio":0.0,"entries":[],"scale":(1,1)}])
    s["pads"]=[(("rect",-3000,900,-1600,2200),None),(("rect",-900,-700,-300,700),0),(("rect",5000,-700,5800,700),0),(("rect",-200,-6000,800,-900),-40)]
    s["props"]=[("mesh",kit_mesh("SM_WhiteBridge"),"TheWhiteBridge",(2600,0,0),0,(1.3,1.0,1.0),None,"abs"),
       ("mesh",kit_mesh("SM_InnThatch"),"WayfarersRest",(-2300,1500,0),-90,1.1,None),
       ("mesh",kit_mesh("SM_StoneWell"),"Well",(-2300,-500,0),0,1.2,None),("mesh",CHEST,"Chest_Inn",(-1600,2100,0),0,1.0,None),
       ("mesh",HORSE,"InnHorse",(-1500,2600,80),-60,1.0,None),("mesh",FIRE,"Fireflies0",(-2300,0,150),0,1.0,None),("mesh",FIRE,"Fireflies1",(300,-3000,150),0,1.0,None),
       ("mesh",CRATE,"Crate1",(400,-3200,0),10,1.0,None),("mesh",CRATE,"Crate2",(500,-3350,0),40,1.0,None),("mesh",CAMP,"QuayFire",(600,-4300,0),0,1.0,None),
       chest("QuayChest",500,-5500,0,90),potion("Potion_Quay",400,-1400,0),potion("Potion_Hill",-6400,-4600,0),chest("FerryChest",4600,-3900,0,0),
       npc("Innkeeper_Fitch","Master Fitch","WB_Fitch",-2600,900,-45,500),npc("Ferryman","Ferryman","WB_Ferry",5000,-1600,90,700),
       npc("Gleeman_Kid","Olver","WB_Olver",-3400,-2200,135,1200),npc("Bargeman","Bargeman","WB_Barge",400,-2800,180,600)]
    s["dialogue"]={**lines("WB_Fitch",["The Wayfarer's Rest, best inn on the Arinelle. Mind the stairs, the cellar floods.","The White Bridge? Older than the town. Harder than steel. Nobody knows who built it."]),
                   **lines("WB_Ferry",["Nobody takes the ferry since the bridge. Cheaper to walk.","The water runs cold and fast below the bridge. I have fished men out of it."]),
                   **lines("WB_Olver",["Did you know the bridge looks like glass but you can't see through it?","Thom the gleeman said he'd teach me to juggle. Then he left."]),
                   **lines("WB_Barge",["Barges from Baerlon, barges to Caemlyn. Everything on the Arinelle stops at Whitebridge.","Down those stairs is the quay. Mind the crates, the cooper will have your ears."])}
    s["props"]+=[hero("Thom","Thom Merrilin","WB_Thom",1800,-700,-120,0,tags=("QuestName=Thom Merrilin",))]
    s["dialogue"].update(lines("WB_Thom",["Across the White Bridge and we are out of Andor's shadow at last. Or into Caemlyn's. Hard to say which is worse.","If a Fade comes, boy, you run. I have run from worse things than Fades. Let an old gleeman buy you a little time.","Caemlyn. Find the Queen's Blessing inn; Master Gill owes me a favor. Go. GO."]))
    s["moves"]=portal_moves(-7300,0,0)
    Rl=Relief(E,base=0,water=WL,seed=12)
    Rl.slope(1000,0,-8000,0,0,420)
    Rl.hill(-6500,-5000,3200,380); Rl.hill(-5500,6000,3200,300); Rl.hill(7000,4500,3200,420); Rl.hill(7500,-6500,3000,360)
    Rl.noise(40,900,seed=1); Rl.noise(18,300,seed=5)
    Rl.valley([(2600,-9000),(2650,-3000),(2600,3000),(2550,9000)],2600,520,flat=1500)
    Rl.plateau(("rect",-200,-6000,800,-900),-40,0)
    Rl.plateau(("rect",-1200,-900,-200,900),0,300); Rl.plateau(("rect",5000,-900,6000,900),0,300)
    Rl.rock_z=None
    relief_apply(s,Rl,{"grass":GRASSMAT,"rock":ROCK,"sand":DIRT,"snow":SNOW})
    return s

def shadarlogoth_spec():
    E=10000
    ruins=[kit_entry("SM_StoneHouse",{"P_Stone":"P_DarkGrey","P_LightGrey":"P_Grey","P_Slate":"P_Black","P_DarkWood":"P_Black","P_Glass":"P_Black"},3),kit_entry("SM_StoneHouse3",{"P_Stone":"P_DarkGrey","P_LightGrey":"P_Grey","P_Slate":"P_Black","P_DarkWood":"P_Black","P_Glass":"P_Black"},2),
           kit_entry("SM_TowerHouse",{"P_Plaster":"P_DarkGrey","P_Slate":"P_Black","P_DarkWood":"P_Black","P_Glass":"P_Black"},2),
           kit_entry("SM_GableHouse",{"P_Plaster":"P_Grey","P_Terracotta":"P_DarkGrey","P_DarkWood":"P_Black","P_Glass":"P_Black"},2),
           kit_entry("SM_RuinWall",None,4),kit_entry("SM_RuinWall",{"P_DarkGrey":"P_Black"},2),kit_entry("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_Black"},1)]
    s=town_base("ShadarLogoth",E,road_mat=MAT("MI_Tile_CobbleDark"))
    alleys=grid_streets(-6500,6500,-6500,6500,9,9,width=3,spacing=900,setback=620,prefix="alley",wig=90,seed=31)
    s["roads"]=[{"name":"Broken wall","kind":"wall","pts":circle(0,0,7700,44),"closed":True,"width":2,"tile_scale":(1,1,3.0),"tile_z":150,"mat":MAT("MI_Wall_DarkStone"),"keep":0.55,"towers":{"step":2600,"scale":(2.8,2.8,5.0),"z":250,"mat":MAT("MI_Wall_DarkStone")}},
           {"name":"Entry road","pts":[(7700,0),(8600,0),(9400,0)],"width":5,"houses":False},
           {"name":"Citadel ring","pts":circle(0,0,1900,20),"closed":True,"width":5,"houses":False},
           {"name":"Terrace ring","pts":circle(0,0,4600,36),"closed":True,"width":5,"houses":True,"sides":"right","spacing":1100,"setback":760}]+radial_streets(0,0,1900,7600,8,width=5,houses=False,prefix="avenue")+alleys
    s["plazas"]=[{"pts":rect(-1500,-1500,1500,1500),"mat":ASH,"linear":True,"scatter":[(kit_mesh("SM_RuinWall"),0.04,0.8,1.3,None),(kit_mesh("SM_Boulder"),0.03,0.6,1.2,kit_mats("SM_Boulder",{"P_Grey":"P_DarkGrey","P_DarkGrey":"P_Black"}))]}]
    s["exclusions"]=[(0,0,800,800),(9000,0,500,700)]
    s["houses"]={"spacing":900,"setback":620,"scale":(0.65,0.95),"keep":0.75,"footprint":(-520,-330,420,380),"jitter":120,"entries":ruins}
    s["foliage"]=base_foliage(TREES_DEAD,forest_ratio=0.12,edge_ratio=0.05,town_ratio=0.012,bush=0.0,grass=0.0,extent=E,ground=None,bands={"forest":(0.88,1.0),"edge":(0.6,0.88),"town":(0.2,0.6)},
                              extra=[{"name":"Rubble","band":(0.0,1.0),"ratio":0.03,"entries":[kit_entry("SM_Boulder",{"P_Grey":"P_DarkGrey","P_DarkGrey":"P_Black"},3),kit_entry("SM_RuinWall",None,1)],"scale":(0.5,1.2),"prune":120}])
    s["pads"]=[(("rect",-1600,-1600,1600,1600),None)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"RuinedCitadel",(0,0,0),180,1.6,kit_mats("SM_Keep",{"P_DarkGrey":"P_Black","P_Grey":"P_DarkGrey","P_Slate":"P_Black"})),
       ("mesh",kit_mesh("SM_Obelisk"),"CursedSpire",(-1800,1900,0),0,1.1,kit_mats("SM_Obelisk",{"P_White":"P_Black","P_LightGrey":"P_DarkGrey","P_Gold":"P_Rust"})),
       ("mesh",kit_mesh("SM_Obelisk"),"CursedSpire2",(5200,-5600,0),0,1.4,kit_mats("SM_Obelisk",{"P_White":"P_DarkGrey","P_LightGrey":"P_Black","P_Gold":"P_Rust"})),
       ("mesh",CHEST,"Chest_Treasure",(300,-1200,0),30,1.0,None),("mesh",CHEST,"Chest_Dagger",(-4000,3500,0),130,1.0,None),
       ladder("CitadelLadder",-1920,0,180,600),ladder("ChasmLadder",-4200,-6100,90,600),
       potion("Potion_Hill",-1200,1200,0),potion("Potion_Gate",8400,700,0),chest("RuinChest",-6500,-6500,0,20),chest("ChasmChest",-3800,-6400,0,0),potion("Potion_Chasm",4000,5900,0),
       npc("Ghost_Mordeth","Mordeth","SL_Mordeth",-600,-600,45,300),npc("Scout_Child","Child Byar","SL_Byar",8200,-600,180,500)]
    s["dialogue"]={**lines("SL_Mordeth",["Treasure... there is treasure here, friend. Take what you like.","No one leaves Shadar Logoth with more than they brought. Except me."]),
                   **lines("SL_Byar",["The Children do not enter this cursed place. Nor should you.","Shadows move in the ravines after dark. Do not follow them."])}
    s["props"]+=waygate("ShadarLogoth",-5200,2600,90,"Ways","ShadarLogothWays","WaysShadarLogoth")
    s["moves"]=portal_moves(9000,0,180)
    Rl=Relief(E,base=0,water=-200,seed=17)
    Rl.hill(0,0,8000,700); Rl.hill(-5000,4500,3200,300); Rl.hill(5000,-4500,3200,260)
    Rl.plateau(("circle",0,0,2300),1000,0); Rl.plateau(("circle",0,0,5100),640,0)
    Rl.valley([(-9000,-7500),(-6000,-6400),(-3200,-6200),(-1500,-4500),(-900,-2600)],900,520,flat=150)
    Rl.valley([(2400,2600),(3600,4800),(4800,6100),(5800,9000)],900,520,flat=150)
    Rl.valley([(9400,-9500),(10400,-3000),(10300,3000),(9600,9500)],3400,900,flat=1800)
    Rl.noise(90,700,seed=3); Rl.noise(30,250,seed=6)
    Rl.rock_z=None; Rl.stairs_mats=kit_mats("SM_Stairs200",{"P_LightGrey":"P_DarkGrey","P_Grey":"P_Black"})
    relief_apply(s,Rl,{"grass":ASH,"rock":DARKROCK,"sand":DARKROCK,"snow":SNOW})
    return s

def faldara_spec():
    E=10000
    houses=stone_town_entries(wall="P_DarkGrey",roof="P_Slate",quoin="P_Grey",shops=False)+[kit_entry("SM_TimberHouseSlate",{"P_Cream":"P_Plaster","P_DarkGreen":"P_Black"},2),kit_entry("SM_Barn",{"P_Shingle":"P_Slate"},1),kit_entry("SM_Longhouse",{"P_Brown":"P_DarkWood","P_Olive":"P_Slate"},3),kit_entry("SM_Longhouse",{"P_Brown":"P_Brown","P_Olive":"P_DarkGrey"},2),
            kit_entry("SM_GableHouse",{"P_Plaster":"P_Brown","P_Terracotta":"P_Slate","P_DarkWood":"P_Black"},3),kit_entry("SM_GableHouse",{"P_Plaster":"P_DarkGrey","P_Terracotta":"P_Snow","P_DarkWood":"P_DarkWood"},2),
            kit_entry("SM_TowerHouse",{"P_Plaster":"P_DarkGrey","P_Slate":"P_Slate","P_DarkWood":"P_Black"},1)]
    s=town_base("FalDara",E)
    streets=grid_streets(-5200,5200,-5200,5200,5,5,width=5,spacing=1100,setback=760,prefix="st")
    for r in streets: r["lines"]=[lamp_line(1800,330,"P_Copper")]
    camp=grid_streets(-4500,4500,7900,9500,5,2,width=3,spacing=900,setback=620,set_="camp",prefix="camp",wig=100,seed=41)
    s["roads"]=[{"name":"Fortress wall","kind":"wall","pts":rect(-6200,-6200,6200,6200),"closed":True,"linear":True,"width":3,"tile_scale":(1,1,4.0),"tile_z":200,"mat":MAT("MI_Wall_DarkStone"),"towers":{"step":2050,"scale":(3.2,3.2,6.4),"z":320,"mat":MAT("MI_Wall_Stone")}},
           {"name":"Keep ring","pts":circle(0,0,2300,20),"closed":True,"width":5,"houses":False},
           {"name":"East road","pts":[(6200,0),(7000,0),(7800,0),(8900,0)],"width":7,"houses":False},
           {"name":"South road","pts":[(0,6200),(0,7000),(0,7800),(0,9600)],"width":7,"houses":False},
           {"name":"Moat","kind":"water","pts":circle(0,0,7300,52),"closed":True,"width":5,"houses":False,"mat":WATER,"tile_z":-3,"depth":160},
           {"name":"East bridge","pts":[(6700,0,20),(7300,0,20),(7900,0,20)],"width":7,"houses":False,"mat":WOODMAT,"tile_z":0,"nocarve":True,"rel":True,"linear":True},
           {"name":"South bridge","pts":[(0,6700,20),(0,7300,20),(0,7900,20)],"width":7,"houses":False,"mat":WOODMAT,"tile_z":0,"nocarve":True,"rel":True,"linear":True}]+streets+camp
    s["plazas"]=[{"pts":rect(-1900,-1900,1900,1900),"mat":MAT("MI_Tile_PlazaDark"),"linear":True,"scatter":[(kit_mesh("SM_MarketStall"),0.03,0.9,1.1,kit_mats("SM_MarketStall",{"P_Terracotta":"P_DarkGrey"}))]}]
    s["exclusions"]=[(-400,0,1700,1500),(8700,0,500,700)]
    s["houses"]={"spacing":1100,"setback":760,"scale":(0.7,0.9),"keep":0.95,"footprint":(-560,-380,520,420),"jitter":60,"entries":houses}
    s["house_sets"]={"camp":dense_set(houses[:4],900,620,(0.55,0.7),0.95)}
    s["foliage"]=base_foliage(TREES_PINE(snow=True),forest_ratio=0.28,edge_ratio=0.06,town_ratio=0.004,bush=0.008,grass=0.12,extent=E,ground="snow",bands={"forest":(0.9,1.0),"edge":(0.84,0.9),"town":(0.3,0.84)},
                              extra=[boulders_extra((0.6,1.0),0.02,{"P_Grey":"P_Ice"}),{"name":"Snow","band":(0.0,1.0),"ratio":0.05,"entries":[kit_entry("SM_SnowPatch")],"scale":(0.8,1.8)}])
    s["pads"]=[(("rect",-1900,-1900,1900,1900),None)]
    s["props"]=[("mesh",kit_mesh("SM_Keep"),"FalDaraKeep",(-400,0,0),180,1.9,None),
       ("mesh",kit_mesh("SM_GateHouse"),"EastGate",(6200,0,0),0,1.5,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_Grey"})),
       ("mesh",kit_mesh("SM_GateHouse"),"SouthGate",(0,6200,0),90,1.5,kit_mats("SM_GateHouse",{"P_Grey":"P_DarkGrey","P_LightGrey":"P_Grey"})),
       ("mesh",CHEST,"Chest_Keep",(1200,-1200,0),45,1.0,None),("mesh",HORSE,"WarHorse",(1500,1300,80),-100,1.0,None),("mesh",CAMP,"GateFire",(5600,700,0),0,1.0,None),("mesh",CAMP,"CampFire",(0,8700,0),0,1.0,None),
       ladder("KeepLadderN",0,-2560,-90,600),ladder("KeepLadderS",0,2560,90,600),ladder("WallLadder",6100,3000,0,600),
       chest("KeepChest",1200,1200,0,45),potion("Potion_Keep",-1200,-1200,0),potion("Potion_Gate",5600,-700,0),chest("WallChest",-5700,5600,0,135),chest("CampChest",3900,8900,0,180),potion("Potion_Ridge",-3000,-9000,0),
       npc("Lord_Agelmar","Lord Agelmar","FD_Agelmar",900,-900,135,500),npc("Soldier_Ingtar","Ingtar","FD_Ingtar",3800,2600,-90,900),
       npc("Smith_Ragan","Ragan","FD_Ragan",-3800,-2600,45,800),npc("Child_Uno","Uno","FD_Uno",5200,-3900,180,1000),npc("Camp_Hurin","Hurin","FD_Hurin",-2000,8600,0,1200)]
    s["dialogue"]={**lines("FD_Agelmar",["Peace favor your sword, traveler. Fal Dara is the last light before the Blight.","The keep has stood against Trollocs for a thousand years. It will stand tonight."]),
                   **lines("FD_Ingtar",["Every Shienaran man carries a sword. Every woman knows how to use one.","The Gap is quiet. Too quiet. Keep your lantern lit."]),
                   **lines("FD_Ragan",["Good steel comes from the forges here, hammered in the cold.","Snow in the streets till Sunday. Mind the ice on the keep stairs."]),
                   **lines("FD_Uno",["Bloody flaming Trollocs took my goat last winter.","If you see a goat with one horn, that one's mine."]),
                   **lines("FD_Hurin",["I can smell violence, you know. Old killings. The mountains north of here reek of it.","The camp outside the walls is for traders. Nobody sleeps outside the walls by choice."])}
    s["props"]+=waygate("FalDara",5200,-4300,-90,"Ways","FalDaraWays","WaysFalDara")
    s["moves"]=portal_moves(8700,0,180)
    Rl=Relief(E,base=0,water=None,seed=23)
    Rl.slope(0,10000,0,-10000,0,500)
    peaks(Rl,[(-11000,-9800),(-6000,-9300),(0,-9900),(6000,-9400),(11000,-9800)],4400,2200,n=7,seed=23)
    Rl.hill(-8500,3000,3000,500); Rl.hill(8800,5500,3200,620); Rl.hill(0,0,6400,300)
    Rl.plateau(("rect",-6400,-6400,6400,6400),360,0); Rl.plateau(("circle",0,0,2600),840,0)
    Rl.plateau(("rect",-5000,7600,5000,9800),80,300)
    Rl.noise(45,900,seed=2); Rl.noise(18,300,seed=3)
    Rl.rock_z=1000; Rl.snow_z=2200
    relief_apply(s,Rl,{"grass":TUNDRA,"rock":DARKROCK,"sand":DIRT,"snow":SNOW})
    return s
