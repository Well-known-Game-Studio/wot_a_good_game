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
# second shade of each roof material: every other course of tiles / thatch / shingle takes it, so roofs read as courses
# instead of one flat slab (kit_entry recolours both shades together, see _expand_recolor)
ALT={"P_Terracotta":"P_Terracotta2","P_RedBrick":"P_RedBrick2","P_Slate":"P_Slate2","P_Straw":"P_Straw2","P_ThatchDark":"P_ThatchDark2","P_Shingle":"P_Shingle2","P_Stone":"P_Stone2"}
def _roof_gable(p,d,w,z,mat,steps=5,step_h=55,eave=60,ridge="X",trim=None,thatch=False,moss=None,patch=None):
    """A stepped voxel gable roof over a d x w footprint starting at height z. ridge "X": the roof steps shrink along Y
    (gable ends face +-X, the street); "Y": they shrink along X. Courses alternate two shades; a ridge cap on top;
    bargeboards on both gables when trim is given. Thatch gets a fatter, rounder stack, deeper eaves and a dark comb
    ridge; moss puts a few green patches on the lower courses, patch a repaired patch in the second shade."""
    if thatch: steps,step_h,eave=steps+1,step_h+10,eave+40
    alt=ALT.get(mat,mat)
    for i in range(steps):
        f=1-(i/steps); hh=step_h; m=mat if i%2==0 else alt
        if ridge=="X": p.append(box(0,0,z+hh/2+i*hh,d+eave,w*f+90,hh,m))
        else: p.append(box(0,0,z+hh/2+i*hh,d*f+90,w+eave,hh,m))
    top=z+steps*step_h
    if thatch:
        if ridge=="X": p.append(box(0,0,top+5,d+eave-40,w*0.16+70,14,alt))
        else: p.append(box(0,0,top+5,d*0.16+70,w+eave-40,14,alt))
    else:
        cap=trim or alt
        if ridge=="X": p.append(box(0,0,top+6,d+eave-16,60,14,cap))
        else: p.append(box(0,0,top+6,60,w+eave-16,14,cap))
        if trim:
            for sx in (-1,1): p.append(box(sx*(d/2+eave/2-6),0,z+step_h/2,16,w*0.92,step_h,trim))   # bargeboards
    if moss:
        for (fx,fy) in ((0.28,-0.42),(-0.2,0.4)): p.append(box(d*fx,w*fy,z+step_h+2,120,70,10,moss))
    if patch:
        p.append(box(d*0.15,w*0.3*(1 if ridge=="X" else -1),z+step_h*2+2,90,120,10,patch))
    return top
