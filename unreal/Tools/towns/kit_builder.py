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
# ---- the art pass (issues #34 #35 #51): regional building families and street props. Same rules: pivot floor centre,
# door on -X, min z 0; every part a box/cyl in the palette so towns recolour them with kit_entry(name, {old: new}).
def _roof_gable(p,d,w,z,mat,steps=5,step_h=55,eave=60,ridge="X",trim=None,thatch=False):
    """A stepped voxel gable roof over a d x w footprint starting at height z. ridge "X": the roof steps shrink along Y
    (gable ends face +-X, the street); "Y": they shrink along X. Thatch gets a fatter, rounder stack and deeper eaves."""
    if thatch: steps,step_h,eave=steps+1,step_h+10,eave+40
    for i in range(steps):
        f=1-(i/steps); hh=step_h
        if ridge=="X": p.append(box(0,0,z+hh/2+i*hh,d+eave,w*f+90,hh,mat))
        else: p.append(box(0,0,z+hh/2+i*hh,d*f+90,w+eave,hh,mat))
    if trim: p.append(box(-d/2-5,0,z+step_h/2,d*0.05,w*0.92,step_h,trim))   # bargeboard on the street gable
    return z+steps*step_h
def _timber_frame(p,d,w,z0,h,beam,bay=190,proud=6):
    """Dark posts, sills and diagonal braces standing proud of a plaster wall block (d x w x h from z0)."""
    t=14; zc=z0+h/2
    for sx in (-1,1):   # corner posts
        for sy in (-1,1): p.append(box(sx*d/2,sy*w/2,zc,t+proud,t+proud,h,beam))
    for sy in (-1,1):   # posts along the long (+-Y) faces and braces between them
        n=max(1,int(d//bay)); step=d/(n+1)
        for i in range(1,n+1):
            x=-d/2+i*step; p.append(box(x,sy*(w/2+proud/2),zc,t,t,h,beam))
            if i<n: p.append(box(x+step/2,sy*(w/2+proud/2),zc,t,t,h*0.9,beam,roll=38))
    for sx in (-1,1):   # the gable faces
        n=max(1,int(w//bay)); step=w/(n+1)
        for i in range(1,n+1):
            y=-w/2+i*step; p.append(box(sx*(d/2+proud/2),y,zc,t,t,h,beam))
    for zz in (z0+h,):  # top plate
        p.append(box(0,0,zz,d+proud,t,t,beam,)); 
        for sy in (-1,1): p.append(box(0,sy*(w/2+proud/2),zz,d+proud,t,t,beam))
        for sx in (-1,1): p.append(box(sx*(d/2+proud/2),0,zz,t,w+proud,t,beam))
def _windows(p,d,w,z,n=2,size=(80,90),shutters=None,sill="P_LightGrey",glass="P_Glass",faces=("x","y")):
    """Windows with sills (and shutters) on the street gable (-X) and the long faces."""
    if "x" in faces:
        for y in (-w*0.28,w*0.28):
            p.append(box(-d/2-3,y,z,10,size[0],size[1],glass)); p.append(box(-d/2-8,y,z-size[1]/2-6,18,size[0]+24,12,sill))
            if shutters:
                for sy in (-1,1): p.append(box(-d/2-5,y+sy*(size[0]/2+22),z,8,36,size[1],shutters))
    if "y" in faces:
        for sy in (-1,1):
            for x in [(-d/2+(i+1)*d/(n+1)) for i in range(n)]:
                p.append(box(x,sy*(w/2+3),z,size[0],10,size[1],glass)); p.append(box(x,sy*(w/2+8),z-size[1]/2-6,size[0]+24,18,12,sill))
def _door(p,d,z=0,wd=120,ht=220,mat="P_DarkWood",lintel="P_DarkWood",step="P_LightGrey"):
    p.append(box(-d/2-3,0,z+ht/2,20,wd,ht,mat)); p.append(box(-d/2-6,0,z+ht+8,26,wd+40,16,lintel)); p.append(box(-d/2-30,0,z+6,60,wd+60,12,step))
def _chimney(p,x,y,z,h=180,mat="P_Grey",cap="P_DarkGrey"):
    p.append(box(x,y,z+h/2,70,70,h,mat)); p.append(box(x,y,z+h+8,90,90,16,cap))
def timber_house(w=620,d=820,storeys=1,roof="thatch",wall="P_Plaster",beam="P_DarkWood",base="P_Grey",roofmat=None,shutters="P_DarkGreen",chimney=True,jetty=False):
    """A Two Rivers / Andor house: stone base course, plaster walls in a dark timber frame, a stepped gable roof of
    thatch, tile, slate or wood shingle, a chimney, shuttered windows. storeys 1-2; jetty overhangs the upper floor."""
    rm=roofmat or {"thatch":"P_Straw","tile":"P_Terracotta","slate":"P_Slate","shingle":"P_Shingle"}[roof]
    fh=250; p=[box(0,0,30,d+16,w+16,60,base)]
    z=60
    for i in range(storeys):
        dd,ww=(d+40,w+40) if (jetty and i>0) else (d,w)
        p.append(box(0,0,z+fh/2,dd,ww,fh,wall)); _timber_frame(p,dd,ww,z,fh,beam)
        _windows(p,dd,ww,z+150,n=2 if i>0 else 1,shutters=shutters if i==0 else None)
        z+=fh
    _door(p,(d+40) if False else d)
    top=_roof_gable(p,d,w,z,rm,steps=5,thatch=(roof=="thatch"),trim=beam if roof!="thatch" else None)
    if chimney: _chimney(p,d*0.28,w*0.18,top-60)
    return p
def thatched_cottage(w=520,d=640,wall="P_Stone",thatch="P_ThatchDark",wood="P_DarkWood"):
    """The poorer Two Rivers house: low rough stone walls under a fat thatch roof that nearly reaches the ground."""
    h=210; p=[box(0,0,h/2,d,w,h,wall)]
    for y in (-w*0.3,w*0.3): p.append(box(-d/2-3,y,135,10,60,70,"P_Glass"))
    _door(p,d,ht=190,wd=100,mat=wood)
    top=_roof_gable(p,d,w,h,thatch,steps=6,step_h=60,eave=110,thatch=True)
    _chimney(p,d*0.3,0,top-80,h=150,mat="P_Grey")
    p.append(box(d/2+20,0,40,30,120,80,"P_Straw"))   # woodpile / straw heap at the back
    return p
def inn(w=900,d=1300,roof="tile",wall="P_Plaster",beam="P_DarkWood",base="P_Grey",roofmat=None,sign="P_Rose"):
    """The village inn: two timber-framed storeys with a jettied upper floor, a cross wing, dormers, two chimneys, a
    porch on posts over the door, a hanging sign and barrels by the wall. The Winespring Inn wears a red tile roof."""
    rm=roofmat or {"thatch":"P_Straw","tile":"P_RedBrick","slate":"P_Slate","shingle":"P_Shingle"}[roof]
    fh=260; p=[box(0,0,30,d+16,w+16,60,base)]
    p.append(box(0,0,60+fh/2,d,w,fh,wall)); _timber_frame(p,d,w,60,fh,beam); _windows(p,d,w,210,n=3,shutters="P_DarkGreen")
    z=60+fh; p.append(box(0,0,z+fh/2,d+40,w+40,fh,wall)); _timber_frame(p,d+40,w+40,z,fh,beam); _windows(p,d+40,w+40,z+150,n=3)
    z+=fh; top=_roof_gable(p,d,w,z,rm,steps=6,step_h=55,trim=beam)
    # cross wing to the back-right, its own gable across the main ridge
    wx,wy,wd,ww=d*0.15,w*0.55,d*0.5,w*0.7
    p.append(box(wx,wy,60+fh,wd,ww,2*fh,wall)); _timber_frame(p,wd,ww,60,2*fh,beam)
    for i in range(5): f=1-i/5; p.append(box(wx,wy,z+27+i*55,wd*f+90,ww+60,55,rm))
    for x in (-d*0.25,d*0.1): p.append(box(x,-w/2+70,z+95,120,110,120,wall)); p.append(box(x,-w/2+70,z+170,160,150,40,rm))   # dormers
    _chimney(p,-d*0.3,0,top-40,h=220); _chimney(p,wx+wd*0.3,wy,top-40,h=200)
    _door(p,d,wd=150,ht=240); 
    for sy in (-1,1): p.append(box(-d/2-120,sy*130,100,24,24,200,beam))   # porch posts
    p.append(box(-d/2-70,0,215,170,340,30,rm))
    p.append(box(-d/2-150,-w/2+60,300,20,20,320,beam)); p.append(box(-d/2-150,-w/2+60,470,24,140,90,sign))   # the sign
    for y in (w/2-90,w/2-200): p.append(cyl(-d/2-70,y,40,70,80,beam)); p.append(cyl(-d/2-70,y,45,74,10,"P_DarkGrey"))   # barrels
    return p
def shop(w=640,d=700,wall="P_Cream",beam="P_DarkWood",roof="P_Slate",cloth="P_Terracotta",cloth2="P_Cream",goods="P_Ochre"):
    """A shop: a timber house whose street front carries a striped awning over a counter of goods and a hanging board."""
    p=timber_house(w,d,storeys=2,roof="slate",wall=wall,beam=beam,roofmat=roof,shutters=None,chimney=True)
    for i in range(6): p.append(box(-d/2-110,-w/2+60+i*(w-120)/5,235,200,(w-120)/5+4,14,cloth if i%2==0 else cloth2,pitch=-12))
    for sy in (-1,1): p.append(box(-d/2-200,sy*(w/2-50),120,16,16,240,beam))
    p.append(box(-d/2-150,0,45,110,w-160,90,beam))   # counter
    for i,m in enumerate((goods,"P_Green","P_Rose","P_Olive")): p.append(box(-d/2-150,-w*0.3+i*w*0.2,110,60,70,40+10*(i%2),m))
    p.append(box(-d/2-20,w/2+40,330,20,110,70,"P_Gold"))   # sign board
    return p
def stone_house(w=620,d=820,storeys=2,wall="P_Stone",quoin="P_LightGrey",roof="P_Slate",wood="P_DarkWood"):
    """A town house of dressed stone (Baerlon, Caemlyn, Tear): quoined corners, slate roof, tall windows, a chimney."""
    fh=270; H=storeys*fh; p=[box(0,0,H/2,d,w,H,wall)]
    for sx in (-1,1):
        for sy in (-1,1):
            for i in range(int(H//70)): p.append(box(sx*d/2,sy*w/2,35+i*70,40,40,36,quoin))
    for i in range(1,storeys): p.append(box(0,0,i*fh,d+20,w+20,18,quoin))
    for i in range(storeys): _windows(p,d,w,i*fh+165,n=2,size=(70,110),sill=quoin,shutters=None if i else wood)
    _door(p,d,wd=120,ht=230,mat=wood,lintel=quoin,step=quoin)
    top=_roof_gable(p,d,w,H,roof,steps=5,step_h=50,trim=wood)
    _chimney(p,d*0.3,-w*0.2,top-40,h=200,mat=wall,cap=quoin)
    return p
def adobe_house(w=640,d=720,floors=1,wall="P_Adobe",trim="P_Clay",wood="P_DarkWood",canvas="P_Cream"):
    """A house of the dry south and the Waste (Tear's lowlands, Tanchico, Rhuidean): thick clay walls, a flat roof
    behind a parapet, pergola beams casting stripes, an outside stair to the roof and a canvas awning over the door."""
    fh=260; H=floors*fh; p=[box(0,0,H/2,d,w,H,wall), box(0,0,H+15,d+20,w+20,30,trim)]
    p.append(box(0,0,H+40,d-60,w-60,20,wall))
    for sx in (-1,1):
        for sy in (-1,1): p.append(box(sx*(d/2-30),sy*(w/2-30),H+40,40,40,60,trim))   # parapet corners
    for i in range(int(w//110)): p.append(box(d*0.1,-w/2+70+i*110,H+70,d*0.55,16,16,wood))   # pergola beams
    for sy in (-1,1): p.append(box(d*0.1,sy*(w/2-60),H+40,d*0.55,16,70,wood))
    for i in range(floors):
        for y in (-w*0.28,w*0.28): p.append(box(-d/2-3,y,i*fh+160,10,50,60,"P_DarkGrey"))
        for x in (-d*0.25,d*0.25): p.append(box(x,w/2+3,i*fh+160,50,10,60,"P_DarkGrey"))
    _door(p,d,wd=110,ht=210,mat=wood,lintel=wood,step=trim)
    for sy in (-1,1): p.append(box(-d/2-110,sy*90,230,16,16,10,wood)); p.append(box(-d/2-110,sy*90,110,14,14,230,wood))
    p.append(box(-d/2-60,0,240,150,220,12,canvas,pitch=-8))
    for i in range(6): p.append(box(d/2+40,-w/2+40+i*40,20+i*40,60,36,40,wall))   # the outside stair up the side
    return p
def adobe_tower(floors=3,w=460,**kw):
    """A tall adobe house with a roof terrace (Tanchico's streets, Rhuidean)."""
    p=adobe_house(w,w+80,floors=floors,**kw)
    for i in range(1,floors): p.append(box(0,0,i*260,w+100,w+20,16,kw.get("trim","P_Clay")))
    return p
def barn(w=700,d=1100,wall="P_Brown",roof="P_Shingle",wood="P_DarkWood",hay="P_Straw"):
    """A barn: plank walls, a wide double door, a shingle roof with a hay loft hatch, hay stacked beside it."""
    h=330; p=[box(0,0,h/2,d,w,h,wall)]
    for i in range(int(d//60)): p.append(box(-d/2+30+i*60,0,h/2,4,w+6,h,wood))   # plank lines
    p.append(box(-d/2-3,0,130,20,260,260,wood)); p.append(box(-d/2-6,0,130,22,8,260,"P_Black"))
    top=_roof_gable(p,d,w,h,roof,steps=5,step_h=60,ridge="X",trim=wood)
    p.append(box(-d/2-3,0,h+60,16,120,100,wood))   # loft hatch
    for i in range(3): p.append(box(d/2+120,-180+i*180,50,140,120,100,hay)); p.append(box(d/2+120,-180+i*180,130,120,100,60,hay))
    return p
def smithy(w=620,d=700,wall="P_Stone",roof="P_Shingle",wood="P_DarkWood",forge="P_DarkGrey",glow="P_Gold"):
    """Master Luhhan's forge: a stone back half, an open front on posts under one roof, the forge chimney and anvil."""
    h=260; p=[box(d*0.2,0,h/2,d*0.6,w,h,wall)]
    for sy in (-1,1): p.append(box(-d/2+30,sy*(w/2-30),h/2,30,30,h,wood))
    top=_roof_gable(p,d,w,h,roof,steps=4,step_h=60,ridge="X",trim=wood)
    _chimney(p,d*0.3,-w*0.25,top-60,h=260,mat=forge,cap="P_Black")
    p.append(box(d*0.05,-w*0.25,60,160,120,120,forge)); p.append(box(d*0.05,-w*0.25,125,100,70,12,glow))   # the forge and its coals
    p.append(box(-d*0.2,w*0.15,70,60,120,20,forge)); p.append(box(-d*0.2,w*0.15,40,30,30,60,forge))       # the anvil
    p.append(cyl(-d*0.25,-w*0.3,40,90,80,wood)); p.append(cyl(-d*0.25,-w*0.3,82,80,8,"P_Blue"))           # the quench barrel
    return p
# ---- street props (plaza_furniture puts anything it does not know on a jittered grid; lines use them along streets)
def cart(wood="P_DarkWood",bed="P_Brown",load="P_Straw"):
    p=[box(0,0,80,220,140,16,bed), box(0,0,70,180,120,30,load)]
    for sy in (-1,1): p.append(box(0,sy*80,60,110,16,110,wood)); p.append(box(0,sy*80,60,30,20,30,wood))
    for sy in (-1,1): p.append(box(-150,sy*50,90,120,14,14,wood,pitch=14))
    for sy in (-1,1): p.append(box(0,sy*66,110,220,8,40,wood))
    return p
def hay_bale(hay="P_Straw"):
    return [box(0,0,45,120,110,90,hay), box(0,0,95,100,90,20,hay), box(60,-30,30,60,60,60,hay)]
def barrel(wood="P_DarkWood",ring="P_DarkGrey"):
    return [cyl(0,0,45,80,90,wood), cyl(0,0,20,84,10,ring), cyl(0,0,70,84,10,ring)]
def crate_stack(wood="P_Brown",dark="P_DarkWood"):
    return [box(0,0,35,70,70,70,wood), box(10,80,35,70,70,70,wood), box(-5,40,105,70,70,70,dark)]
def festival_pole(h=620,pole="P_DarkWood",ribbons=("P_Rose","P_Blue","P_Gold","P_Green","P_White","P_Purple")):
    """A Bel Tine pole: ribbons from the top out to the ground around it."""
    p=[box(0,0,h/2,24,24,h,pole), box(0,0,h+16,60,60,32,"P_Rose")]
    n=len(ribbons); L=h*0.95
    for i,m in enumerate(ribbons):
        a=i*360/n; r=190; x=math.cos(math.radians(a))*r/2; y=math.sin(math.radians(a))*r/2
        p.append(box(x,y,h/2,10,L*0.0+10,10,m)); p.append(box(x,y,h/2+10,10,10,L,m,pitch=0,roll=0))
    return p
def woodpile(wood="P_Brown",dark="P_DarkWood"):
    p=[]
    for i in range(3):
        for j in range(4-i): p.append(cyl(0,-60+j*40+i*20,20+i*36,38,120,wood if (i+j)%2 else dark))
    return p
def signpost(pole="P_DarkWood",board="P_Plaster"):
    return [box(0,0,160,18,18,320,pole), box(40,0,290,110,12,36,board), box(-30,0,230,90,12,36,board)]
def bench(wood="P_DarkWood"):
    return [box(0,0,46,40,160,10,wood), box(0,0,24,8,140,40,wood), box(-22,0,70,8,160,40,wood,roll=0)]
def fence_segment(length=300,wood="P_Brown",post="P_DarkWood"):
    p=[]
    for y in (-length/2,length/2): p.append(box(0,y,55,16,16,110,post))
    for z in (45,90): p.append(box(0,0,z,10,length,12,wood))
    return p
def flower_box(wood="P_DarkWood",flower="P_Rose",leaf="P_Green"):
    return [box(0,0,16,60,150,32,wood), box(0,0,42,50,140,20,leaf), box(0,-40,60,30,30,20,flower), box(0,30,62,30,30,20,flower)]

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
 "SM_TimberHouse":timber_house,"SM_TimberHouse2":lambda:timber_house(storeys=2,roof="tile",jetty=True),"SM_TimberHouseSlate":lambda:timber_house(roof="slate",wall="P_Cream"),
 "SM_TimberHouseShingle":lambda:timber_house(w=560,d=760,roof="shingle",wall="P_Plaster",shutters="P_Brown"),"SM_ThatchedCottage":thatched_cottage,
 "SM_Inn":inn,"SM_InnThatch":lambda:inn(roof="thatch"),"SM_Shop":shop,"SM_StoneHouse":stone_house,"SM_StoneHouse3":lambda:stone_house(storeys=3,w=560,d=700),
 "SM_AdobeHouse":adobe_house,"SM_AdobeHouse2":lambda:adobe_house(floors=2,w=600,d=660),"SM_AdobeTower":adobe_tower,"SM_Barn":barn,"SM_Smithy":smithy,
 "SM_Cart":cart,"SM_HayBale":hay_bale,"SM_Barrel":barrel,"SM_CrateStack":crate_stack,"SM_FestivalPole":festival_pole,"SM_Woodpile":woodpile,"SM_Signpost":signpost,"SM_Bench":bench,"SM_FenceSegment":fence_segment,"SM_FlowerBox":flower_box,
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
