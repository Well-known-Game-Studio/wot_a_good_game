#!/usr/bin/env python3
"""The second Overworld: a terraced voxel diorama built from the map texture (blender/overworld-full.png), in the
style of the issue #35 / #51 references (stepped cliffs with strata, sea as a slab with a pale shore, forest clumps).

Classifies every texel (sea, coast sand, grass, dry plain, forest, hill skirt, mountain), resamples to CELL cm cells on
the same texture->world mapping as the live Overworld (ow_fit.py: texel (0,0) at (-13271,-17662), 40.6 cm per texel),
gives each cell a terrace height (mountains rise by distance into their mass, one terrace per voxel, rolling +1 on the
plains, sea two below), run-merges rows of equal (material, height) into boxes and writes ow_voxel.json:
  boxes: [(palette, x, y, z, sx, sy, sz)] for the terrain spawner (top surface slabs + the sea slab),
  trees: [(x, y, z, kind, scale)] on the forest mask (dense) and scattered over grass (sparse),
  heights: a coarse grid to put portals, starts, houses and landmarks on the ground (ow2_build.py samples it).
Needs Pillow + numpy: python/env/bin/python unreal/Tools/towns/ow_voxel.py
"""
import json, math, os
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.abspath(os.path.join(HERE,"..","..",".."))
TEX=os.path.join(ROOT,"blender","overworld-full.png"); OUT=os.path.join(HERE,"ow_voxel.json")
# texture -> world (ow_fit.py)
AX,AY=-230.0,-360.0; EX,EY=4484.0,4736.0; TX,TY=0.29,0.30; CX,CY=0.27,-0.16
CELL=100.0          # one voxel = 1 m; the texture is ~40.6 cm per texel, so a cell averages ~2.5 texels
STEP=100.0          # one terrace = one voxel
SEA_Z=-200.0        # sea surface (top of the water slab); sand at 0, grass 0..1, hills 1, mountains 2..9
def px_to_world(c,r):
    u,v=c/761.0,r/762.0; lx=0.5+CX+(u-0.5)/TX; ly=0.5+CY+(v-0.5)/TY
    return (AX-EX)+lx*2*EX,(AY-EY)+ly*2*EY
def world_to_px(x,y):
    lx=(x-(AX-EX))/(2*EX); ly=(y-(AY-EY))/(2*EY)
    return ((lx-0.5-CX)*TX+0.5)*761.0,((ly-0.5-CY)*TY+0.5)*762.0
def dilate(m,it=1):
    for _ in range(it): m=m|np.roll(m,1,0)|np.roll(m,-1,0)|np.roll(m,1,1)|np.roll(m,-1,1)
    return m
def erode(m,it=1):
    for _ in range(it): m=m&np.roll(m,1,0)&np.roll(m,-1,0)&np.roll(m,1,1)&np.roll(m,-1,1)
    return m
def dist_in(m,maxd=30):
    d=np.zeros(m.shape,int); cur=m.copy()
    for k in range(1,maxd+1):
        cur=erode(cur); d[cur]=k
        if not cur.any(): break
    return d+m.astype(int)
def blur(a,rad):
    k=2*rad+1; c=np.cumsum(np.cumsum(np.pad(a,rad+1),0),1)
    out=(c[k:,k:]-c[:-k,k:]-c[k:,:-k]+c[:-k,:-k])/(k*k)
    return out[:a.shape[0],:a.shape[1]]
