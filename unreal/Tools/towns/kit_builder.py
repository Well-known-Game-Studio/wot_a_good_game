import math
# ---- composite voxel-style kit meshes, built from primitives and merged into StaticMesh assets ----
SPH="/Engine/BasicShapes/Sphere.Sphere"
def PM(n): return f"/Game/Wot/Towns/Common/Materials/{n}.{n}"
KIT_FOLDER="/Game/Wot/Towns/Common/Buildings"
SCRATCH=(0,0,-30000)   # parts are assembled far below the level, merged, and the sources destroyed

def box(x,y,z,sx,sy,sz,mat,yaw=0,pitch=0,roll=0):
    """A cube scaled to (sx,sy,sz) cm, centred at (x,y,z) relative to the part pivot; rotation (pitch,yaw,roll)."""
    return (CUBE,(x,y,z),(pitch,yaw,roll) if (pitch or roll) else yaw,(sx/100.0,sy/100.0,sz/100.0),mat)
def cyl(x,y,z,d,h,mat):
    return (CYL,(x,y,z),0,(d/100.0,d/100.0,h/100.0),mat)

# Every generator returns a list of parts; pivot is the floor centre, door faces -X, min z = 0.
# A part whose material is None is an existing mesh asset placed as-is (no palette material): see rand_house.
RAND_HOUSE_SRC="/Game/Wot/Meshes/Buildings/rand_house/rand_house.rand_house"
def rand_house():
    """The imported Two Rivers house (rand_house) re-merged with a half turn so its door, awning and windows face -X like
    every other kit house. House slots point +X away from the street, so the original mesh stood with its back to the
    road. Keeps the source's five material slots (roof, wood, wall, window, awning) in order."""
    return [(RAND_HOUSE_SRC,(0,0,0),180,(1,1,1),None)]
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

# ---------------- the Ways: dark decayed stone built by Ogier and Aes Sedai, floating in a void ----------------
class WotRandom:
    """Deterministic LCG with the subset of random.Random the generators use (the toolset sandbox forbids `random`)."""
    def __init__(self,seed=1): self.s=(int(seed)*2654435761+12345)&0xFFFFFFFF
    def random(self):
        self.s=(self.s*1664525+1013904223)&0xFFFFFFFF; return self.s/4294967296.0
    def uniform(self,a,b): return a+(b-a)*self.random()
    def choice(self,seq): return seq[int(self.random()*len(seq))%len(seq)]
    def randint(self,a,b): return a+int(self.random()*(b-a+1))
    def randrange(self,n): return int(self.random()*n)%max(1,n)
class _RndShim:
    Random=WotRandom
_rnd=_RndShim()
def way_island(radius=1400,thick=320,seed=1,stone="P_WayStone",dark="P_WayStoneDark"):
    """A ragged disc of blackened stone (pivot at the walking surface centre): a top plate of 100 cm voxels with
    crumbled rim, a thicker underside that tapers, and a few pits. Top surface at z=0."""
    r=_rnd.Random(seed); p=[]; cell=100
    n=int(radius/cell)
    for i in range(-n,n+1):
        for j in range(-n,n+1):
            x=i*cell; y=j*cell; d=math.hypot(x,y)
            edge=radius*(0.86+0.14*math.sin(3*math.atan2(y,x)+seed)+0.05*r.random())
            if d>edge: continue
            if d>edge-160 and r.random()<0.35: continue           # crumbled rim
            h=cell*(1 if d<edge-250 or r.random()<0.6 else 0.6)
            p.append(box(x,y,-h/2,cell,cell,h,stone if r.random()<0.75 else dark))
    # underside: stepped taper
    for k,(f,t) in enumerate(((0.8,thick*0.45),(0.55,thick*0.35),(0.3,thick*0.2))):
        z0=-cell-sum(tt for _,tt in (((0.8,thick*0.45),(0.55,thick*0.35),(0.3,thick*0.2))[:k]))
        p.append(cyl(0,0,z0-t/2,radius*2*f,t,dark))
    return p
def way_bridge(length=1200,width=500,seed=3,stone="P_WayStone",dark="P_WayStoneDark"):
    """One bridge span (pivot at the near end, centre, deck top at z=0): deck slabs 200 long with a missing or
    sunken slab now and then, thin side kerbs and a sagging underside."""
    r=_rnd.Random(seed); p=[]; slab=200; n=int(length/slab)
    for i in range(n):
        x=i*slab+slab/2; roll=r.random()
        drop=0 if roll>0.3 else r.choice((0,10,20,30))           # sunken slabs instead of gaps: the navmesh must stay continuous
        p.append(box(x,0,-30-drop,slab-6,width,60,stone if roll>0.25 else dark))
    for sg in (1,-1):                                            # low flush kerb stones (nothing above the deck: the collision hull must stay flat)
        for i in range(n):
            if r.random()<0.7: p.append(box(i*slab+slab/2,sg*(width/2-20),-30,slab-6,40,60,dark))
    p.append(box(length/2,0,-120,length-100,width*0.5,120,dark))
    return p
def guiding(h=520,w=260,stone="P_GuidingPale",script="P_GuidingScript",base="P_WayStoneDark"):
    """A Guiding: tall pale slab with Ogier script (emissive) on both faces, on a stepped base. Pivot at the base centre."""
    p=[box(0,0,30,w+120,160,60,base),box(0,0,80,w+40,110,40,base),box(0,0,100+h/2,w,70,h,stone)]
    for k in range(6):
        z=160+k*(h-80)/6
        for face in (1,-1): p.append(box(face*38,-w/2+40+(k%3)*60,z,8,w-80-(k%3)*60,22,script))
    p.append(box(0,0,h+100+15,w+30,90,30,stone))
    return p
