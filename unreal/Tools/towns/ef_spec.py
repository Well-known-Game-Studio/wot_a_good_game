def ef_spec():
    W=MAT("MI_Wall_Cream"); W2=MAT("MI_Wall_Tan"); W3=MAT("MI_Wall_Plaster"); W4=MAT("MI_Wall_White")
    R=MAT("MI_Roof_Thatch"); R2=MAT("MI_Roof_ThatchPale"); WD=MAT("MI_Wood_Dark"); WG=MAT("MI_Wood_Grey")
    H=MESH["house"]; ring=circle(0,0,1250,20)
    inn_yaw=math.degrees(math.atan2(-1250,-1750))+180
    return {
     "graph_name":"PCG_EmondsField","folder":"/Game/Wot/Towns/EmondsField","center":(0,0),
     "road_mat":MAT("MI_Tile_DirtRoad"),
     "plazas":[{"pts":circle(650,-450,330,12),"mat":MAT("MI_Tile_Plaza")}],
     "exclusions":[(0,0,380,380),(1750,1250,1150,950),(650,-450,380,380),(-3000,2400,750,750),(-4450,-200,650,700)],
     "roads":[
       {"name":"Green ring","pts":ring,"closed":True,"width":5,"houses":True,"sides":"right","spacing":1700,"setback":820},
       {"name":"West road","pts":[(-1250,0),(-2500,-250),(-3600,-150),(-4950,-200)],"width":7,"houses":True,"spacing":1500},
       {"name":"East road","pts":[(1250,0),(2600,250),(3700,150),(4950,120)],"width":7,"houses":True,"spacing":1500},
       {"name":"North road","pts":[(0,1250),(250,2500),(150,3600),(250,4950)],"width":7,"houses":True,"spacing":1500},
       {"name":"South road","pts":[(0,-1250),(-250,-2500),(50,-3700),(-100,-4950)],"width":7,"houses":True,"spacing":1500},
       {"name":"Inn lane","pts":[(900,870),(1400,1250),(1900,1500)],"width":5,"houses":False},
       {"name":"Winespring brook","kind":"water","pts":[(3100,2300),(3050,1500),(2850,700),(2600,-300),(2900,-1300),(3300,-2400),(3200,-3500),(3600,-4950)],"width":3,"houses":False,"mat":WATER,"tile_z":-2},
     ],
     "houses":{"spacing":1500,"setback":960,"scale":(0.72,0.9),"keep":0.9,
       "entries":[(H,3,[R,WD,W,None,None]),(H,2,[R,WD,W2,None,None]),(H,2,[R2,WD,W3,None,None]),(H,1,[R2,WG,W4,None,None]),(H,1,[R,WG,W2,None,None])]},
     "foliage":{"extent":5000,"forest_radius":4600,
       "trees":[(MESH["tree1"],3,None),(MESH["tree2"],3,None),(MESH["tree3"],2,None),(MESH["tree4"],2,None),(MESH["oldtree"],1,None)],
       "forest_band":(0.82,1.0),"forest_ratio":0.22,"edge_band":(0.58,0.82),"edge_ratio":0.05,"town_band":(0.2,0.58),"town_ratio":0.008,
       "bush_ratio":0.022,"grass_ratio":0.3,"mush_ratio":0.01},
     "props":[
       ("mesh",MESH["oldtree"],"TheGreen_OldTree",(0,0,0),20,2.2,None),
       ("mesh",MESH["well"],"VillageWell",(650,-450,0),15,1.5,None),
       ("mesh",H,"WinespringInn",(1750,1250,0),inn_yaw,1.45,[MAT("MI_Roof_RedTile"),MAT("MI_Wood_Dark"),MAT("MI_Wall_Plaster"),None,None]),
       ("mesh","/Game/Wot/Actors/Props/Campfire","Campfire",(-480,380,0),0,1.0,None),
       ("mesh","/Game/Wot/Actors/Props/Chest","Chest_Inn",(2650,1900,0),inn_yaw+180,1.0,None),
       ("mesh","/Game/Wot/Actors/Props/Chest","Chest_West",(-3300,1450,0),90,1.0,None),
       ("mesh","/Game/Wot/Actors/Props/BP_BreakableCrate","Crate1",(2350,1750,0),10,1.0,None),
       ("mesh","/Game/Wot/Actors/Props/BP_BreakableCrate","Crate2",(2480,1800,0),35,1.0,None),
       ("mesh","/Game/Wot/GeometryScripting/BP_AnimalPen","SheepPen",(-3000,2400,0),0,1.0,None),
       ("mesh","/Game/Wot/NPCs/animals/sheep/Sheep_BP","Sheep0",(-3250,2200,80),0,1.0,None),
       ("mesh","/Game/Wot/NPCs/animals/sheep/Sheep_BP","Sheep1",(-2850,2500,80),70,1.0,None),
       ("mesh","/Game/Wot/NPCs/animals/sheep/Sheep_BP","Sheep2",(-3050,2650,80),140,1.0,None),
       ("mesh","/Game/Wot/NPCs/animals/horse/HorseBP","InnHorse",(2500,2300,80),-60,1.0,None),
       ("mesh","/Game/Wot/Actors/Fireflies","Fireflies0",(-700,-900,150),0,1.0,None),
       ("mesh","/Game/Wot/Actors/Fireflies","Fireflies1",(900,700,150),0,1.0,None),
       ("mesh","/Game/Wot/Actors/Fireflies","Fireflies2",(300,-1500,150),0,1.0,None),
     ],
     "moves":[("Tam_NPC",(1250,700,96),-140),("Lan_NPC",(1500,450,121),160),("OverworldPortal",(-4750,-200,120),0),("OverworldPlayerStart",(-4250,-200,93),0)],
    }