SEA,SAND,GRASS,FOREST,HILL,MOUNT,DRY,MEADOW,SNOW,RIVER,ROAD,LAKE=range(12)
PALETTE={SAND:"MI_Ground_Sand",GRASS:"MI_Ground_Lush",FOREST:"MI_Ground_Lush",HILL:"MI_Ground_Dirt",MOUNT:"MI_Ground_Rock",DRY:"MI_Ground_Tundra",MEADOW:"MI_Ground_Tundra",SNOW:"G_Snow",RIVER:"MI_VoxelWater",ROAD:"MI_Ground_Dirt",LAKE:"MI_VoxelWater"}
STRATA={MOUNT:"MI_Ground_DarkRock",HILL:"MI_Ground_Dirt",GRASS:"MI_Ground_Dirt",FOREST:"MI_Ground_Dirt",DRY:"MI_Ground_Dirt",SAND:"MI_Ground_Sand",MEADOW:"MI_Ground_Dirt",SNOW:"MI_Ground_Rock",RIVER:"MI_Ground_Dirt",ROAD:"MI_Ground_Dirt",LAKE:"MI_Ground_Dirt"}   # the exposed face below a terrace
MIN_ROAD_CELLS=15; MAX_LAKE_CELLS=400
BASE=os.path.join(ROOT,"maps","newwotbase-1.png"); BX,BY,SX,SY=20.0,-60.0,11.6,9.05   # texture texel -> base map pixel (ow_fit.py)
MIN_RIVER_CELLS=12   # a connected run of river cells shorter than this is a blue place name, not a river
SNOW_H=7            # mountain terraces at or above this wear snow
def classify():
    im=np.asarray(Image.open(TEX).convert("RGB")).astype(int); H,W,_=im.shape
    r,g,b=im[...,0],im[...,1],im[...,2]
    sea=(b>r+40)&(b>g+15)&(r<170)
    forest=(g>r+25)&(g<175)&(r<125)&(b<120)
    grass=(g>r+10)&~forest&~sea
    tan=(r>g+12)&(r>b+35)&~sea
    pale=(r>185)&(g>185)&(b>175)&~sea
    mount=tan|pale
    dry=(r>160)&(g>150)&(b<130)&~mount&~sea
    cls=np.full((H,W),GRASS,np.uint8); cls[sea]=SEA; cls[dry]=DRY; cls[forest]=FOREST; cls[mount]=MOUNT
    land=cls!=SEA
    cls[dilate(~land,2)&land]=SAND
    dist=dist_in(mount)
    h=np.zeros((H,W),int); h[mount]=np.clip((dist[mount]/2.0).astype(int)+2,2,9)
    skirt=dilate(mount,2)&~mount&land; cls[skirt]=HILL; h[skirt]=1
    rng=np.random.default_rng(3); noise=blur(rng.random((H,W)),9); noise2=blur(rng.random((H,W)),5)
    roll=(cls==GRASS)|(cls==DRY)|(cls==FOREST)
    h[roll&(noise>0.55)]=1; h[roll&(noise>0.585)]=2            # two gentle terraces across the plains
    cls[(cls==GRASS)&(noise2>0.535)]=MEADOW                       # paler meadow patches break up the green
    cls[mount&(h>=SNOW_H)]=SNOW                                  # snow on the high terraces
    h[~land]=-2
    return cls,h
def components(mask,nx,ny):
    """8-connected runs of true cells: a list of cell lists."""
    seen=np.zeros_like(mask); out=[]
    for j in range(ny):
        for i in range(nx):
            if not mask[j,i] or seen[j,i]: continue
            comp=[(j,i)]; seen[j,i]=True; k=0
            while k<len(comp):
                cj,ci=comp[k]; k+=1
                for dj in (-1,0,1):
                    for di in (-1,0,1):
                        jj,ii=cj+dj,ci+di
                        if 0<=jj<ny and 0<=ii<nx and mask[jj,ii] and not seen[jj,ii]: seen[jj,ii]=True; comp.append((jj,ii))
            out.append(comp)
    return out
