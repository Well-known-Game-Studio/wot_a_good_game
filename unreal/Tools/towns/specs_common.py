# ---------------- shared helpers for the town specs (v3: relief terrain) ----------------
H=MESH["house"]                       # the imported Two Rivers house: door on +X (hand-placed inns with an authored yaw)
HK=kit_mesh("SM_RandHouse")           # the same house turned to the kit convention (door -X): every street-house entry
PORTAL_BP="/Game/Wot/Actors/Portal"; CHEST="/Game/Wot/Actors/Props/Chest"; CRATE="/Game/Wot/Actors/Props/BP_BreakableCrate"; CAMP="/Game/Wot/Actors/Props/Campfire"; FIRE="/Game/Wot/Actors/Fireflies"
HORSE="/Game/Wot/NPCs/animals/horse/HorseBP"; SHEEP="/Game/Wot/NPCs/animals/sheep/Sheep_BP"; PEN="/Game/Wot/GeometryScripting/BP_AnimalPen"
GRASS=MAT("MI_Ground_Lush"); ROCK=MAT("MI_Ground_Rock"); DARKROCK=MAT("MI_Ground_DarkRock"); CLIFF=MAT("MI_Ground_Cliff"); REDROCK=MAT("MI_Ground_RedRock"); SAND=MAT("MI_Ground_Sand"); SNOW=PM("G_Snow"); TUNDRA=MAT("MI_Ground_Tundra"); ASH=MAT("MI_Ground_Ash"); DIRT=MAT("MI_Ground_Dirt")
def grid_streets(x0,x1,y0,y1,nx,ny,width=5,houses=True,spacing=1300,setback=850,set_=None,prefix="st",wig=0,seed=1):
    """Rectilinear street grid between (x0,y0)-(x1,y1): nx vertical and ny horizontal streets (optionally meandering)."""
    out=[]
    for i in range(nx):
        x=x0+(x1-x0)*i/(nx-1) if nx>1 else (x0+x1)/2
        pts=[(x,y0),(x,(y0+y1)/2),(x,y1)]
        if wig: pts=wiggle(pts,wig,seed+i,2)
        r={"name":f"{prefix} v{i}","pts":pts,"width":width,"houses":houses,"spacing":spacing,"setback":setback,"linear":not wig}
        if set_: r["set"]=set_
        out.append(r)
    for j in range(ny):
        y=y0+(y1-y0)*j/(ny-1) if ny>1 else (y0+y1)/2
        pts=[(x0,y),((x0+x1)/2,y),(x1,y)]
        if wig: pts=wiggle(pts,wig,seed+10+j,2)
        r={"name":f"{prefix} h{j}","pts":pts,"width":width,"houses":houses,"spacing":spacing,"setback":setback,"linear":not wig}
        if set_: r["set"]=set_
        out.append(r)
    return out
def radial_streets(cx,cy,r0,r1,n,width=5,houses=True,spacing=1400,setback=900,start=0.0,set_=None,prefix="radial"):
    out=[]
    for k in range(n):
        th=start+2*math.pi*k/n; t=(math.cos(th),math.sin(th))
        r={"name":f"{prefix} {k}","pts":[(cx+r0*t[0],cy+r0*t[1]),(cx+(r0+r1)/2*t[0],cy+(r0+r1)/2*t[1]),(cx+r1*t[0],cy+r1*t[1])],"width":width,"houses":houses,"spacing":spacing,"setback":setback}
        if set_: r["set"]=set_
        out.append(r)
    return out
def lamp_line(step=1400,offset=None,mat="P_Gold"):
    return {"mesh":kit_mesh("SM_LampPost"),"step":step,"offset":offset or 380,"mat":None,"scale":(1,1,1)}
def tree_line(step=1200,offset=420,mesh=None):
    return {"mesh":mesh or MESH["tree3"],"step":step,"offset":offset,"mat":None,"scale":(0.9,0.9,0.9)}
def base_foliage(trees,forest_ratio=0.2,edge_ratio=0.05,town_ratio=0.008,bush=0.02,grass=0.3,mush=0.0,extent=5000,fr=None,clip=1.0,town_trees=None,extra=None,bands=None,ground="temperate"):
    fr=fr or extent-400; bush=bush*2.0; grass=min(1.0,grass*1.6); town_ratio=town_ratio*2.0; extra=(extra or [])+ground_extras(ground)
    b=bands or {"forest":(0.84,1.0),"edge":(0.6,0.84),"town":(0.2,0.6)}
    return {"extent":extent,"forest_radius":fr,"clip_density":clip,"trees":trees,
            "forest_band":b["forest"],"forest_ratio":forest_ratio,"edge_band":b["edge"],"edge_ratio":edge_ratio,"town_band":b["town"],"town_ratio":town_ratio,
            "town_trees":town_trees or trees,"bush_ratio":bush,"grass_ratio":grass,"mush_ratio":mush,"extra":extra or []}
