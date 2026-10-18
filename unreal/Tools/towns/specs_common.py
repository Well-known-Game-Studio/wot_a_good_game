# ---------------- shared helpers for the town specs (v3: relief terrain) ----------------
H=MESH["house"]                       # the imported Two Rivers house: door on +X (hand-placed inns with an authored yaw)
HK=kit_mesh("SM_RandHouse")           # the same house turned to the kit convention (door -X): every street-house entry
PORTAL_BP="/Game/Wot/Actors/Portal"; CHEST="/Game/Wot/Actors/Props/Chest"; CRATE="/Game/Wot/Actors/Props/BP_BreakableCrate"; CAMP="/Game/Wot/Actors/Props/Campfire"; FIRE="/Game/Wot/Actors/Fireflies"
HORSE="/Game/Wot/NPCs/animals/horse/HorseBP"; SHEEP="/Game/Wot/NPCs/animals/sheep/Sheep_BP"; PEN="/Game/Wot/GeometryScripting/BP_AnimalPen"
GRASS=MAT("MI_Ground_Lush"); ROCK=MAT("MI_Ground_Rock"); DARKROCK=MAT("MI_Ground_DarkRock"); CLIFF=MAT("MI_Ground_Cliff"); REDROCK=MAT("MI_Ground_RedRock"); SAND=MAT("MI_Ground_Sand"); SNOW=PM("P_Snow"); TUNDRA=MAT("MI_Ground_Tundra"); ASH=MAT("MI_Ground_Ash"); DIRT=MAT("MI_Ground_Dirt")
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
def base_foliage(trees,forest_ratio=0.2,edge_ratio=0.05,town_ratio=0.008,bush=0.02,grass=0.3,mush=0.0,extent=5000,fr=None,clip=1.0,town_trees=None,extra=None,bands=None):
    fr=fr or extent-400
    b=bands or {"forest":(0.84,1.0),"edge":(0.6,0.84),"town":(0.2,0.6)}
    return {"extent":extent,"forest_radius":fr,"clip_density":clip,"trees":trees,
            "forest_band":b["forest"],"forest_ratio":forest_ratio,"edge_band":b["edge"],"edge_ratio":edge_ratio,"town_band":b["town"],"town_ratio":town_ratio,
            "town_trees":town_trees or trees,"bush_ratio":bush,"grass_ratio":grass,"mush_ratio":mush,"extra":extra or []}
TREES_TEMPERATE=[(MESH["tree1"],3,None),(MESH["tree2"],3,None),(MESH["tree3"],2,None),(MESH["tree4"],2,None),(MESH["oldtree"],1,None)]
TREES_FORMAL=[(MESH["tree3"],3,None),(MESH["tree4"],3,None),(kit_mesh("SM_Birch"),2,None)]
def TREES_PINE(snow=False):
    if snow: return [kit_entry("SM_Pine",{"P_DarkGreen":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_DarkGreen":"P_Ice"},1),(MESH["tree4"],1,None)]
    return [kit_entry("SM_Pine",{"P_DarkGreen":"P_DarkGreen"},3),kit_entry("SM_Pine",{"P_DarkGreen":"P_Olive"},1),(MESH["tree4"],1,None)]
TREES_DESERT=[kit_entry("SM_Palm",None,3),kit_entry("SM_Palm",{"P_Green":"P_Olive"},2),(MESH["tree3"],1,None)]
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