def roads(nx,ny,x0,y0):
    """Road cells from the base map: its roads are dotted brown lines (the borders are dark red dash-dots, dropped).
    Dots are joined by taking any road pixel in a cell and keeping only long connected runs."""
    im=np.asarray(Image.open(BASE).convert("RGB")); r=im[...,0].astype(int); g=im[...,1].astype(int); b=im[...,2].astype(int)
    road=(r>100)&(r<200)&(g>35)&(g<140)&(b<95)&(r>g+25)&~((r>150)&(g<70)&(b<40))   # the dots run (120,50,0)..(150,110,60)
    border=dilate((r>120)&(g<90)&(b>g-18)&(r>g+50),1)   # the borders' red has b about equal to g; the road dots' brown has b well under g
    # only dots lying on open country count: the mountains' brown shading passes the colour test too
    plain=((g>r+10)&(g>b+10))|((r>150)&(g>140)&(b<140)&(r<200)&(g<190))
    p4=plain[::4,::4].astype(np.float64); o4=blur(p4,2)>0.55                     # at quarter resolution (window ~20 px)
    open_=np.repeat(np.repeat(o4,4,axis=0),4,axis=1)[:plain.shape[0],:plain.shape[1]]
    road&=~border; road&=open_; BH,BW=road.shape
    mask=np.zeros((ny,nx),bool)
    for j in range(ny):
        for i in range(nx):
            c0,r0=world_to_px(x0+i*CELL,y0+j*CELL); c1,r1=world_to_px(x0+(i+1)*CELL,y0+(j+1)*CELL)
            bx0,by0=max(0,int(BX+SX*c0)),max(0,int(BY+SY*r0)); bx1,by1=min(BW,int(BX+SX*c1)),min(BH,int(BY+SY*r1))
            if bx1>bx0 and by1>by0 and road[by0:by1,bx0:bx1].sum()>=3: mask[j,i]=True
    keep=np.zeros_like(mask)
    for comp in components(mask,nx,ny):   # dots are dense enough that a road is 8-connected at cell size; keep the long runs
        js=[q[0] for q in comp]; is_=[q[1] for q in comp]; bw=max(is_)-min(is_)+1; bh=max(js)-min(js)+1
        if len(comp)>=MIN_ROAD_CELLS and max(bw,bh)>=12:
            for (jj,ii) in comp: keep[jj,ii]=True
    return keep
def rivers(nx,ny,x0,y0):
    """River cells from the base map: its rivers are thin saturated blue lines (the sea is a paler blue). For every
    1 m cell the matching block of base pixels is tested for river blue; runs of connected river cells shorter than
    MIN_RIVER_CELLS are dropped (the map's blue lettering)."""
    im=np.asarray(Image.open(BASE).convert("RGB")); r=im[...,0].astype(int); g=im[...,1].astype(int); b=im[...,2].astype(int)
    riv=(b>r+60)&(b>g+40)&(b<220)&(r<120)   # saturated river blue; the sea is (170,200,235)-ish and fails r<120 / b<220
    BH,BW=riv.shape
    mask=np.zeros((ny,nx),bool)
    for j in range(ny):
        for i in range(nx):
            c0,r0=world_to_px(x0+i*CELL,y0+j*CELL); c1,r1=world_to_px(x0+(i+1)*CELL,y0+(j+1)*CELL)
            bx0,by0=int(BX+SX*c0),int(BY+SY*r0); bx1,by1=int(BX+SX*c1),int(BY+SY*r1)
            bx0,bx1=max(0,bx0),min(BW,bx1); by0,by1=max(0,by0),min(BH,by1)
            if bx1<=bx0 or by1<=by0: continue
            if riv[by0:by1,bx0:bx1].sum()>=6: mask[j,i]=True
    # connected runs (8-neighbour) at cell resolution; keep the long ones
    seen=np.zeros_like(mask); keep=np.zeros_like(mask)
    for j in range(ny):
        for i in range(nx):
            if not mask[j,i] or seen[j,i]: continue
            comp=[(j,i)]; seen[j,i]=True; k=0
            while k<len(comp):
                cj,ci=comp[k]; k+=1
                for dj in (-1,0,1):
                    for di in (-1,0,1):
                        jj,ii=cj+dj,ci+di
                        if 0<=jj<ny and 0<=ii<nx and mask[jj,ii] and not seen[jj,ii]: seen[jj,ii]=True; comp.append((jj,ii))
            js=[q[0] for q in comp]; is_=[q[1] for q in comp]; bw=max(is_)-min(is_)+1; bh=max(js)-min(js)+1
            blob=len(comp)>0.45*bw*bh and max(bw,bh)<16      # a filled square is a sigil or flag on the map, not a river
            if len(comp)>=MIN_RIVER_CELLS and not blob:
                for (jj,ii) in comp: keep[jj,ii]=True
    return keep