TREES_TEMPERATE=[(MESH["tree1"],3,None),(MESH["tree2"],3,None),(MESH["tree3"],2,None),(MESH["tree4"],2,None),(MESH["oldtree"],1,None),kit_entry("SM_Oak",None,2),kit_entry("SM_Oak",{"P_LeafGreen":"P_LeafDark","P_LeafPale":"P_LeafGreen"},1),kit_entry("SM_AppleTree",None,1)]
TREES_FORMAL=[(MESH["tree3"],3,None),(MESH["tree4"],3,None),(kit_mesh("SM_Birch"),2,None),kit_entry("SM_Oak",None,1)]
def TREES_PINE(snow=False):
    if snow: return [kit_entry("SM_Pine",{"P_LeafDark":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_LeafDark":"P_Ice"},1),(MESH["tree4"],1,None)]
    return [kit_entry("SM_Pine",{"P_LeafDark":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_LeafDark":"P_Olive"},1),(MESH["tree4"],1,None)]
TREES_DESERT=[kit_entry("SM_Palm",None,3),kit_entry("SM_Palm",{"P_LeafGreen":"P_Olive"},2),(MESH["tree3"],1,None)]
TREES_DEAD=[kit_entry("SM_DeadTree",None,4),kit_entry("SM_DeadTree",{"P_DarkGrey":"P_Black"},2)]
def level_scale(size): return (size/200.0,size/200.0,10)
def waygate(town,x,y,yaw,to_level,from_tag,to_tag,z=0):
    """A Waygate into the Ways: the carved arch, a Portal actor (ToLevel/PortalName) and a tagged PlayerStart in front of it.
    from_tag is this gate's PortalName (<Town>Ways), to_tag the PlayerStart tag arrivals from the Ways use (Ways<Town>)."""
    dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw))
    return [("mesh",kit_mesh("SM_Waygate"),f"Waygate_{town}",(x,y,z),yaw,1.0,None),
            ("actor",PORTAL_BP,f"WaysPortal_{town}",(x-dx*120,y-dy*120,z+20),yaw,1.0,{"ToLevel":to_level,"PortalName":from_tag,"Enabled":True}),
            ("class","/Script/Engine.PlayerStart",f"WaysPlayerStart_{town}",(x-dx*320,y-dy*320,z+95),yaw+180,1.0,{"PlayerStartTag":to_tag})]
def portal_moves(x,y,yaw):
    """Portal and player start at the town entrance, facing yaw (player looks along yaw into town). z is relative to the ground."""
    dx=math.cos(math.radians(yaw)); dy=math.sin(math.radians(yaw))
    return [("OverworldPortal",(x-dx*250,y-dy*250,20),yaw),("OverworldPlayerStart",(x+dx*200,y+dy*200,95),yaw)]
def rand_house_entries(walls,roofs,woods=("MI_Wood_Dark","MI_Wood_Grey"),w=1):
    e=[]
    for i,wl in enumerate(walls):
        for j,rf in enumerate(roofs):
            e.append((HK,w,[MAT(rf),MAT(woods[(i+j)%len(woods)]),MAT(wl),None,None]))
    return e
def dense_set(entries,spacing=850,setback=600,scale=(0.55,0.72),keep=0.97,footprint=(-480,-330,400,380)):
    return {"spacing":spacing,"setback":setback,"scale":scale,"keep":keep,"footprint":footprint,"jitter":40,"entries":entries}
def pier_roads(prefix,x0,ys,length,water,width=3,along_x=True):
    out=[]
    for k,yy in enumerate(ys):
        pts=[(x0,yy,water+12),(x0+length*0.5,yy,water+12),(x0+length,yy,water+12)] if along_x else [(yy,x0,water+12),(yy,x0+length*0.5,water+12),(yy,x0+length,water+12)]
        out.append({"name":f"{prefix} {k}","pts":pts,"width":width,"houses":False,"mat":WOODMAT,"tile_z":0,"tile_scale":(1,1,0.1),"nocarve":True,"linear":True})
    return out
def boulders_extra(band=(0.75,1.0),ratio=0.02,recolor=None,scale=(0.8,1.6)):
    return {"name":"Boulders","band":band,"ratio":ratio,"entries":[kit_entry("SM_Boulder",recolor,3),kit_entry("SM_Boulder",{"P_Grey":"P_DarkGrey"},1)],"scale":scale,"prune":200}
def town_base(name,E,center=(0,0),road_mat=None):
    return {"town":name,"level_scale":level_scale(2*E),"graph_name":f"PCG_{name}","folder":f"/Game/Wot/Towns/{name}","center":center,"road_mat":road_mat or MAT("MI_Tile_CobbleMid"),
            "plazas":[],"exclusions":[],"roads":[],"props":[],"moves":[],"pads":[],"house_sets":{}}

# ---- the art pass: regional house families from the kit (kit_builder timber_house / stone_house / adobe_house ...).
# kit_entry recolours by palette name; keys the mesh does not use are ignored. Weights are relative within a town.
YARD_FOOTPRINT=(-800,-600,780,600)   # house sets with yard variants need this footprint (fences, laundry, coops around the house)
def two_rivers_entries(yards=True):
    """Two Rivers villages: timber and thatch with yards, dormered cottages, an L-shaped house, a long farmhouse, a barn."""
    Y=lambda a,b: a if yards else b
    return [kit_entry(Y("SM_TimberHouseYard","SM_TimberHouse"),None,3),kit_entry("SM_TimberHouse",{"P_Plaster":"P_Cream","P_DarkGreen":"P_Brown"},2),
            kit_entry(Y("SM_CottageYard","SM_ThatchedCottage"),None,2),kit_entry("SM_ThatchedCottage",{"P_ThatchDark":"P_Straw","P_Stone":"P_Grey"},1),
            kit_entry(Y("SM_CottageDormerYard","SM_CottageDormer"),None,2),kit_entry("SM_CottageDormer",{"P_Plaster":"P_Cream","P_Straw":"P_ThatchDark"},1),
            kit_entry("SM_TimberHouseShingle",None,1),kit_entry(Y("SM_TimberHouse2Yard","SM_TimberHouse2"),{"P_Terracotta":"P_Straw","P_DarkGreen":"P_Olive"},1),
            kit_entry(Y("SM_LHouseYard","SM_LHouse"),None,1),kit_entry(Y("SM_FarmhouseYard","SM_Farmhouse"),None,1),kit_entry("SM_Barn",None,1)]
def andor_entries(white=False):
    w={"P_Plaster":"P_White","P_Cream":"P_White"} if white else {}
    return [kit_entry("SM_TimberHouse2",w,3),kit_entry("SM_TimberHouseSlate",w,2),kit_entry("SM_StoneHouse",None,2),kit_entry("SM_StoneHouse3",{"P_Slate":"P_Terracotta"},1),
            kit_entry("SM_StoneTimberHouse",w,3),kit_entry("SM_StoneTimberHouse",dict(w,**{"P_Slate":"P_Terracotta"}),1),
            kit_entry("SM_Shop",{"P_Terracotta":"P_RedBrick"},2),kit_entry("SM_Shop",{"P_Terracotta":"P_Blue","P_Cream":"P_White"},1)]
def stone_town_entries(wall="P_Stone",roof="P_Slate",quoin="P_LightGrey",shops=True):
    rc={"P_Stone":wall,"P_Slate":roof,"P_LightGrey":quoin}
    e=[kit_entry("SM_StoneHouse",rc,3),kit_entry("SM_StoneHouse3",rc,2),kit_entry("SM_StoneTimberHouse",rc,2),kit_entry("SM_TimberHouseSlate",{"P_Slate":roof},1)]
    if shops: e.append(kit_entry("SM_Shop",{"P_Slate":roof},1))
    return e
def adobe_entries(wall="P_Adobe",trim="P_Clay"):
    return [kit_entry("SM_AdobeHouse",{"P_Adobe":wall,"P_Clay":trim},3),kit_entry("SM_AdobeHouse2",{"P_Adobe":wall,"P_Clay":trim},2),kit_entry("SM_AdobeTower",{"P_Adobe":wall,"P_Clay":trim},2)]
# plaza furniture extras: carts, hay, barrels, crates, benches, troughs, barrows, festival poles (plaza_furniture spreads unknown kinds on a grid)
def market_extras(ratio=0.02,poles=False):
    e=[(kit_mesh("SM_Cart"),ratio,0.9,1.1,None),(kit_mesh("SM_Barrel"),ratio*1.5,0.9,1.1,None),(kit_mesh("SM_CrateStack"),ratio,0.9,1.1,None),(kit_mesh("SM_HayBale"),ratio*0.7,0.9,1.1,None),(kit_mesh("SM_Bench"),ratio,1.0,1.0,None),
       (kit_mesh("SM_WaterTrough"),ratio*0.6,1.0,1.0,None),(kit_mesh("SM_Wheelbarrow"),ratio*0.6,1.0,1.0,None),(kit_mesh("SM_LogBench"),ratio*0.5,1.0,1.0,None)]
    if poles: e.append((kit_mesh("SM_FestivalPole"),ratio*0.5,1.0,1.0,None))
    return e
# ---- ground cover and farmland (the "lived in" pass): one scatter per kind, all from the ground lattice
def ground_extras(biome="temperate"):
    """Dense ground cover: grass tufts, flower patches, small rocks, round bushes; the forest band gets stumps, fallen
    logs and mushroom clusters. Snow and desert biomes get their own sparser mixes."""
    if biome=="temperate":
        return [{"name":"GroundTufts","band":(0.0,1.0),"ratio":0.12,"entries":[kit_entry("SM_GrassTuft",None,3),kit_entry("SM_GrassTuft",{"P_LeafPale":"P_LeafGreen"},1)],"scale":(0.7,1.3),"prune":40},
                {"name":"GroundFlowers","band":(0.0,0.9),"ratio":0.05,"entries":[kit_entry("SM_FlowerPatch",None,2),kit_entry("SM_FlowerPatch",{"P_Rose":"P_Blue","P_Gold":"P_White"},1),kit_entry("SM_FlowerShrub",None,1)],"scale":(0.8,1.2),"prune":60},
                {"name":"GroundRocks","band":(0.3,1.0),"ratio":0.03,"entries":[kit_entry("SM_RockSmall",None,3),kit_entry("SM_Boulder",None,1)],"scale":(0.6,1.3),"prune":80},
                {"name":"GroundRoundBushes","band":(0.2,1.0),"ratio":0.04,"entries":[kit_entry("SM_BushRound",None,2),kit_entry("SM_BushRound",{"P_LeafGreen":"P_LeafDark"},1)],"scale":(0.7,1.3),"prune":90},
                {"name":"GroundDeadfall","band":(0.75,1.0),"ratio":0.04,"entries":[kit_entry("SM_Stump",None,2),kit_entry("SM_FallenLog",None,2),kit_entry("SM_MushroomCluster",None,2)],"scale":(0.8,1.3),"prune":120}]
    if biome=="snow":
        return [{"name":"GroundTufts","band":(0.0,1.0),"ratio":0.05,"entries":[kit_entry("SM_GrassTuft",{"P_LeafPale":"P_Straw","P_LeafGreen":"P_LeafOlive"},1)],"scale":(0.6,1.0),"prune":40},
                {"name":"GroundRocks","band":(0.3,1.0),"ratio":0.04,"entries":[kit_entry("SM_RockSmall",None,3),kit_entry("SM_Boulder",{"P_Grey":"P_LightGrey"},1)],"scale":(0.6,1.4),"prune":80},
                {"name":"GroundDeadfall","band":(0.7,1.0),"ratio":0.04,"entries":[kit_entry("SM_Stump",None,2),kit_entry("SM_FallenLog",{"P_Olive":"P_Snow"},2)],"scale":(0.8,1.3),"prune":120}]
    if biome=="desert":
        return [{"name":"GroundTufts","band":(0.0,1.0),"ratio":0.03,"entries":[kit_entry("SM_GrassTuft",{"P_LeafPale":"P_Straw","P_LeafGreen":"P_LeafOlive"},1)],"scale":(0.6,1.0),"prune":40},
                {"name":"GroundRocks","band":(0.2,1.0),"ratio":0.05,"entries":[kit_entry("SM_RockSmall",{"P_Grey":"P_Sand","P_DarkGrey":"P_Clay"},3),kit_entry("SM_Boulder",{"P_Grey":"P_Clay"},1)],"scale":(0.6,1.4),"prune":80}]
    return []
def field_extras(ratio=0.015,band=(0.55,0.85)):
    """Tabac and vegetable plots, scarecrows and bee skeps in the farmland ring outside a Two Rivers village."""
    return [{"name":"Fields","band":band,"ratio":ratio,"entries":[kit_entry("SM_CropRows",None,4),kit_entry("SM_CropRows",{"P_LeafGreen":"P_Olive","P_Olive":"P_Straw"},1),kit_entry("SM_Scarecrow",None,1),kit_entry("SM_Beehive",None,1)],"scale":(0.9,1.4),"prune":350}]
def fence_line(step=300,offset=430,sides="both",mesh="SM_PicketFence"):
    """A continuous fence / hedge / dry-stone wall along a road (relief leaves gaps at house fronts: line_house_clear)."""
    return {"mesh":kit_mesh(mesh),"step":step,"offset":offset,"mat":None,"scale":(1,1,1),"sides":sides}
def lantern_line(step=1600,offset=330,sides="both"):
    return {"mesh":kit_mesh("SM_LanternPost"),"step":step,"offset":offset,"mat":None,"scale":(1,1,1),"sides":sides}
