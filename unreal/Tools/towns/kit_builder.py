import math
# ---- composite voxel-style kit meshes, built from primitives and merged into StaticMesh assets ----
SPH="/Engine/BasicShapes/Sphere.Sphere"
def PM(n): return f"/Game/Wot/Towns/Common/Materials/{n}.{n}"
KIT_FOLDER="/Game/Wot/Towns/Common/Buildings"
SCRATCH=(0,0,-30000)   # parts are assembled far below the level, merged, and the sources destroyed

def box(x,y,z,sx,sy,sz,mat,yaw=0):
    """A cube scaled to (sx,sy,sz) cm, centred at (x,y,z) relative to the part pivot."""
    return (CUBE,(x,y,z),yaw,(sx/100.0,sy/100.0,sz/100.0),mat)
def cyl(x,y,z,d,h,mat):
    return (CYL,(x,y,z),0,(d/100.0,d/100.0,h/100.0),mat)

# Every generator returns a list of parts; pivot is the floor centre, door faces -X, min z = 0.
def gable_house(w=600,d=800,h=300,steps=5,wall="P_Plaster",roof="P_Terracotta",trim="P_DarkWood",door="P_DarkWood",win="P_Glass"):
    p=[box(0,0,h/2,d,w,h,wall)]
    rh=h+ (steps*60)/2
    for i in range(steps):           # stepped gable along +Y/-Y (ridge along X)
        f=1-(i/steps); ww=w*f+80; p.append(box(0,0,h+30+i*60,d+60,ww,60,roof))
    p.append(box(-d/2-5,0,h+30,d*0.06,w*0.9,60,trim))
    p.append(box(-d/2-3,0,110,20,120,220,door))
    for y in (-w*0.3,w*0.3): p.append(box(-d/2-2,y,190,10,90,90,win))
    for y in (-w*0.3,w*0.3): p.append(box(d/2+2,y,190,10,90,90,win))
    return p
def tower_house(w=450,d=450,floors=3,wall="P_Plaster",trim="P_DarkWood",roof="P_Slate",door="P_DarkWood",win="P_Glass"):
    fh=280; H=floors*fh; p=[box(0,0,H/2,d,w,H,wall)]
    for i in range(1,floors): p.append(box(0,0,i*fh,d+30,w+30,25,trim))
    p.append(box(0,0,H+25,d+60,w+60,50,roof)); p.append(box(0,0,H+90,d-60,w-60,80,roof))
    p.append(box(-d/2-3,0,110,20,120,220,door))
    for i in range(floors):
        for y in (-w*0.28,w*0.28):
            if i==0 and True: continue
            p.append(box(-d/2-2,y,i*fh+170,10,80,100,win))
    return p
def dome_house(w=600,wall="P_Cream",dome="P_Teal",door="P_DarkWood",band="P_Ochre"):
    h=280; p=[box(0,0,h/2,w,w,h,wall), box(0,0,h+10,w+40,w+40,20,band)]
    steps=[(w*0.95,70),(w*0.78,70),(w*0.58,70),(w*0.36,60),(w*0.16,50)]
    z=h+20
    for (dd,hh) in steps: p.append(cyl(0,0,z+hh/2,dd,hh,dome)); z+=hh
    p.append(box(-w/2-3,0,110,20,120,220,door))
    for y in (-w*0.32,w*0.32): p.append(box(-w/2-2,y,190,10,80,110,"P_DarkGrey"))
    return p
def courtyard_house(w=900,d=900,wall="P_Sand",roof="P_Terracotta",door="P_DarkWood"):
    t=180; h=300
    p=[box(-d/2+t/2,0,h/2,t,w,h,wall), box(d/2-t/2,0,h/2,t,w,h,wall), box(0,-w/2+t/2,h/2,d-2*t,t,h,wall), box(0,w/2-t/2,h/2,d-2*t,t,h,wall)]
    p+=[box(-d/2+t/2,0,h+25,t+40,w+40,50,roof), box(d/2-t/2,0,h+25,t+40,w+40,50,roof), box(0,-w/2+t/2,h+25,d-2*t,t+40,50,roof), box(0,w/2-t/2,h+25,d-2*t,t+40,50,roof)]
    p.append(box(-d/2-3,0,120,20,160,240,door))
    p.append(cyl(0,0,20,160,40,"P_LightGrey"))   # courtyard fountain base
    p.append(cyl(0,0,70,60,100,"P_Glass"))
    return p