def waygate(w=420,h=560,frame="P_WayStone",dark="P_WayStoneDark",leaf="P_GuidingPale"):
    """A Waygate: a carved arch of pale stone around a dark doorway, the Avendesora leaf above. Opening faces -X. Pivot floor centre."""
    p=[box(0,-w/2-60,h/2,120,120,h,frame),box(0,w/2+60,h/2,120,120,h,frame),box(0,0,h+60,120,w+240,120,frame)]
    p.append(box(40,0,h/2,20,w,h,dark))                           # the door itself (dark)
    for k in range(5): p.append(box(-10,-w/2-70+k*((w+140)/4),h+130,60,40,40,leaf))
    p.append(box(-50,0,h+180,30,60,80,leaf)); p.append(box(-50,0,h+230,30,120,30,leaf))
    p.append(box(0,0,-10,260,w+300,20,dark))
    return p
def way_pillar(h=700,seed=5,stone="P_WayStone",dark="P_WayStoneDark"):
    """A broken pillar; height varies with seed. Pivot floor centre."""
    r=_rnd.Random(seed); hh=h*(0.5+0.5*r.random()); p=[box(0,0,20,180,180,40,dark)]
    z=40
    while z<hh:
        seg=min(120,hh-z); p.append(box(r.uniform(-8,8),r.uniform(-8,8),z+seg/2,120,120,seg,stone if r.random()<0.7 else dark)); z+=seg
    return p
def way_rubble(seed=7,dark="P_WayStoneDark",stone="P_WayStone"):
    r=_rnd.Random(seed); p=[]
    for k in range(6):
        sz=r.uniform(40,110); p.append(box(r.uniform(-120,120),r.uniform(-120,120),sz/2,sz,sz*r.uniform(0.6,1.2),sz,dark if r.random()<0.6 else stone,r.uniform(0,90)))
    return p

KIT={
 "SM_GableHouse":gable_house,"SM_TowerHouse":tower_house,"SM_DomeHouse":dome_house,"SM_CourtyardHouse":courtyard_house,"SM_RandHouse":rand_house,
 "SM_Keep":keep,"SM_Longhouse":longhouse,"SM_MarketStall":market_stall,"SM_StoneWell":well_stone,"SM_LampPost":lamp_post,
 "SM_Obelisk":obelisk,"SM_GateHouse":gate_house,"SM_RuinWall":ruin_wall,"SM_BridgeArch":bridge_arch,
 "SM_WayIsland":way_island,"SM_WayIsland2":lambda:way_island(1100,300,2),"SM_WayIsland3":lambda:way_island(1700,380,3),
 "SM_WayBridge":way_bridge,"SM_WayBridge2":lambda:way_bridge(1200,500,8),"SM_Guiding":guiding,"SM_Waygate":waygate,
 "SM_WayPillar":way_pillar,"SM_WayPillar2":lambda:way_pillar(700,9),"SM_WayRubble":way_rubble,
 "SM_Pine":pine,"SM_Palm":palm,"SM_DeadTree":dead_tree,"SM_Boulder":boulder,"SM_Cactus":cactus,"SM_Reeds":reeds,"SM_FlowerShrub":shrub_flower,"SM_SnowPatch":snow_patch,"SM_Birch":birch,
}

def stairs(rise=300,width=500,step_rise=20,step_run=40,mat="P_LightGrey",side="P_Grey"):
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
def white_bridge(span=4000,rise=650,width=700,n=24,mat="P_White",rail="P_Ice",pier="P_LightGrey"):
    """The White Bridge: a tall glassy arch (deck segments along a parabola, pivot at the centre of the span at deck-foot level)."""
    p=[]; seg=span/n
    for i in range(n):
        x0=-span/2+i*seg; x1=x0+seg; xm=(x0+x1)/2
        f=lambda x: rise*(1-(2*x/span)**2)
        z0=f(x0); z1=f(x1); zm=(z0+z1)/2; L=math.hypot(seg,z1-z0); pitch=math.degrees(math.atan2(z1-z0,seg))
        p.append(box(xm,0,zm+20,L+6,width,40,mat,pitch=pitch))
        for sy in (-1,1): p.append(box(xm,sy*(width/2-20),zm+90,L+6,36,100,rail,pitch=pitch))
        if i%3==1 and abs(xm)<span*0.42: p.append(box(xm,0,zm/2,80,width*0.6,max(zm-40,20),pier))
    for sx in (-1,1):
        p.append(box(sx*(span/2+80),0,120,200,width+160,240,pier)); p.append(box(sx*(span/2+80),0,280,160,width+100,60,mat))
    return p
KIT.update({f"SM_Stairs{r}":(lambda r=r: stairs(r)) for r in (200,240,280,320,360,400,480,600)})
KIT.update({"SM_Ladder600":lambda: ladder(600),"SM_Ladder900":lambda: ladder(900),"SM_WhiteBridge":white_bridge})

def build_kit_mesh(name,parts,folder=KIT_FOLDER):
    """Places parts as StaticMeshActors in the scratch area, merges them into one StaticMesh asset, destroys sources."""
    actors=[]
    ox,oy,oz=SCRATCH
    # tiny pivot part first so the merged mesh pivot sits at the floor centre (a wrapped asset keeps its own pivot and materials)
    wrapped=parts[0][4] is None
    if not wrapped: parts=[box(0,0,1,20,20,2,parts[0][4])]+list(parts)
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
    mp=f"{folder}/{name}.{name}"
    if wrapped: return f"{folder}/{name}"
    order=kit_slot_mats(parts)
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
