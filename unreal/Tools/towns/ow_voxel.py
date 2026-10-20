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
SEA,SAND,GRASS,FOREST,HILL,MOUNT,DRY,MEADOW,SNOW,RIVER=range(10)
PALETTE={SAND:"MI_Ground_Sand",GRASS:"MI_Ground_Lush",FOREST:"MI_Ground_Lush",HILL:"MI_Ground_Dirt",MOUNT:"MI_Ground_Rock",DRY:"MI_Ground_Tundra",MEADOW:"MI_Ground_Tundra",SNOW:"G_Snow",RIVER:"MI_VoxelWater"}
STRATA={MOUNT:"MI_Ground_DarkRock",HILL:"MI_Ground_Dirt",GRASS:"MI_Ground_Dirt",FOREST:"MI_Ground_Dirt",DRY:"MI_Ground_Dirt",SAND:"MI_Ground_Sand",MEADOW:"MI_Ground_Dirt",SNOW:"MI_Ground_Rock",RIVER:"MI_Ground_Dirt"}   # the exposed face below a terrace
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
            blk=cls[rs:re,cs:ce]; counts=np.bincount(blk.ravel(),minlength=10); k=int(counts.argmax())
            cc[j,i]=k; ch[j,i]=int(round(h[rs:re,cs:ce].mean())) if k!=SEA else -2
    # rivers: carved one terrace into whatever they cross (never below the coast), water on top
    rv=rivers(nx,ny,x0,y0)
    for j in range(ny):
        for i in range(nx):
            if rv[j,i] and cc[j,i]!=SEA: cc[j,i]=RIVER; ch[j,i]=max(-1,ch[j,i]-1)
    print("river cells",int(rv.sum()))
    # boxes: for each row, runs of equal (class, height) -> one top slab (STEP thick, the terrace's material) plus, where
    # a neighbour is lower, the exposed strata face as a darker slab below (down to the lowest neighbour)
    boxes=[]
    def world(i,j): return x0+(i+0.5)*CELL, y0+(j+0.5)*CELL
    for j in range(ny):
        i=0
        while i<nx:
            k=int(cc[j,i]); z=int(ch[j,i]); i2=i
            while i2+1<nx and cc[j,i2+1]==k and ch[j,i2+1]==z: i2+=1
            if k!=SEA:
                xa,_=world(i,j); xb,yb=world(i2,j)
                boxes.append((PALETTE[k],(xa+xb)/2,yb,z*STEP-STEP/2,(i2-i+1)*CELL,CELL,STEP))
                # strata under the terrace edge: the drop to the lowest of the four neighbours along this run
                lows=[]
                for ii in range(i,i2+1):
                    nb=[ch[jj,ii2] for jj,ii2 in ((j-1,ii),(j+1,ii),(j,ii-1),(j,ii+1)) if 0<=jj<ny and 0<=ii2<nx]
                    lows.append(min(nb) if nb else z)
                lo=min(lows)
                if lo<z-1 or (lo<z and k in (MOUNT,HILL)):
                    top=z*STEP-STEP; bot=max(lo,-2)*STEP
                    boxes.append((STRATA[k],(xa+xb)/2,yb,(top+bot)/2,(i2-i+1)*CELL,CELL,top-bot))
            i=i2+1
    # the sea: one slab under everything (its top at SEA_Z); the land sits on strata down to -2 so coasts read as cliffs
    boxes.append(("MI_VoxelWater",(x0+x1)/2,(y0+y1)/2,SEA_Z-50,x1-x0,y1-y0,100))
    # trees: dense on forest cells, sparse on grass, seeded; the kit's oaks, round bushes and pines
    rng=np.random.default_rng(7); trees=[]
    for j in range(ny):
        for i in range(nx):
            k=int(cc[j,i])
            p=0.55 if k==FOREST else (0.03 if k in (GRASS,MEADOW) else (0.06 if k==HILL else 0.0))
            n=int(p)+ (1 if rng.random()<(p-int(p)) else 0)
            for _ in range(n):
                x,y=world(i,j); x+=rng.uniform(-45,45); y+=rng.uniform(-45,45)
                kind="SM_Pine" if (k==HILL or rng.random()<0.25) else ("SM_Oak" if rng.random()<0.6 else "SM_BushRound")
                trees.append((round(x),round(y),int(ch[j,i])*STEP,kind,round(float(rng.uniform(0.35,0.55)),2)))
    pal={SEA:(70,120,200),SAND:(222,205,150),GRASS:(120,180,90),FOREST:(60,120,60),HILL:(170,150,110),MOUNT:(190,175,150),DRY:(175,165,95),MEADOW:(160,200,120),SNOW:(240,240,245),RIVER:(60,110,220)}
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
    print(f"cells {nx}x{ny}, boxes {len(boxes)}, trees {len(trees)}, classes {np.bincount(cc.ravel(),minlength=10).tolist()} -> {OUT} ({n} parts staged)")
if __name__=="__main__": main()