def main():
    cls,h=classify(); H,W=cls.shape
    # world grid of CELL cm cells covering the texture; each cell takes the majority class / mean height of its texels
    x0,y0=px_to_world(0,0); x1,y1=px_to_world(W,H)
    nx=int((x1-x0)//CELL); ny=int((y1-y0)//CELL)
    cc=np.zeros((ny,nx),np.uint8); ch=np.zeros((ny,nx),int)
    for j in range(ny):
        for i in range(nx):
            c0,r0=world_to_px(x0+i*CELL,y0+j*CELL); c1,r1=world_to_px(x0+(i+1)*CELL,y0+(j+1)*CELL)
            cs,ce=max(0,int(c0)),min(W,int(math.ceil(c1))); rs,re=max(0,int(r0)),min(H,int(math.ceil(r1)))
            if ce<=cs or re<=rs: cc[j,i]=SEA; ch[j,i]=-2; continue
            blk=cls[rs:re,cs:ce]; counts=np.bincount(blk.ravel(),minlength=12); k=int(counts.argmax())
            cc[j,i]=k; ch[j,i]=int(round(h[rs:re,cs:ce].mean())) if k!=SEA else -2
    # lakes: enclosed sea-class cells (not connected to the map edge, small) sit one terrace below their shore, not two
    seacells=(cc==SEA)
    for comp in components(seacells,nx,ny):
        edge=any(j==0 or i==0 or j==ny-1 or i==nx-1 for (j,i) in comp)
        if not edge and len(comp)<=MAX_LAKE_CELLS:
            shore=[ch[jj,ii] for (j,i) in comp for (jj,ii) in ((j-1,i),(j+1,i),(j,i-1),(j,i+1)) if 0<=jj<ny and 0<=ii<nx and cc[jj,ii]!=SEA]
            lvl=max(-1,(min(shore) if shore else 0)-1)
            for (j,i) in comp: cc[j,i]=LAKE; ch[j,i]=lvl
    # rivers: carved one terrace into whatever they cross (never below the coast), water on top
    rv=rivers(nx,ny,x0,y0)
    for j in range(ny):
        for i in range(nx):
            if rv[j,i] and cc[j,i] not in (SEA,LAKE): cc[j,i]=RIVER; ch[j,i]=max(-1,ch[j,i]-1)
    print("river cells",int(rv.sum()))
    # roads: a dirt line at ground level across grass, meadow, dry plain and forest (fords across rivers)
    rd=roads(nx,ny,x0,y0)
    for j in range(ny):
        for i in range(nx):
            if rd[j,i] and cc[j,i] in (GRASS,MEADOW,DRY,FOREST,HILL): cc[j,i]=ROAD
    print("road cells",int(rd.sum()))
    # boxes: for each row, runs of equal (class, height) -> one top slab (STEP thick, the terrace's material) plus, where
    # a neighbour is lower, the exposed strata face as a darker slab below (down to the lowest neighbour)
    boxes=[]
    def world(i,j): return x0+(i+0.5)*CELL, y0+(j+0.5)*CELL
    # top slabs: greedy rectangles of equal (class, height) - a row run first, then grown downwards while the rows
    # below match exactly, so plains become a few big slabs instead of hundreds of strips (far fewer instances for the
    # distance field and Lumen to track)
    done=np.zeros((ny,nx),bool)
    for j in range(ny):
        i=0
        while i<nx:
            if done[j,i] or cc[j,i]==SEA: i+=1; continue
            k=int(cc[j,i]); z=int(ch[j,i]); i2=i
            while i2+1<nx and not done[j,i2+1] and cc[j,i2+1]==k and ch[j,i2+1]==z: i2+=1
            j2=j
            while j2+1<ny and j2-j<40 and all((not done[j2+1,ii]) and cc[j2+1,ii]==k and ch[j2+1,ii]==z for ii in range(i,i2+1)): j2+=1
            done[j:j2+1,i:i2+1]=True
            xa,ya=world(i,j); xb,yb=world(i2,j2)
            boxes.append((PALETTE[k],(xa+xb)/2,(ya+yb)/2,z*STEP-STEP/2,(i2-i+1)*CELL,(j2-j+1)*CELL,STEP))
            i=i2+1
    # strata under terrace edges: per row run, the drop to the lowest of the four neighbours along the run
    for j in range(ny):
        i=0
        while i<nx:
            k=int(cc[j,i]); z=int(ch[j,i]); i2=i
            while i2+1<nx and cc[j,i2+1]==k and ch[j,i2+1]==z: i2+=1
            if k!=SEA:
                lows=[]
                for ii in range(i,i2+1):
                    nb=[ch[jj,ii2] for jj,ii2 in ((j-1,ii),(j+1,ii),(j,ii-1),(j,ii+1)) if 0<=jj<ny and 0<=ii2<nx]
                    lows.append(min(nb) if nb else z)
                lo=min(lows)
                if lo<z-1 or (lo<z and k in (MOUNT,HILL)):
                    xa,_=world(i,j); xb,yb=world(i2,j)
                    top=z*STEP-STEP; bot=max(lo,-2)*STEP
                    boxes.append((STRATA[k],(xa+xb)/2,yb,(top+bot)/2,(i2-i+1)*CELL,CELL,top-bot))
            i=i2+1
    # the sea: one slab under everything (its top at SEA_Z); the land sits on strata down to -2 so coasts read as cliffs
    boxes.append(("MI_VoxelWater",(x0+x1)/2,(y0+y1)/2,SEA_Z-50,x1-x0,y1-y0,100))
    # trees: dense on forest cells, sparse on grass, seeded; the kit's oaks, round bushes and pines
    # no trees on the city sites (ow_sites.json: the tiny models stand 420 cm behind each portal)
    try:
        sites=json.load(open(os.path.join(HERE,"ow_sites.json")))
        clear=[(st["x"]-420*math.cos(math.radians(st["yaw"])),st["y"]-420*math.sin(math.radians(st["yaw"]))) for st in sites.values()]
    except Exception: clear=[]
    rng=np.random.default_rng(7); trees=[]
    for j in range(ny):
        for i in range(nx):
            k=int(cc[j,i])
            p=0.55 if k==FOREST else (0.03 if k in (GRASS,MEADOW) else (0.06 if k==HILL else 0.0))
            if k in (ROAD,LAKE,RIVER): p=0.0
            n=int(p)+ (1 if rng.random()<(p-int(p)) else 0)
            for _ in range(n):
                x,y=world(i,j); x+=rng.uniform(-45,45); y+=rng.uniform(-45,45)
                if any(abs(x-cx)<700 and abs(y-cy)<700 for (cx,cy) in clear): continue
                kind="SM_Pine" if (k==HILL or rng.random()<0.25) else ("SM_Oak" if rng.random()<0.6 else "SM_BushRound")
                trees.append((round(x),round(y),int(ch[j,i])*STEP,kind,round(float(rng.uniform(0.35,0.55)),2)))
    pal={ROAD:(150,110,70),LAKE:(80,140,230),SEA:(70,120,200),SAND:(222,205,150),GRASS:(120,180,90),FOREST:(60,120,60),HILL:(170,150,110),MOUNT:(190,175,150),DRY:(175,165,95),MEADOW:(160,200,120),SNOW:(240,240,245),RIVER:(60,110,220)}
    pv=np.zeros((ny,nx,3),np.uint8)
    for k,c in pal.items(): pv[cc==k]=c
    pv=np.clip(pv.astype(int)+(ch[...,None]*7),0,255).astype(np.uint8)
    Image.fromarray(pv).resize((nx*3,ny*3),Image.NEAREST).save(os.path.join(HERE,"ow_voxel_preview.png"))
    data={"cell":CELL,"step":STEP,"x0":x0,"y0":y0,"nx":nx,"ny":ny,"heights":ch.tolist(),"classes":cc.tolist(),"boxes":boxes,"trees":trees}
    json.dump(data,open(OUT,"w"))
    # the editor's read_file tool takes at most 80 KB a call: stage the same data as numbered parts under Saved/PlayTest
    parts=os.path.join(ROOT,"unreal","Saved","PlayTest","ow_voxel_parts"); os.makedirs(parts,exist_ok=True)
    for f in os.listdir(parts): os.remove(os.path.join(parts,f))
    text=json.dumps(data,separators=(",",":")); n=0
    for i in range(0,len(text),70000):
        open(os.path.join(parts,f"{n:04d}.txt"),"w").write(text[i:i+70000]); n+=1
    open(os.path.join(parts,"count.txt"),"w").write(str(n))
    print(f"cells {nx}x{ny}, boxes {len(boxes)}, trees {len(trees)}, classes {np.bincount(cc.ravel(),minlength=12).tolist()} -> {OUT} ({n} parts staged)")
if __name__=="__main__": main()