def keep(w=1400,d=1400,h=700,wall="P_DarkGrey",cren="P_Grey",roof="P_Slate",tower_h=950):
    p=[box(0,0,h/2,d,w,h,wall), box(0,0,h+15,d+20,w+20,30,cren)]
    n=int(w//150)
    for i in range(n):
        y=-w/2+75+i*150
        if i%2==0: p.append(box(-d/2+40,y,h+60,80,100,90,cren)); p.append(box(d/2-40,y,h+60,80,100,90,cren))
    for sx in (-1,1):
        for sy in (-1,1):
            p.append(box(sx*(d/2-100),sy*(w/2-100),tower_h/2,320,320,tower_h,wall))
            p.append(box(sx*(d/2-100),sy*(w/2-100),tower_h+40,380,380,80,roof))
    p.append(box(-d/2-5,0,220,30,300,440,"P_DarkWood"))
    return p
def longhouse(w=500,d=1200,wall="P_Brown",roof="P_Olive",trim="P_DarkWood"):
    h=260; p=[box(0,0,h/2,d,w,h,wall)]
    for i in range(4): f=1-i/4; p.append(box(0,0,h+30+i*55,d+60,w*f+60,55,roof))
    p.append(box(-d/2-3,0,110,20,130,220,trim))
    for x in (-d*0.3,0,d*0.3): p.append(box(x,-w/2-2,180,110,10,80,"P_DarkGrey")); p.append(box(x,w/2+2,180,110,10,80,"P_DarkGrey"))
    return p
def market_stall(w=400,d=250,cloth="P_Terracotta",wood="P_DarkWood"):
    p=[box(0,0,80,d,w,20,wood)]
    for sx in (-1,1):
        for sy in (-1,1): p.append(box(sx*(d/2-20),sy*(w/2-20),140,30,30,280,wood))
    p.append(box(0,0,290,d+80,w+60,20,cloth))
    for i in range(3): p.append(box(-d/2-50,-w/2+40+i*(w-80)/2,100,60,60,40,"P_Ochre" if i!=1 else "P_Green"))
    return p
def well_stone(d=240,wall="P_Grey",wood="P_DarkWood",roof="P_Terracotta"):
    p=[cyl(0,0,50,d,100,wall), cyl(0,0,60,d-60,100,"P_Blue")]
    for sx in (-1,1): p.append(box(sx*(d/2-10),0,200,30,30,300,wood))
    p.append(box(0,0,360,d+60,120,30,roof)); p.append(box(0,0,400,d-40,80,50,roof))
    return p
def lamp_post(h=380,pole="P_DarkGrey",lamp="P_Gold"):
    return [box(0,0,h/2,24,24,h,pole), box(0,0,h+30,60,60,60,lamp), box(0,0,h+70,80,80,20,pole)]
def obelisk(h=900,mat="P_White",base="P_LightGrey"):
    return [box(0,0,30,260,260,60,base), box(0,0,h/2+60,120,120,h,mat), box(0,0,h+90,80,80,60,"P_Gold")]
def gate_house(w=700,h=500,wall="P_Grey",cren="P_LightGrey"):
    p=[box(0,-w/2,h/2,300,220,h,wall), box(0,w/2,h/2,300,220,h,wall), box(0,0,h-60,300,w+220,120,wall)]
    for y in (-w/2,0,w/2): p.append(box(0,y,h+40,120,120,80,cren))
    return p
def ruin_wall(w=700,wall="P_DarkGrey"):
    p=[box(0,-w/4,120,80,w/2,240,wall), box(0,w/4-60,70,80,w/2-120,140,wall), box(0,-w/2+40,200,80,80,160,wall)]
    for i in range(4): p.append(box(120+i*70,-w/3+i*110,30,60,60,60,wall))
    return p
def pier_post(): return [box(0,0,60,40,40,120,"P_DarkWood")]
def bridge_arch(span=1200,w=400,mat="P_LightGrey",rail="P_White"):
    p=[box(0,0,110,span,w,30,mat)]
    for sy in (-1,1): p.append(box(0,sy*(w/2-15),150,span,30,60,rail))
    for x in (-span/2+60,span/2-60): p.append(box(x,0,50,120,w,100,mat))
    return p
# ---- foliage ----
def pine(h=600,trunk="P_Trunk",leaf="P_DarkGreen",tiers=4,snow=None):
    p=[box(0,0,h*0.25,60,60,h*0.5,trunk)]
    z=h*0.35
    for i in range(tiers):
        f=1-i/tiers; s=320*f+60; hh=h*0.16
        p.append(box(0,0,z+hh/2,s,s,hh,leaf));
        if snow: p.append(box(0,0,z+hh,s*0.8,s*0.8,20,snow))
        z+=hh*0.8
    return p
def palm(h=700,trunk="P_Trunk",leaf="P_Green"):
    p=[]
    for i in range(6): p.append(box(0,0,i*h/6+h/12,70-i*5,70-i*5,h/6,trunk))
    for k in range(6):
        a=math.radians(k*60); p.append(box(130*math.cos(a),130*math.sin(a),h+10,260,90,24,leaf,yaw=k*60))
        p.append(box(230*math.cos(a),230*math.sin(a),h-30,120,70,20,leaf,yaw=k*60))
    return p
def dead_tree(h=520,mat="P_DarkGrey"):
    return [box(0,0,h/2,55,55,h,mat), box(90,0,h*0.8,200,45,45,mat), box(-70,40,h*0.65,160,40,40,mat,yaw=30), box(150,0,h*0.95,45,45,120,mat)]
def boulder(s=220,mat="P_Grey"):
    return [box(0,0,s*0.35,s,s*0.8,s*0.7,mat), box(s*0.3,s*0.2,s*0.3,s*0.6,s*0.6,s*0.6,mat,yaw=25), box(-s*0.25,-s*0.2,s*0.2,s*0.5,s*0.5,s*0.4,"P_DarkGrey",yaw=-20)]
def cactus(h=300,mat="P_Olive"):
    return [box(0,0,h/2,60,60,h,mat), box(80,0,h*0.6,100,50,50,mat), box(100,0,h*0.75,50,50,120,mat), box(-70,0,h*0.45,80,50,50,mat), box(-90,0,h*0.6,50,50,100,mat)]
def reeds(h=180,mat="P_Olive"):
    p=[]
    for i,(x,y) in enumerate([(0,0),(40,30),(-30,40),(20,-40),(-40,-20)]): p.append(box(x,y,h/2+i*8,16,16,h+i*16,mat))
    return p
def shrub_flower(mat="P_Green",flower="P_Rose"):
    return [box(0,0,50,120,120,100,mat), box(40,30,110,40,40,30,flower), box(-40,-20,105,40,40,30,flower), box(10,-45,112,40,40,30,"P_Gold")]
def snow_patch(): return [box(0,0,4,260,220,8,"P_Snow"), box(90,60,6,160,140,8,"P_Snow")]
def birch(h=550,trunk="P_White",leaf="P_Olive"):
    return [box(0,0,h/2,45,45,h,trunk), box(0,0,h*0.78,260,260,180,leaf), box(0,0,h*0.95,180,180,120,leaf), box(15,0,h*0.5,50,50,30,"P_DarkGrey")]

KIT={
 "SM_GableHouse":gable_house,"SM_TowerHouse":tower_house,"SM_DomeHouse":dome_house,"SM_CourtyardHouse":courtyard_house,
 "SM_Keep":keep,"SM_Longhouse":longhouse,"SM_MarketStall":market_stall,"SM_StoneWell":well_stone,"SM_LampPost":lamp_post,
 "SM_Obelisk":obelisk,"SM_GateHouse":gate_house,"SM_RuinWall":ruin_wall,"SM_BridgeArch":bridge_arch,
 "SM_Pine":pine,"SM_Palm":palm,"SM_DeadTree":dead_tree,"SM_Boulder":boulder,"SM_Cactus":cactus,"SM_Reeds":reeds,"SM_FlowerShrub":shrub_flower,"SM_SnowPatch":snow_patch,"SM_Birch":birch,
}

def stairs(rise=300,width=500,step_rise=25,step_run=50,mat="P_LightGrey",side="P_Grey"):
    """Voxel stairs climbing toward +X from the pivot (bottom front centre)."""
    n=int(rise/step_rise); p=[]
    for i in range(n):
        h=(i+1)*step_rise
        p.append(box(i*step_run+step_run/2,0,h/2,step_run,width,h,mat))
    L=n*step_run
    p.append(box(L/2,-width/2-15,rise*0.35,L,30,rise*0.7,side)); p.append(box(L/2,width/2+15,rise*0.35,L,30,rise*0.7,side))
    return p
def ladder(h=600,mat="P_DarkWood"):
    p=[box(0,-45,h/2,15,15,h,mat),box(0,45,h/2,15,15,h,mat)]
    for i in range(int(h/60)): p.append(box(0,0,40+i*60,12,100,12,mat))
    return p
KIT.update({"SM_Stairs200":lambda: stairs(200),"SM_Stairs300":lambda: stairs(300),"SM_Stairs400":lambda: stairs(400),"SM_Stairs600":lambda: stairs(600),"SM_Ladder600":lambda: ladder(600),"SM_Ladder900":lambda: ladder(900)})

def build_kit_mesh(name,parts,folder=KIT_FOLDER):
    """Places parts as StaticMeshActors in the scratch area, merges them into one StaticMesh asset, destroys sources."""
    actors=[]
    ox,oy,oz=SCRATCH
    # tiny pivot part first so the merged mesh pivot sits at the floor centre
    parts=[box(0,0,1,20,20,2,parts[0][4])]+list(parts)
    for i,(mesh,(x,y,z),yaw,scale,mat) in enumerate(parts):
        a=place_mesh(mesh,f"{name}_part{i}",(ox+x,oy+y,oz+z),yaw,scale,[PM(mat)] if mat else None,tag=None)
        actors.append(a)
    if T(AS+"exists",{"path":f"{folder}/{name}"}): T(AS+"delete",{"path":f"{folder}/{name}"})
    base=name[3:] if name.startswith("SM_") else name   # the tool prefixes SM_ itself
    r=T(SC+"merge_actors",{"actors":[ref(a) for a in actors],"output_path":f"{folder}/{base}","name":name,"destroy_source_actors":True})
    # remove the merged actor from the level, keep the asset
    try:
        ap=r["refPath"] if isinstance(r,dict) else None
        if ap: remove(ap)
    except Exception: pass
    # assign slot materials in first-appearance order of distinct part materials
    order=kit_slot_mats(parts)
    mp=f"{folder}/{name}.{name}"
    try:
        slots=T("editor_toolset.toolsets.static_mesh.StaticMeshTools.get_material_slots",{"mesh":ref(mp)})
        for sl,m in zip(slots,order):
            T("editor_toolset.toolsets.static_mesh.StaticMeshTools.set_material",{"mesh":ref(mp),"slot_name":sl,"material":ref(PM(m))})
    except Exception as e:
        return f"{folder}/{name} (slot err {str(e)[:60]})"
    return f"{folder}/{name}"
def kit_slot_mats(parts):
    order=[]
    for part in parts:
        m=part[4]
        if m not in order: order.append(m)
    return order
def kit_entry(name,recolor=None,weight=1):
    """Weighted spawner entry for a kit mesh with optional {material->material} recolour."""
    parts=[box(0,0,1,20,20,2,KIT[name]()[0][4])]+list(KIT[name]())
    order=kit_slot_mats(parts)
    mats=[PM((recolor or {}).get(m,m)) for m in order] if recolor else None
    return (f"{KIT_FOLDER}/{name}.{name}",weight,mats)
def kit_mesh(name): return f"{KIT_FOLDER}/{name}.{name}"
def kit_mats(name,recolor=None):
    parts=[box(0,0,1,20,20,2,KIT[name]()[0][4])]+list(KIT[name]())
    return [PM((recolor or {}).get(m,m)) for m in kit_slot_mats(parts)]