def _timber_frame(p,d,w,z0,h,beam,bay=190,proud=8,rail=True):
    """Dark posts, sills, a mid rail and braces standing proud of a plaster wall block (d x w x h from z0): a brace
    leans the other way in every second bay and every third bay gets a full X, so the frames read as real carpentry."""
    t=14; zc=z0+h/2
    # every member stands a different distance proud of the wall (posts 11, braces 12 / 14, rails 13, corners 14 cm)
    # so no two of them share a face plane where they cross: coplanar faces z-fight
    for sx in (-1,1):   # corner posts
        for sy in (-1,1): p.append(box(sx*d/2,sy*w/2,zc,t+proud+6,t+proud+6,h,beam))
    for sy in (-1,1):   # posts along the long (+-Y) faces and braces between them (tilting in the wall's own plane)
        n=max(1,int(d//bay)); step=d/(n+1); yy=sy*(w/2+proud/2)
        for i in range(1,n+1):
            x=-d/2+i*step; p.append(box(x,yy,zc,t,t,h,beam))
        for i in range(0,n+1):
            x=-d/2+(i+0.5)*step; k=(i+(1 if sy>0 else 0))%3
            if k==2: p.append(box(x,yy+sy*2,zc,12,12,h*0.78,beam,pitch=36)); p.append(box(x,yy+sy*4,zc,12,12,h*0.78,beam,pitch=-36))
            else: p.append(box(x,yy+sy*2,zc,12,12,h*0.8,beam,pitch=36 if k==0 else -36))
    for sx in (-1,1):   # the gable faces: posts and one pair of braces
        n=max(1,int(w//bay)); step=w/(n+1); xx=sx*(d/2+proud/2)
        for i in range(1,n+1):
            y=-w/2+i*step; p.append(box(xx,y,zc,t,t,h,beam))
        if sx>0 or w>400:
            for sy in (-1,1): p.append(box(xx+sx*2,sy*w*0.3,zc,12,12,h*0.75,beam,roll=sy*34))
    for zz in ((z0+t/2,),(z0+h,))+(((z0+h*0.52,),) if rail else ()):   # sill, top plate, mid rail
        z_=zz[0]
        for sy in (-1,1): p.append(box(0,sy*(w/2+proud/2+1),z_,d+proud,16,t,beam))
        for sx in (-1,1): p.append(box(sx*(d/2+proud/2+1),0,z_,16,w+proud,t,beam))
def _jetty_brackets(p,d,w,z,beam,n=3):
    """Diagonal brackets under a jettied upper floor, along both long faces."""
    for sy in (-1,1):
        for i in range(n): p.append(box(-d/2+(i+0.5)*d/n,sy*(w/2+10),z-40,14,14,90,beam,roll=-sy*40))
def _window(p,x,y,z,size,face,glass="P_Glass",sill="P_LightGrey",bar="P_DarkWood",shutters=None,flowers=None):
    """One window: glass, a dark mullion cross, a lintel, a sill, optional shutters with an inset panel and a flower box."""
    sw,sh=size
    if face=="x":   # on a +-X face at x
        p.append(box(x,y,z,10,sw,sh,glass)); p.append(box(x-2,y,z,12,6,sh,bar)); p.append(box(x-2,y,z,12,sw,6,bar))
        p.append(box(x-4,y,z+sh/2+8,16,sw+30,14,bar)); p.append(box(x-5,y,z-sh/2-6,18,sw+24,12,sill))
        if shutters:
            for sy in (-1,1): p.append(box(x-2,y+sy*(sw/2+22),z,8,36,sh,shutters)); p.append(box(x-8,y+sy*(sw/2+22),z,4,22,sh*0.7,bar))
        if flowers: p.append(box(x-18,y,z-sh/2-20,26,sw+10,24,"P_DarkWood")); p.append(box(x-20,y-sw*0.25,z-sh/2-2,22,24,16,flowers)); p.append(box(x-20,y+sw*0.2,z-sh/2-2,22,24,16,"P_Gold"))
    else:   # on a +-Y face at y
        p.append(box(x,y,z,sw,10,sh,glass)); p.append(box(x,y-2,z,6,12,sh,bar)); p.append(box(x,y-2,z,sw,12,6,bar))
        p.append(box(x,y-4,z+sh/2+8,sw+30,16,14,bar)); p.append(box(x,y-5,z-sh/2-6,sw+24,18,12,sill))
        if shutters:
            for sx in (-1,1): p.append(box(x+sx*(sw/2+22),y-2,z,36,8,sh,shutters)); p.append(box(x+sx*(sw/2+22),y-5,z,22,4,sh*0.7,bar))
def _windows(p,d,w,z,n=2,size=(80,90),shutters=None,sill="P_LightGrey",glass="P_Glass",faces=("x","y"),flowers=None,bar="P_DarkWood"):
    """Windows with mullions, lintels and sills (and shutters, flower boxes) on the street gable (-X) and the long faces."""
    if "x" in faces:
        for y in (-w*0.28,w*0.28): _window(p,-d/2-3,y,z,size,"x",glass,sill,bar,shutters,flowers)
    if "y" in faces:
        for sy in (-1,1):
            for x in [(-d/2+(i+1)*d/(n+1)) for i in range(n)]:
                # a +Y face window is mirrored: build it on -Y and flip the small offsets by using the sign on y
                sw,sh=size; yy=sy*(w/2+3)
                p.append(box(x,yy,z,sw,10,sh,glass)); p.append(box(x,yy+sy*2,z,6,12,sh,bar)); p.append(box(x,yy+sy*2,z,sw,12,6,bar))
                p.append(box(x,yy+sy*4,z+sh/2+8,sw+30,16,14,bar)); p.append(box(x,yy+sy*5,z-sh/2-6,sw+24,18,12,sill))
                if shutters:
                    for sx in (-1,1): p.append(box(x+sx*(sw/2+22),yy+sy*2,z,36,8,sh,shutters)); p.append(box(x+sx*(sw/2+22),yy+sy*8,z,22,4,sh*0.7,bar))
def _door(p,d,z=0,wd=120,ht=220,mat="P_DarkWood",lintel="P_DarkWood",step="P_LightGrey",lantern="P_Gold",planks="P_Brown"):
    """A plank door with a handle, lintel and doorstep, and a lantern on a bracket beside it."""
    x=-d/2-3
    p.append(box(x,0,z+ht/2,20,wd,ht,mat))
    for k in range(1,3): p.append(box(x-10,-wd/2+k*wd/3,z+ht/2,4,6,ht-10,planks))
    p.append(box(x-12,wd*0.3,z+ht*0.48,6,14,14,"P_DarkGrey"))   # handle
    p.append(box(x-3,0,z+ht+8,26,wd+40,16,lintel)); p.append(box(x-27,0,z+6,60,wd+60,12,step))
    if lantern:
        p.append(box(x-18,wd/2+50,z+ht*0.85,30,8,8,"P_DarkGrey")); p.append(box(x-30,wd/2+50,z+ht*0.85-22,20,20,32,lantern)); p.append(box(x-30,wd/2+50,z+ht*0.85-2,26,26,8,"P_DarkGrey"))
def _chimney(p,x,y,z,h=180,mat="P_Grey",cap="P_DarkGrey"):
    p.append(box(x,y,z+h/2,70,70,h,mat)); p.append(box(x,y,z+h+8,90,90,16,cap)); p.append(box(x,y,z+h+30,40,40,28,"P_LightGrey"))   # a puff of smoke
def _dormer(p,x,y,z,roofmat,wall="P_Plaster",sy=-1):
    """A small gabled dormer on a long roof slope at (x, y) with its sill at z, window facing sy*Y."""
    p.append(box(x,y,z+55,120,110,110,wall)); p.append(box(x,y+sy*58,z+58,60,8,60,"P_Glass")); p.append(box(x,y+sy*60,z+58,6,10,60,"P_DarkWood"))
    for i in range(3): p.append(box(x,y,z+118+i*20,160-i*40,150,20,roofmat if i%2 else ALT.get(roofmat,roofmat)))
def timber_house(w=620,d=820,storeys=1,roof="thatch",wall="P_Plaster",beam="P_DarkWood",base="P_Grey",roofmat=None,shutters="P_DarkGreen",chimney=True,jetty=False,dormers=0,wing=False,flowers="P_Rose"):
    """A Two Rivers / Andor house: stone base course, plaster walls in a dark timber frame, a stepped gable roof of
    thatch, tile, slate or wood shingle, a chimney, shuttered windows with flower boxes. storeys 1-2; jetty overhangs
    the upper floor on brackets; dormers on the front slope; wing adds a cross wing at the back."""
    rm=roofmat or {"thatch":"P_Straw","tile":"P_Terracotta","slate":"P_Slate","shingle":"P_Shingle"}[roof]
    fh=250; p=[box(0,0,30,d+16,w+16,60,base)]
    for i in range(int((d+16)//90)): p.append(box(-d/2-8+45+i*90,0,28,4,w+20,52,"P_DarkGrey"))   # stone courses in the base
    z=60
    for i in range(storeys):
        dd,ww=(d+40,w+40) if (jetty and i>0) else (d,w)
        if jetty and i>0: _jetty_brackets(p,dd,ww,z,beam)
        p.append(box(0,0,z+fh/2,dd,ww,fh,wall)); _timber_frame(p,dd,ww,z,fh,beam)
        _windows(p,dd,ww,z+150,n=2 if i>0 else 1,shutters=shutters if i==0 else None,flowers=flowers if i==storeys-1 else None)
        z+=fh
    _door(p,d)
    top=_roof_gable(p,d,w,z,rm,steps=5,thatch=(roof=="thatch"),trim=beam if roof!="thatch" else None,moss="P_Olive" if roof=="thatch" else None,patch=None if roof=="thatch" else ALT.get(rm))
    for k in range(dormers): _dormer(p,-d*0.25+k*d*0.5,-w/2+95,z+60,rm,wall,-1)
    if wing:
        wx,wy,wd,ww=d*0.18,w*0.55,d*0.45,w*0.7
        p.append(box(wx,wy,60+(z-60)/2,wd,ww,z-60,wall)); _timber_frame(p,wd,ww,60,z-60,beam,rail=False)
        _window(p,wx,wy+ww/2+3,160,(70,80),"y")
        for i in range(5): f=1-i/5; p.append(box(wx,wy,z+27+i*55,wd*f+90,ww+60,55,rm if i%2==0 else ALT.get(rm,rm)))
    if chimney: _chimney(p,d*0.28,w*0.18,top-60)
    return p
def thatched_cottage(w=520,d=640,wall="P_Stone",thatch="P_ThatchDark",wood="P_DarkWood"):
    """The poorer Two Rivers house: low rough stone walls under a fat thatch roof that nearly reaches the ground."""
    h=210; p=[box(0,0,h/2,d,w,h,wall)]
    for y in (-w*0.3,w*0.3): p.append(box(-d/2-3,y,135,10,60,70,"P_Glass"))
    _door(p,d,ht=190,wd=100,mat=wood)
    for i in range(int(d//70)):   # rough stone courses
        p.append(box(-d/2+35+i*70,0,h*0.33,4,w+6,8,"P_DarkGrey")); p.append(box(-d/2+35+i*70,0,h*0.66,4,w+6,8,"P_DarkGrey"))
    top=_roof_gable(p,d,w,h,thatch,steps=6,step_h=60,eave=110,thatch=True,moss="P_Olive")
    _chimney(p,d*0.3,0,top-80,h=150,mat="P_Grey")
    p.append(box(d/2+20,0,40,30,120,80,"P_Straw"))   # woodpile / straw heap at the back
    p.append(box(-d/2-60,-w*0.3,24,40,120,8,wood)); p.append(box(-d/2-60,-w*0.3,10,8,100,20,wood))   # a bench by the door
    return p
def inn(w=900,d=1300,roof="tile",wall="P_Plaster",beam="P_DarkWood",base="P_Grey",roofmat=None,sign="P_Rose"):
    """The village inn: two timber-framed storeys with a jettied upper floor, a cross wing, dormers, two chimneys, a
    porch on posts over the door, a hanging sign and barrels by the wall. The Winespring Inn wears a red tile roof."""
    rm=roofmat or {"thatch":"P_Straw","tile":"P_RedBrick","slate":"P_Slate","shingle":"P_Shingle"}[roof]
    fh=260; p=[box(0,0,30,d+16,w+16,60,base)]
    p.append(box(0,0,60+fh/2,d,w,fh,wall)); _timber_frame(p,d,w,60,fh,beam); _windows(p,d,w,210,n=3,shutters="P_DarkGreen")
    z=60+fh; _jetty_brackets(p,d+40,w+40,z,beam,4); p.append(box(0,0,z+fh/2,d+40,w+40,fh,wall)); _timber_frame(p,d+40,w+40,z,fh,beam); _windows(p,d+40,w+40,z+150,n=3,flowers="P_Rose")
    z+=fh; top=_roof_gable(p,d,w,z,rm,steps=6,step_h=55,trim=beam)
    # cross wing to the back-right, its own gable across the main ridge
    wx,wy,wd,ww=d*0.15,w*0.55,d*0.5,w*0.7
    p.append(box(wx,wy,60+fh,wd,ww,2*fh,wall)); _timber_frame(p,wd,ww,60,2*fh,beam)
    for i in range(5): f=1-i/5; p.append(box(wx,wy,z+27+i*55,wd*f+90,ww+60,55,rm if i%2==0 else ALT.get(rm,rm)))
    for x in (-d*0.25,d*0.1): _dormer(p,x,-w/2+95,z+60,rm,wall,-1)
    _chimney(p,-d*0.3,0,top-40,h=220); _chimney(p,wx+wd*0.3,wy,top-40,h=200)
    _door(p,d,wd=150,ht=240); 
    for sy in (-1,1): p.append(box(-d/2-120,sy*130,100,24,24,200,beam))   # porch posts
    p.append(box(-d/2-70,0,215,170,340,30,rm))
    p.append(box(-d/2-150,-w/2+60,300,20,20,320,beam)); p.append(box(-d/2-150,-w/2+60,470,24,140,90,sign)); p.append(box(-d/2-150,-w/2+60,470,30,60,40,"P_Gold"))   # the sign and its painted grape
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
    for i in range(1,int(H//90)): p.append(box(0,0,i*90,d+6,w+6,5,ALT.get(wall,"P_DarkGrey")))   # mortar courses
    for sx in (-1,1):
        for sy in (-1,1):
            for i in range(int(H//70)): p.append(box(sx*d/2,sy*w/2,35+i*70,40,40,36,quoin))
    for i in range(1,storeys): p.append(box(0,0,i*fh,d+20,w+20,18,quoin))
    for i in range(storeys): _windows(p,d,w,i*fh+165,n=2,size=(70,110),sill=quoin,shutters=None if i else wood,flowers="P_Rose" if i==storeys-1 else None)
    _door(p,d,wd=120,ht=230,mat=wood,lintel=quoin,step=quoin)
    top=_roof_gable(p,d,w,H,roof,steps=5,step_h=50,trim=wood,patch=ALT.get(roof))
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

# ---- yards, farm props and ground cover (the "lived in" pass: issues #34 / #35 references)
def laundry_line(length=360,post="P_DarkWood",cloths=("P_White","P_Blue","P_Rose","P_Cream")):
    p=[box(-length/2,0,100,16,16,200,post),box(length/2,0,100,16,16,200,post),box(0,0,186,length,4,4,"P_LightGrey")]
    n=len(cloths)
    for i,m in enumerate(cloths): p.append(box(-length/2+(i+0.5)*length/n,0,150,60,8,70-10*(i%2),m))
    return p
def water_trough(wood="P_DarkWood",water="P_Blue"):
    p=[box(0,0,8,70,170,16,wood),box(-28,0,30,14,170,60,wood),box(28,0,30,14,170,60,wood),box(0,-78,30,70,14,60,wood),box(0,78,30,70,14,60,wood)]
    return p+[box(0,0,52,42,142,8,water)]
def wheelbarrow(wood="P_Brown",dark="P_DarkWood",load="P_Straw"):
    return [box(0,0,50,100,70,30,wood),box(0,0,70,80,54,16,load),box(60,0,30,34,10,34,dark),box(-80,0,52,70,10,10,dark,yaw=12),box(-80,0,52,70,10,10,dark,yaw=-12),box(-40,0,18,10,10,36,dark)]
def lantern_post(h=220,post="P_DarkWood",lamp="P_Gold"):
    return [box(0,0,h/2,16,16,h,post),box(14,0,h-8,40,8,8,"P_DarkGrey"),box(34,0,h-34,22,22,36,lamp),box(34,0,h-12,28,28,8,"P_DarkGrey")]
def hedge(length=300,leaf="P_LeafGreen"):
    """A clipped hedge run along Y (like fence_segment / picket_fence: posts at +-length/2 in y)."""
    p=[box(0,0,55,80,length,110,leaf)]
    for i in range(int(length//90)): p.append(box((i%2)*30-15,-length/2+45+i*90,118,60,60,24,leaf))
    return p
def stone_wall(length=300,stone="P_Grey",dark="P_DarkGrey",cap="P_LightGrey"):
    """A dry-stone field wall run along Y: three staggered courses and a line of capstones."""
    p=[]
    for z,m in enumerate((stone,dark,stone)):
        n=int(length//80)
        for i in range(n): p.append(box(0,-length/2+40+i*80+(20 if z%2 else -20),z*38+19,60,76,38,m if (i+z)%3 else dark))
    p.append(box(0,0,118,70,length,12,cap))
    return p
def gate(width=240,wood="P_Brown",post="P_DarkWood"):
    p=[box(-width/2,0,70,20,20,140,post),box(width/2,0,70,20,20,140,post)]
    for z in (30,70,110): p.append(box(0,0,z,width-30,10,12,wood))
    p.append(box(0,0,70,width-40,8,10,wood,pitch=-32))
    return p
def scarecrow(pole="P_DarkWood",shirt="P_Brown",head="P_Straw",hat="P_DarkGrey"):
    return [box(0,0,120,12,12,240,pole),box(0,0,200,12,150,12,pole),box(0,0,180,40,90,90,shirt),box(0,0,250,40,40,40,head),box(0,0,276,60,60,10,hat),box(0,0,290,36,36,22,hat),box(0,-70,186,30,30,30,shirt),box(0,70,186,30,30,30,shirt)]
def crop_rows(w=600,d=420,soil="P_Brown",leaf="P_LeafGreen",leaf2="P_Olive",rows=5,per=6):
    """A tabac / vegetable patch: a slab of turned earth with rows of plants (foliage material, so they stir in the wind)."""
    p=[box(0,0,6,d,w,12,soil)]
    for r in range(rows):
        y=-w/2+(r+0.5)*w/rows
        for i in range(per): p.append(box(-d/2+(i+0.5)*d/per,y,30+6*((i+r)%2),42,40,36+12*((i+r)%2),leaf if (i+r)%3 else leaf2))
    return p
def beehive(straw="P_Straw",band="P_ThatchDark",wood="P_DarkWood"):
    return [box(0,0,16,90,90,32,wood),cyl(0,0,60,70,60,straw),cyl(0,0,96,56,24,straw),cyl(0,0,112,36,12,band),cyl(0,0,60,74,8,band)]
def chicken_coop(wall="P_Brown",roof="P_Shingle",wood="P_DarkWood"):
    p=[box(0,0,60,120,100,100,wall),box(0,0,114,150,130,16,roof),box(0,0,128,110,96,16,ALT.get(roof,roof)),box(-62,0,40,8,40,50,"P_Black"),box(-66,0,10,60,30,8,wood)]
    for i in range(3): p.append(box(-110+i*20,-70+i*35,8,26,22,16,"P_White")); p.append(box(-100+i*20,-70+i*35,22,10,10,10,"P_Rose"))   # hens
    return p
def log_bench(wood="P_Trunk",dark="P_DarkWood"):
    return [cyl(-70,0,16,40,32,dark),cyl(70,0,16,40,32,dark),box(0,0,46,220,50,28,wood)]
def stump(wood="P_Trunk",top="P_Brown"):
    return [cyl(0,0,22,80,44,wood),cyl(0,0,46,70,6,top),box(44,0,14,30,24,28,wood)]
def fallen_log(wood="P_Trunk",moss="P_Olive"):
    return [box(0,0,26,280,52,52,wood),box(60,36,40,40,40,40,wood),box(-40,0,54,90,40,8,moss),box(-150,0,20,24,30,30,"P_Brown")]
def rock_small(mat="P_Grey",dark="P_DarkGrey"):
    return [box(0,0,22,90,70,44,mat,yaw=20),box(40,20,16,50,40,32,dark,yaw=-15)]
def flower_patch(leaf="P_LeafGreen",flowers=("P_Rose","P_Gold","P_White","P_Purple")):
    p=[box(0,0,8,150,130,16,leaf)]
    pts=((-40,-30),(30,-40),(45,30),(-30,40),(0,0),(-55,5))
    for i,(x,y) in enumerate(pts): p.append(box(x,y,30,16,16,30,leaf)); p.append(box(x,y,48,24,24,14,flowers[i%len(flowers)]))
    return p
def grass_tuft(leaf="P_LeafPale",leaf2="P_LeafGreen"):
    p=[]
    for i,(x,y,h) in enumerate(((0,0,110),(28,14,80),(-24,20,90),(10,-30,70),(-30,-18,64),(34,-14,96))): p.append(box(x,y,h/2,12,12,h,leaf if i%2 else leaf2))
    return p
def oak(h=520,trunk="P_Trunk",leaf="P_LeafGreen",light="P_LeafPale"):
    """A broad old oak: thick trunk, two limbs, a wide lumpy canopy with lighter crown blobs."""
    p=[box(0,0,h*0.3,90,90,h*0.6,trunk),box(70,0,h*0.5,160,50,50,trunk,pitch=-35),box(-60,40,h*0.55,140,50,50,trunk,yaw=140,pitch=-30)]
    p+=[box(0,0,h*0.78,460,420,200,leaf),box(90,60,h*0.92,300,280,160,leaf),box(-110,-70,h*0.9,280,300,150,leaf),box(20,-20,h*1.05,200,200,110,light),box(150,-90,h*0.72,160,150,100,leaf),box(-160,110,h*0.74,150,160,100,light)]
    return p
def apple_tree(h=380,trunk="P_Trunk",leaf="P_LeafGreen",fruit="P_Rose"):
    p=[box(0,0,h*0.3,60,60,h*0.6,trunk),box(0,0,h*0.78,300,290,180,leaf),box(40,30,h*0.98,200,190,110,leaf)]
    for (x,y,z) in ((120,60,h*0.75),(-110,80,h*0.85),(60,-130,h*0.8),(-40,-100,h*0.95),(140,-30,h*0.9)): p.append(box(x,y,z,24,24,24,fruit))
    return p
def bush_round(leaf="P_LeafGreen",light="P_LeafPale"):
    return [box(0,0,60,170,150,120,leaf),box(50,40,100,110,100,80,light),box(-60,-30,96,100,110,70,leaf)]
def mushroom_cluster(stem="P_Cream",cap="P_Brown",cap2="P_Rose"):
    p=[]
    for i,(x,y,h) in enumerate(((0,0,36),(40,20,26),(-30,30,22))): p.append(box(x,y,h/2,16,16,h,stem)); p.append(box(x,y,h+6,36-6*i,36-6*i,14,cap if i%2 else cap2))
    return p
def picket_fence(length=300,wood="P_Brown",post="P_DarkWood"):
    p=[]
    for y in (-length/2,length/2): p.append(box(0,y,50,16,16,100,post))
    for z in (40,78): p.append(box(0,0,z,8,length,10,wood))
    for i in range(int(length//46)): p.append(box(6,-length/2+23+i*46,52,8,22,96,wood))
    return p
def _place(parts,ox,oy,oz=0,oyaw=0):
    """Parts of a sub-generator moved to (ox,oy,oz) and turned by oyaw."""
    out=[]; c,s_=math.cos(math.radians(oyaw)),math.sin(math.radians(oyaw))
    for mesh,(px,py,pz),pyaw,sc,m in parts:
        rot=(pyaw[0],pyaw[1]+oyaw,pyaw[2]) if isinstance(pyaw,tuple) else pyaw+oyaw
        out.append((mesh,(ox+px*c-py*s_,oy+px*s_+py*c,oz+pz),rot,sc,m))
    return out
def yard(parts,d,w,seed=1,fence="picket",n_extra=4):
    """Wraps a house (door on -X) in the things a lived-in house has around it: a fence down both long sides and across
    the back (a gap for a gate on one side), a lantern post by the path, and a seeded pick of woodpile, barrels,
    laundry line, trough, vegetable patch, beehive, hen coop, wheelbarrow, bushes, crates and hay. Everything stays
    inside x -d/2-230..d/2+210, y +-(w/2+210), so the house set footprint (-760,-560,760,560) covers it."""
    r=_rnd.Random(seed); p=list(parts)
    fy=w/2+200; fx0=-d/2-150; fx1=d/2+190
    def run(L): return picket_fence(L) if fence=="picket" else (stone_wall(L) if fence=="stone" else hedge(L))
    gate_side=r.choice((-1,1))
    n=int((fx1-fx0)//300)
    for sy in (-1,1):   # long sides, 300 cm runs (turned to lie along X) from the front post to the back
        for i in range(n):
            if sy==gate_side and i==1: continue   # the gate gap
            p+=_place(run(300),fx0+150+i*300,sy*fy,0,90)
    for i in range(int((2*fy)//300)): p+=_place(run(300),fx1,-fy+150+i*300,0,0)   # the back fence runs along Y
    p+=_place(lantern_post(),fx0+40,-gate_side*(w/2+140),0,0)
    extras=[("woodpile",(d/2+90,-w/2-90,30),woodpile),("barrels",(d/2+70,w/2+90,0),lambda:barrel()+_place(barrel(),95,-20)),
            ("laundry",(d*0.05,w/2+130,90),laundry_line),("trough",(-d/2-170,-w/2-80,0),water_trough),("veg",(d/2+120,0,0),lambda:crop_rows(w*0.6,150,rows=3,per=3)),
            ("beehive",(-d*0.15,-w/2-140,0),beehive),("coop",(d*0.3,w/2+140,0),chicken_coop),("barrow",(-d/2-130,w/2+60,0),wheelbarrow),
            ("bush",(-d/2-120,-w/2-130,0),bush_round),("crates",(d/2+100,w*0.1,0),crate_stack),("hay",(d/2+110,-w*0.25,0),hay_bale),("tuft",(-d/2-200,w/2+150,0),grass_tuft)]
    picks=[]; pool=list(extras)
    while pool and len(picks)<n_extra:
        e=pool.pop(r.randrange(len(pool)))
        if any(abs(e[1][0]-q[1][0])<150 and abs(e[1][1]-q[1][1])<150 for q in picks): continue
        picks.append(e)
    for name,(ox,oy,oyaw),fn in picks: p+=_place(fn(),ox,oy,0,oyaw)
    return p
def cottage_dormer(**kw):
    """A one-and-a-half storey Two Rivers house: thatch with two dormers in the front slope."""
    return timber_house(w=640,d=860,storeys=1,roof="thatch",dormers=2,**kw)
def l_house(**kw):
    """A timber house with a cross wing at the back (the better-off Two Rivers farmers, Baerlon lanes)."""
    return timber_house(w=620,d=880,storeys=2,roof="tile",jetty=True,wing=True,**kw)
def stone_timber_house(w=640,d=820,wall="P_Stone",quoin="P_LightGrey",upper="P_Cream",beam="P_DarkWood",roof="P_Slate"):
    """Stone ground floor under a jettied timber upper storey (Baerlon, Whitebridge, Caemlyn's older streets)."""
    fh=260; p=[box(0,0,fh/2,d,w,fh,wall)]
    for i in range(1,3): p.append(box(0,0,i*85,d+6,w+6,5,ALT.get(wall,"P_DarkGrey")))
    for sx in (-1,1):
        for sy in (-1,1):
            for i in range(int(fh//70)): p.append(box(sx*d/2,sy*w/2,35+i*70,40,40,36,quoin))
    _windows(p,d,w,165,n=2,size=(70,100),sill=quoin,shutters=beam)
    _door(p,d,wd=120,ht=230,mat=beam,lintel=quoin,step=quoin)
    z=fh; dd,ww=d+50,w+50
    _jetty_brackets(p,dd,ww,z,beam,4); p.append(box(0,0,z+fh/2,dd,ww,fh,upper)); _timber_frame(p,dd,ww,z,fh,beam); _windows(p,dd,ww,z+150,n=2,flowers="P_Rose")
    top=_roof_gable(p,d,w,z+fh,roof,steps=5,step_h=55,trim=beam,patch=ALT.get(roof))
    _dormer(p,-d*0.2,-w/2+95,z+fh+60,roof,upper,-1)
    _chimney(p,d*0.3,w*0.2,top-50,h=200,mat=wall,cap=quoin)
    return p
def farmhouse(w=560,d=1100,wall="P_Plaster",beam="P_DarkWood",thatch="P_Straw",byre="P_Brown"):
    """A long Two Rivers farmhouse: the family end in timber and plaster, the byre end in planks under one thatch roof,
    a dormer, hay by the byre door."""
    fh=250; p=[box(0,0,30,d+16,w+16,60,"P_Grey")]
    p.append(box(-d*0.2,0,60+fh/2,d*0.6,w,fh,wall))
    hp=[]; _timber_frame(hp,d*0.6,w,60,fh,beam); _windows(hp,d*0.6,w,210,n=2,shutters="P_DarkGreen",flowers="P_Rose"); _door(hp,d*0.6)
    p+=_place(hp,-d*0.2,0)   # the house end is built about its own centre, then slid to -X
    p.append(box(d*0.3,0,60+fh/2,d*0.4,w,fh,byre))
    for i in range(int((d*0.4)//60)): p.append(box(d*0.1+30+i*60,0,60+fh/2,4,w+6,fh,beam))
    p.append(box(d*0.3,w/2+3,160,200,20,200,beam)); p.append(box(d*0.3,w/2+6,160,8,24,200,"P_Black"))   # byre double door on the side
    top=_roof_gable(p,d,w,60+fh,thatch,steps=5,thatch=True,moss="P_Olive")
    _dormer(p,-d*0.25,-w/2+95,60+fh+60,thatch,wall,-1)
    _chimney(p,-d*0.3,w*0.15,top-60)
    p+=_place(hay_bale(),d*0.3,w/2+160)
    return p
def watermill(w=700,d=900,wall="P_Stone",quoin="P_LightGrey",roof="P_Shingle",wood="P_DarkWood"):
    """Jon Thane's mill: a stone mill house with a big paddle wheel on its +Y side, the axle housing and a sluice."""
    p=stone_house(w=w,d=d,storeys=2,wall=wall,quoin=quoin,roof=roof,wood=wood)
    R=230; cx,cy,cz=d*0.1,w/2+120,R+40
    p.append(box(cx,cy-80,cz,60,160,60,wood))   # axle
    for k in range(12):
        a=k*30; x=cx+R*math.cos(math.radians(a)); z=cz+R*math.sin(math.radians(a))
        p.append(box(x,cy,z,24,110,70,wood,pitch=-a))              # paddles
        p.append(box((cx+x)/2,cy-40,(cz+z)/2,R,16,16,wood,pitch=-a)); p.append(box((cx+x)/2,cy+40,(cz+z)/2,R,16,16,wood,pitch=-a))   # spokes
    p.append(box(cx,cy,30,2*R+140,200,60,"P_Blue"))                # the race under the wheel
    p.append(box(cx-R-200,cy,120,300,120,16,wood,pitch=-10)); p.append(box(cx-R-200,cy-60,110,300,10,60,wood,pitch=-10)); p.append(box(cx-R-200,cy+60,110,300,10,60,wood,pitch=-10))   # sluice
    p+=_place(crate_stack(),-d/2-120,w*0.3)
    p.append(box(-d/2-90,-w*0.3,40,140,100,80,"P_Cream")); p.append(box(-d/2-90,-w*0.3,100,120,80,40,"P_Cream"))   # flour sacks
    return p
def stable(w=600,d=900,wall="P_Brown",roof="P_Shingle",wood="P_DarkWood",hay="P_Straw"):
    """An open-fronted stable: a plank back wall, three stalls on posts, hay in the stalls, a trough out front."""
    h=280; p=[box(d*0.2,0,h/2,d*0.6,w,h,wall)]
    for i in range(int((d*0.6)//60)): p.append(box(-d*0.1+30+i*60,0,h/2,4,w+6,h,wood))
    for sy in (-1,1): p.append(box(-d/2+30,sy*(w/2-20),h/2,30,30,h,wood))
    for y in (-w/6,w/6): p.append(box(-d*0.1,y,80,d*0.5,10,160,wood))   # stall partitions
    for y in (-w/3,0,w/3): p.append(box(0,y,50,160,140,100,hay))
    top=_roof_gable(p,d,w,h,roof,steps=4,step_h=60,ridge="X",trim=wood)
    p+=_place(water_trough(),-d/2-110,0)
    p+=_place(laundry_line(),d*0.1,w/2+120)
    return p

# ---- dense town rows (the issue #35 town references: houses shoulder to shoulder, two or three storeys, jetties,
# dormers, lit windows). A row is one kit mesh of 3-5 houses sharing walls, each a different height, colour and roof,
# so a street of rows reads as a packed terrace rather than detached houses with gaps.
def row_houses(n=4,seed=1,fronts=("P_Plaster","P_Cream","P_White","P_Plaster"),beam="P_DarkWood",roofs=("P_Slate","P_Terracotta","P_Shingle","P_Slate"),glass="P_GlassLit"):
    """n houses in a row along +-Y, doors on -X, no gaps: shared party walls, different storeys (2-3), jettied upper
    floors on some, a dormer or two, chimneys, lit windows. Pivot at the row's floor centre."""
    r=_rnd.Random(seed); p=[]; fh=250; d=720
    widths=[r.uniform(380,520) for _ in range(n)]; total=sum(widths); y=-total/2
    for k in range(n):
        w=widths[k]; cy=y+w/2; y+=w
        storeys=r.choice((2,2,3)); wall=fronts[k%len(fronts)]; rm=roofs[k%len(roofs)]; jetty=r.random()<0.5
        p.append(box(0,cy,30,d+16,w,60,"P_Grey"))
        z=60
        for i in range(storeys):
            dd=d+40 if (jetty and i>0) else d
            if jetty and i>0: _jetty_brackets(p,dd,w,z,beam,2)
            p.append(box(0,cy,z+fh/2,dd,w,fh,wall))
            # a frame only on the street face (the sides are shared)
            xx=-dd/2-4
            for sy in (-1,1): p.append(box(xx,cy+sy*(w/2-7),z+fh/2,14,14,fh,beam))
            p.append(box(xx-1,cy,z+fh-7,16,w,14,beam)); p.append(box(xx-1,cy,z+7,16,w,14,beam))
            p.append(box(xx+2,cy,z+fh/2,12,12,fh*0.8,beam,roll=34 if k%2 else -34))
            wz=z+150
            if i==0:
                _door(p,dd,z=z,wd=100,ht=200,mat=beam,lantern="P_Gold" if k%2==0 else None)
                _window(p,-dd/2-3,cy+w*0.28,wz,(70,80),"x",glass=glass,shutters=beam)
            else:
                for yy in (cy-w*0.26,cy+w*0.26): _window(p,-dd/2-3,yy,wz,(70,90),"x",glass=glass,flowers="P_Rose" if (i==storeys-1 and r.random()<0.5) else None)
            z+=fh
        # a gable roof per house, ridge along X (into the row), so neighbours' roofs step up and down
        rm2=ALT.get(rm,rm)
        for st in range(4):
            f=1-st/4; p.append(box(0,cy,z+27+st*55,d+60,w*f+30,55,rm if st%2==0 else rm2))
        p.append(box(0,cy,z+4*55+6,d+40,50,14,beam))
        if r.random()<0.5: _dormer(p,-d*0.2,cy,z+60,rm,wall,-1 if r.random()<0.5 else 1)
        if r.random()<0.7: _chimney(p,d*0.25,cy+w*0.2,z+4*55-40,h=180)
    # a few things against the street fronts: a barrel, a crate, a hanging sign
    p+=_place(barrel(),-d/2-70,-total/2+90); p+=_place(crate_stack(),-d/2-80,total/2-120)
    p.append(box(-d/2-60,0,320,20,20,16,beam)); p.append(box(-d/2-90,0,290,24,120,70,"P_Gold"))
    return p
def row_houses_stone(n=4,seed=1):
    return row_houses(n,seed,fronts=("P_Stone","P_LightGrey","P_Stone","P_Grey"),beam="P_DarkWood",roofs=("P_Slate","P_Slate","P_DarkGrey","P_Slate"))
def row_houses_timber(n=4,seed=1):
    return row_houses(n,seed,fronts=("P_Plaster","P_Cream","P_Plaster","P_White"),roofs=("P_Shingle","P_Terracotta","P_Straw","P_Shingle"))

# ---- tiny city models for the Overworld table (1 voxel = 1 m on the map, so a city is a few metres across). Pivot at
# the ground centre; each is the city's silhouette from the books, not a plan: Caemlyn's white Inner City on its hill,
# Tar Valon's island and White Tower, the Stone of Tear, Cairhien's topless towers, Illian's canals, Tanchico's harbour
# and Panarch's palace, Fal Dara's keep, Shadar Logoth's blackened ruin, Baerlon's wall and gate, Whitebridge's bridge,
# Emond's Field's green with its thatched roofs and the inn. Unit: v (one voxel, 40 cm).
def _tiny_houses(p,pts,v,wall="P_Plaster",roof="P_Terracotta",h=1.2,seed=1,z0=0.0,avoid=()):
    """Little gabled blocks at pts (centre, metres as voxels). Houses never overlap each other or the footprints in
    `avoid` ((x,y,d,w) boxes such as an inn or a tower): a house that would is shrunk once, then skipped, so no two
    roofs or walls share a plane. z0 raises the bases (Tar Valon's houses stand on the island)."""
    r=_rnd.Random(seed); taken=list(avoid)
    def clash(x,y,d,w): return any(abs(x-tx)<(d+td)/2+v*0.25 and abs(y-ty)<(w+tw)/2+v*0.25 for (tx,ty,td,tw) in taken)
    for (x,y) in pts:
        w=v*r.uniform(1.4,2.2); d=v*r.uniform(1.4,2.4); hh=v*h*r.uniform(0.8,1.3)
        if clash(x,y,d,w):
            w*=0.7; d*=0.7
            if clash(x,y,d,w): continue
        taken.append((x,y,d+v*0.3,w+v*0.3))
        p.append(box(x,y,z0+hh/2,d,w,hh,wall)); p.append(box(x,y,z0+hh+v*0.35,d+v*0.3,w+v*0.3,v*0.7,roof))
def _tiny_wall(p,pts,v,h=1.6,mat="P_LightGrey",tower="P_Grey",closed=True):
    n=len(pts)
    for i in range(n if closed else n-1):
        ax,ay=pts[i]; bx,by=pts[(i+1)%n]; L=math.hypot(bx-ax,by-ay); yaw=math.degrees(math.atan2(by-ay,bx-ax))
        p.append(box((ax+bx)/2,(ay+by)/2,v*h/2,L,v*0.6,v*h,mat,yaw=yaw))
        p.append(box(ax,ay,v*h*0.65,v*1.2,v*1.2,v*h*1.3,tower))
def _ring(cx,cy,r,n,start=0.0): return [(cx+r*math.cos(start+2*math.pi*k/n),cy+r*math.sin(start+2*math.pi*k/n)) for k in range(n)]
def tiny_emondsfield(v=40):
    p=[box(0,0,v*0.1,v*14,v*14,v*0.2,"P_Green")]
    inn=[(v*3.5,v*3.5,v*3.4,v*2.6)]; oak=[(0,0,v*2.8,v*2.8)]
    _tiny_houses(p,_ring(0,0,v*5.2,9),v,"P_Plaster","P_Straw",seed=2,avoid=inn+oak); _tiny_houses(p,_ring(0,0,v*3.0,5,0.3),v,"P_Cream","P_ThatchDark",h=1.0,seed=3,avoid=inn+oak)
    p.append(box(v*3.5,v*3.5,v*1.2,v*3,v*2.2,v*2.4,"P_Plaster")); p.append(box(v*3.5,v*3.5,v*2.8,v*3.4,v*2.6,v*0.8,"P_RedBrick"))   # the Winespring Inn
    p.append(box(0,0,v*1.4,v*0.6,v*0.6,v*2.8,"P_Trunk")); p.append(box(0,0,v*3.2,v*2.6,v*2.6,v*1.6,"P_LeafGreen"))             # the old oak on the Green
    return p
def tiny_baerlon(v=40):
    p=[box(0,0,v*0.1,v*16,v*14,v*0.2,"P_Olive")]
    _tiny_wall(p,[(-v*7,-v*6),(v*7,-v*6),(v*7,v*6),(-v*7,v*6)],v,1.4,"P_Brown","P_DarkWood")
    _tiny_houses(p,[(x*v,y*v) for x in (-4.5,-1.5,1.5,4.5) for y in (-3.5,0,3.5)],v,"P_Plaster","P_Shingle",seed=4)
    p.append(box(-v*7,0,v*1.2,v*1.4,v*3,v*2.4,"P_DarkWood"))   # the gate
    return p
def tiny_whitebridge(v=40):
    p=[box(0,0,v*0.1,v*14,v*10,v*0.2,"P_Olive"),box(0,0,v*0.2,v*3,v*12,v*0.3,"P_Blue")]   # the Arinelle
    for i in range(7): a=(i-3)/3.0; p.append(box(0,-v*6+i*v*2,v*(2.6-2.2*a*a),v*1.6,v*2.2,v*0.5,"P_White"))   # the White Bridge
    for sy in (-1,1): p.append(box(0,sy*v*6.5,v*0.8,v*1.4,v*1.2,v*1.6,"P_White"))
    _tiny_houses(p,[(x*v,y*v) for x in (-4,-2.5) for y in (-3,0,3)]+[(x*v,y*v) for x in (2.5,4) for y in (-3,0,3)],v,"P_Cream","P_Straw",seed=5)
    return p
def tiny_shadarlogoth(v=40):
    p=[box(0,0,v*0.1,v*16,v*16,v*0.2,"P_DarkGrey")]
    r=_rnd.Random(9)
    for (x,y) in _ring(0,0,v*5,12)+_ring(0,0,v*2.5,6,0.5):
        hh=v*r.uniform(1.5,4.5); p.append(box(x,y,hh/2,v*1.6,v*1.6,hh,"P_Black" if r.random()<0.5 else "P_DarkGrey"))
    _tiny_wall(p,_ring(0,0,v*7.5,10),v,1.2,"P_DarkGrey","P_Black")
    return p
def tiny_faldara(v=40):
    p=[box(0,0,v*0.1,v*14,v*14,v*0.2,"P_Olive")]
    _tiny_wall(p,[(-v*6,-v*6),(v*6,-v*6),(v*6,v*6),(-v*6,v*6)],v,2.2,"P_Grey","P_DarkGrey")
    p.append(box(0,0,v*2.5,v*5,v*5,v*5,"P_Grey")); p.append(box(0,0,v*5.6,v*2.4,v*2.4,v*1.2,"P_Slate"))   # the keep
    _tiny_houses(p,[(x*v,y*v) for x in (-4,4) for y in (-4,-1.5,1.5,4)],v,"P_Stone","P_Slate",h=1.0,seed=6,avoid=[(0,0,v*5,v*5)])
    return p
def tiny_caemlyn(v=40):
    p=[box(0,0,v*0.1,v*20,v*20,v*0.2,"P_Olive")]
    _tiny_wall(p,_ring(0,0,v*9,14),v,1.6,"P_LightGrey","P_Grey")                       # the outer wall
    _tiny_houses(p,_ring(0,0,v*6.5,16)+_ring(0,0,v*4.8,10,0.2),v,"P_Cream","P_Terracotta",seed=7,avoid=[(0,0,v*7,v*7)])
    p.append(box(0,0,v*1.0,v*7,v*7,v*2.0,"P_Grey")); _tiny_wall(p,_ring(0,0,v*3.4,8),v,1.6,"P_White","P_White")   # the hill and the Inner City wall
    for (x,y) in _ring(0,0,v*2.2,5): p.append(box(x,y,v*3.6,v*1.2,v*1.2,v*3.2,"P_White")); p.append(box(x,y,v*5.4,v*0.8,v*0.8,v*0.8,"P_Gold"))
    p.append(box(0,0,v*4.0,v*2.4,v*2.4,v*4.0,"P_White")); p.append(box(0,0,v*6.3,v*1.2,v*1.2,v*0.6,"P_Gold"))   # the palace
    return p
def tiny_tarvalon(v=40):
    p=[box(0,0,v*0.1,v*22,v*16,v*0.2,"P_Blue"),box(0,0,v*0.5,v*14,v*9,v*0.6,"P_Olive")]   # the island in the Erinin
    _tiny_wall(p,[(-v*7,-v*4.5),(v*7,-v*4.5),(v*7,v*4.5),(-v*7,v*4.5)],v,1.4,"P_White","P_White")
    _tiny_houses(p,[(x*v,y*v) for x in (-5,-3,-1,1,3,5) for y in (-3,3)],v,"P_White","P_Slate",h=1.1,seed=8,z0=v*0.8,avoid=[(0,0,v*2.6,v*2.6)])
    p.append(box(0,0,v*4.5,v*2.6,v*2.6,v*9,"P_White")); p.append(box(0,0,v*9.3,v*1.6,v*1.6,v*0.6,"P_White"))   # the White Tower
    for sx in (-1,1): p.append(box(sx*v*9,0,v*0.9,v*4,v*1.2,v*0.6,"P_LightGrey"))   # the bridges
    return p
def tiny_cairhien(v=40):
    p=[box(0,0,v*0.1,v*16,v*16,v*0.2,"P_Olive")]
    _tiny_wall(p,[(-v*7,-v*7),(v*7,-v*7),(v*7,v*7),(-v*7,v*7)],v,1.4,"P_Grey","P_DarkGrey")
    _tiny_houses(p,[(x*v,y*v) for x in (-5,-2.5,0,2.5,5) for y in (-5,-2.5,0,2.5,5) if (x,y)!=(0,0)],v,"P_LightGrey","P_Slate",seed=10)
    for (x,y,hh) in ((0,0,6),(-3,3,4.5),(3,-3,5),(2,3,4)): p.append(box(x*v,y*v,v*hh/2,v*1.2,v*1.2,v*hh,"P_LightGrey"))   # the topless towers
    return p
def tiny_tear(v=40):
    p=[box(0,0,v*0.1,v*18,v*16,v*0.2,"P_Olive"),box(0,v*7,v*0.2,v*18,v*3,v*0.3,"P_Blue")]   # the Erinin mouth
    _tiny_houses(p,[(x*v,y*v) for x in (-6,-3.5,-1,1.5,4,6.5) for y in (-5,-2.5,0,2.5)],v,"P_Cream","P_Terracotta",seed=11,avoid=[(-v*5,v*3,v*5,v*4)])
    p.append(box(-v*5,v*3,v*3,v*5,v*4,v*6,"P_DarkGrey")); p.append(box(-v*5,v*3,v*6.4,v*3,v*2.4,v*0.8,"P_Grey"))   # the Stone of Tear
    for i in range(4): p.append(box(v*2+i*v*2,v*6,v*0.5,v*1.2,v*3,v*0.4,"P_DarkWood"))   # docks
    return p
def tiny_illian(v=40):
    p=[box(0,0,v*0.1,v*18,v*16,v*0.2,"P_Olive")]
    for x in (-v*4,0,v*4): p.append(box(x,0,v*0.15,v*1.2,v*16,v*0.3,"P_Blue"))   # the canals
    p.append(box(0,0,v*0.15,v*18,v*1.2,v*0.3,"P_Blue"))
    _tiny_houses(p,[(x*v,y*v) for x in (-6,-2,2,6) for y in (-6,-3,3,6)],v,"P_White","P_Teal",seed=12,avoid=[(0,v*0.5,v*4.6,v*3.6)])
    p.append(box(0,v*0.5,v*1.5,v*4,v*3,v*3,"P_White")); p.append(box(0,v*0.5,v*3.4,v*4.6,v*3.6,v*0.8,"P_Gold"))   # the Great Hall of the Council
    return p
def tiny_tanchico(v=40):
    p=[box(0,0,v*0.1,v*18,v*16,v*0.2,"P_Sand"),box(v*5,0,v*0.2,v*8,v*8,v*0.3,"P_Blue")]   # the circular harbour
    _tiny_houses(p,[(x*v,y*v) for x in (-7,-4.5,-2) for y in (-6,-3,0,3,6)],v,"P_Adobe","P_Clay",h=1.4,seed=13,avoid=[(-v*5,0,v*3,v*3)])
    p.append(box(-v*5,0,v*2.2,v*3,v*3,v*4.4,"P_Cream")); p.append(box(-v*5,0,v*4.8,v*2,v*2,v*1.2,"P_Gold"))   # the Panarch's palace
    for a in range(0,360,45): p.append(box(v*5+v*4.6*math.cos(math.radians(a)),v*4.6*math.sin(math.radians(a)),v*0.5,v*1.2,v*1.2,v*1.0,"P_Sand"))
    return p
def tiny_house(v=40,seed=1,wall="P_Plaster",roof="P_Straw"):
    """One map-scale house for the settlement clusters (ow_houses.json): the same voxel language as the tiny cities."""
    r=_rnd.Random(seed); w=v*r.uniform(1.6,2.4); d=v*r.uniform(1.8,2.8); hh=v*r.uniform(1.0,1.5)
    p=[box(0,0,hh/2,d,w,hh,wall),box(0,0,hh+v*0.35,d+v*0.3,w+v*0.3,v*0.7,roof),box(0,0,hh+v*0.9,d*0.6,w*0.4,v*0.4,ALT.get(roof,roof))]
    p.append(box(d*0.3,w*0.3,hh+v*1.1,v*0.4,v*0.4,v*0.9,"P_Grey"))   # chimney
    return p
def tiny_village(v=40,seed=1):
    """A nameless town or village on the map: a handful of thatched houses round a well."""
    p=[]; _tiny_houses(p,_ring(0,0,v*2.6,6,seed*0.7),v,"P_Plaster","P_Straw",h=1.0,seed=seed); p.append(cyl(0,0,v*0.3,v*0.8,v*0.6,"P_Grey"))
    return p
def tiny_city(v=40,seed=1):
    """A nameless city on the map: a walled square of stone houses with a hall in the middle."""
    p=[]; _tiny_wall(p,[(-v*5,-v*5),(v*5,-v*5),(v*5,v*5),(-v*5,v*5)],v,1.4,"P_LightGrey","P_Grey")
    _tiny_houses(p,[(x*v,y*v) for x in (-3,0,3) for y in (-3,0,3) if (x,y)!=(0,0)],v,"P_Stone","P_Slate",seed=seed,avoid=[(0,0,v*2.4,v*2.4)]); p.append(box(0,0,v*1.5,v*2.4,v*2.4,v*3,"P_LightGrey"))
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
def pine(h=600,trunk="P_Trunk",leaf="P_LeafDark",tiers=4,snow=None):
    p=[box(0,0,h*0.25,60,60,h*0.5,trunk)]
    z=h*0.35
    for i in range(tiers):
        f=1-i/tiers; s=320*f+60; hh=h*0.16
        p.append(box(0,0,z+hh/2,s,s,hh,leaf));
        if snow: p.append(box(0,0,z+hh,s*0.8,s*0.8,20,snow))
        z+=hh*0.8
    return p
def palm(h=700,trunk="P_Trunk",leaf="P_LeafGreen"):
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
def cactus(h=300,mat="P_LeafOlive"):
    return [box(0,0,h/2,60,60,h,mat), box(80,0,h*0.6,100,50,50,mat), box(100,0,h*0.75,50,50,120,mat), box(-70,0,h*0.45,80,50,50,mat), box(-90,0,h*0.6,50,50,100,mat)]
def reeds(h=180,mat="P_LeafOlive"):
    p=[]
    for i,(x,y) in enumerate([(0,0),(40,30),(-30,40),(20,-40),(-40,-20)]): p.append(box(x,y,h/2+i*8,16,16,h+i*16,mat))
    return p
def shrub_flower(mat="P_LeafGreen",flower="P_Rose"):
    return [box(0,0,50,120,120,100,mat), box(40,30,110,40,40,30,flower), box(-40,-20,105,40,40,30,flower), box(10,-45,112,40,40,30,"P_Gold")]
def snow_patch(): return [box(0,0,4,260,220,8,"P_Snow"), box(90,60,6,160,140,8,"P_Snow")]
def birch(h=550,trunk="P_White",leaf="P_LeafPale"):
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
 # the lived-in pass: houses with yards, farm buildings, yard and field props, ground cover, more trees
 "SM_TimberHouseYard":lambda:yard(timber_house(),820,620,seed=11),"SM_TimberHouse2Yard":lambda:yard(timber_house(storeys=2,roof="tile",jetty=True),820,620,seed=12,fence="hedge"),
 "SM_CottageYard":lambda:yard(thatched_cottage(),640,520,seed=13,fence="stone",n_extra=3),"SM_CottageDormer":cottage_dormer,"SM_CottageDormerYard":lambda:yard(cottage_dormer(),860,640,seed=14),
 "SM_LHouse":l_house,"SM_LHouseYard":lambda:yard(l_house(),880,620,seed=15,fence="hedge"),"SM_StoneTimberHouse":stone_timber_house,"SM_StoneTimberHouseYard":lambda:yard(stone_timber_house(),820,640,seed=16,fence="stone"),
 "SM_Farmhouse":farmhouse,"SM_FarmhouseYard":lambda:yard(farmhouse(),1100,560,seed=17,n_extra=5),"SM_Watermill":watermill,"SM_Stable":stable,
 "SM_LaundryLine":laundry_line,"SM_WaterTrough":water_trough,"SM_Wheelbarrow":wheelbarrow,"SM_LanternPost":lantern_post,"SM_Hedge":hedge,"SM_StoneWall":stone_wall,"SM_Gate":gate,"SM_PicketFence":picket_fence,
 "SM_Scarecrow":scarecrow,"SM_CropRows":crop_rows,"SM_Beehive":beehive,"SM_ChickenCoop":chicken_coop,"SM_LogBench":log_bench,"SM_Stump":stump,"SM_FallenLog":fallen_log,"SM_RockSmall":rock_small,
 "SM_FlowerPatch":flower_patch,"SM_GrassTuft":grass_tuft,"SM_Oak":oak,"SM_AppleTree":apple_tree,"SM_BushRound":bush_round,"SM_MushroomCluster":mushroom_cluster,
 # dense town rows
 "SM_Row3":lambda:row_houses(3,1),"SM_Row4":lambda:row_houses(4,2),"SM_Row5":lambda:row_houses(5,3),"SM_RowStone4":lambda:row_houses_stone(4,4),"SM_RowStone5":lambda:row_houses_stone(5,5),
 "SM_RowTimber4":lambda:row_houses_timber(4,6),"SM_RowTimber3":lambda:row_houses_timber(3,7),
 # the Overworld table's tiny cities
 "SM_Tiny_EmondsField":tiny_emondsfield,"SM_Tiny_Baerlon":tiny_baerlon,"SM_Tiny_Whitebridge":tiny_whitebridge,"SM_Tiny_ShadarLogoth":tiny_shadarlogoth,"SM_Tiny_FalDara":tiny_faldara,
 "SM_Tiny_Caemlyn":tiny_caemlyn,"SM_Tiny_TarValon":tiny_tarvalon,"SM_Tiny_Cairhien":tiny_cairhien,"SM_Tiny_Tear":tiny_tear,"SM_Tiny_Illian":tiny_illian,"SM_Tiny_Tanchico":tiny_tanchico,
 "SM_Tiny_Village":tiny_village,"SM_Tiny_Village2":lambda:tiny_village(seed=2),"SM_Tiny_City":tiny_city,"SM_Tiny_City2":lambda:tiny_city(seed=2),
 "SM_Tiny_House":tiny_house,"SM_Tiny_House2":lambda:tiny_house(seed=2,wall="P_Cream",roof="P_ThatchDark"),"SM_Tiny_House3":lambda:tiny_house(seed=3,wall="P_Stone",roof="P_Slate"),"SM_Tiny_Barn":lambda:tiny_house(seed=4,wall="P_Brown",roof="P_Shingle"),
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
def _expand_recolor(recolor):
    """P_Terracotta -> P_Slate also sends the roof's second shade P_Terracotta2 -> P_Slate2 (or the plain target when it has none)."""
    if not recolor: return recolor
    out=dict(recolor)
    for a,b in recolor.items():
        if a in ALT and ALT[a] not in out: out[ALT[a]]=ALT.get(b,b)
    return out
def kit_entry(name,recolor=None,weight=1):
    """Weighted spawner entry for a kit mesh with optional {material->material} recolour."""
    parts=[box(0,0,1,20,20,2,KIT[name]()[0][4])]+list(KIT[name]())
    order=kit_slot_mats(parts); recolor=_expand_recolor(recolor)
    mats=[PM((recolor or {}).get(m,m)) for m in order] if recolor else None
    return (f"{KIT_FOLDER}/{name}.{name}",weight,mats)
def kit_mesh(name): return f"{KIT_FOLDER}/{name}.{name}"
def kit_mats(name,recolor=None):
    parts=[box(0,0,1,20,20,2,KIT[name]()[0][4])]+list(KIT[name]()); recolor=_expand_recolor(recolor)
    return [PM((recolor or {}).get(m,m)) for m in kit_slot_mats(parts)]
