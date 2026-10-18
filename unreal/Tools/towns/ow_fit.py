#!/usr/bin/env python3
"""Fits the Overworld portals to the map texture the PCG landscape samples and to the hand-placed settlements; writes
ow_sites.json (town -> x, y, yaw in UE cm, +X = east, +Y = south).

World <-> texture. PCG_Landscape's Get Texture Data node maps blender/overworld-full.png (761 x 762, the same image the
landscape was sculpted from) onto a surface centred on BP_LandscapeLayers (-230, -360) with half-size = the actor's local
bounds extent (its closed spline: 4484 x 4736), using advanced tiling (0.29, 0.30) and centre offset (0.27, -0.16):
UV = frac((local01 - 0.5 - offset) * tiling + 0.5). That puts texel (0, 0) at (-13271, -17662) and (761, 762) at
(17653, 13911): 40.6 x 41.4 cm per texel, i.e. the texture covers the landscape (-13410..17600, -17420..13550) almost
exactly. Checked against the generated mountains: 44% of the mountain instances sit on mountain texels under this
mapping (21% under the old fixed fit, whose Y origin was 2000 cm too far north and whose reference-map crop was off).

Reference map. City icons on maps/westlands-national-borders-and-cities.png (2000 x 1550 view). The fixed crop from
that map to the texture was wrong in both axes, so the reference pixels are now mapped to the world by an affine fit
through ANCHORS: towns whose hand-placed settlement cluster in the Overworld (ow_houses.json, written by ow_houses.py from
the foliage house / big-building instances) is unambiguous by geography. Each town then snaps to the nearest cluster
within SNAP cm of its predicted spot; a town with no cluster nearby goes to the nearest plain-land texel instead.
Needs Pillow + numpy: python/env/bin/python unreal/Tools/towns/ow_fit.py
"""
import json, math, os
from PIL import Image
import numpy as np
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.abspath(os.path.join(HERE,"..","..",".."))
CITIES={"TarValon":(1500,582),"Caemlyn":(1375,916),"Cairhien":(1614,733),"Tear":(1524,1288),"Illian":(1230,1428),"Tanchico":(430,1026),
        "FalDara":(1763,320),"ShadarLogoth":(838,752),"Baerlon":(829,778),"EmondsField":(798,850),"Whitebridge":(1062,913)}
# the settlement cluster each anchor town stands at (approximate world position; resolved to the nearest cluster)
ANCHORS={"TarValon":(9770,-5439),"Caemlyn":(7859,878),"Whitebridge":(3203,702),"Baerlon":(-815,-1096),"EmondsField":(-941,277),
         "Tanchico":(-6536,3326),"Tear":(15666,11260),"FalDara":(13492,-10651),"Illian":(8132,9053)}
SNAP=3500.0; CLUSTER=900.0
# the PCG texture surface (see the docstring)
AX,AY=-230.0,-360.0; EX,EY=4484.0,4736.0; TX,TY=0.29,0.30; CX,CY=0.27,-0.16; W,H=761,762
def uv_to_world(u,v):
    lx=0.5+CX+(u-0.5)/TX; ly=0.5+CY+(v-0.5)/TY
    return (AX-EX)+lx*2*EX, (AY-EY)+ly*2*EY
def world_to_px(x,y):
    lx=(x-(AX-EX))/(2*EX); ly=(y-(AY-EY))/(2*EY)
    return ((lx-0.5-CX)*TX+0.5)*W, ((ly-0.5-CY)*TY+0.5)*H
def px_to_world(c,r): return uv_to_world(c/W,r/H)
def clusters(houses):
    cl=[]
    for h in houses:
        p=(h["x"],h["y"])
        for c in cl:
            if any(math.hypot(p[0]-q[0],p[1]-q[1])<CLUSTER for q in c): c.append(p); break
        else: cl.append([p])
    return [(sum(x for x,_ in c)/len(c),sum(y for _,y in c)/len(c),len(c)) for c in cl]
def main():
    img=np.array(Image.open(os.path.join(ROOT,"blender","overworld-full.png")).convert("RGB")).astype(int)
    green=(abs(img[...,0]-155)<25)&(abs(img[...,1]-189)<25)&(abs(img[...,2]-130)<30)
    forest=(img[...,1]>img[...,0]+30)&(img[...,1]>img[...,2]+30)&(img[...,1]<170)
    hills=(abs(img[...,0]-184)<20)&(abs(img[...,1]-184)<20)&(abs(img[...,2]-121)<25)
    sea=(abs(img[...,0]-121)<20)&(abs(img[...,1]-167)<20)&(abs(img[...,2]-227)<20)
    good=green|forest|hills          # where a portal may stand
    land=~sea                        # what the portal faces (yaw = toward the most land)
    hp=os.path.join(HERE,"ow_houses.json"); cl=clusters(json.load(open(hp))) if os.path.exists(hp) else []
    nearest=lambda x,y: min(cl,key=lambda k:math.hypot(k[0]-x,k[1]-y)) if cl else None
    # affine reference px -> world through the anchors
    A=[];B=[]
    for n,(ax,ay) in ANCHORS.items():
        k=nearest(ax,ay); dx,dy=CITIES[n]; A.append([dx,dy,1.0]); B.append([k[0],k[1]])
    M=np.linalg.lstsq(np.array(A),np.array(B),rcond=None)[0]
    res=np.array(B)-np.array(A)@M; print("affine residuals (cm):",[round(float(np.hypot(*r))) for r in res])
    out={}; used=[nearest(*a) for a in ANCHORS.values()]   # a cluster serves one town
    order=sorted(CITIES,key=lambda n: 0 if n in ANCHORS else 1)   # houses first, so the rest can keep clear of them
    for n in order:
        dx,dy=CITIES[n]
        X,Y=(np.array([dx,dy,1.0])@M).tolist(); src="affine"
        if n in ANCHORS: k=nearest(*ANCHORS[n])
        else:
            free=[c for c in cl if c not in used]
            k=min(free,key=lambda c:math.hypot(c[0]-X,c[1]-Y)) if free else None
            if k and math.hypot(k[0]-X,k[1]-Y)<=SNAP: used.append(k)
            else: k=None
        if k: X,Y=k[0],k[1]; src=f"houses({k[2]})"
        else:
            k=None
            c,r=world_to_px(X,Y); c=min(max(int(round(c)),4),W-5); r=min(max(int(round(r)),4),H-5); best=None
            for dc in range(-70,71):
                for dr in range(-70,71):
                    cc,rr=c+dc,r+dr
                    if 4<=cc<W-4 and 4<=rr<H-4 and good[rr-2:rr+3,cc-2:cc+3].all():
                        d=dc*dc+dr*dr
                        if best is None or d<best[0]: best=(d,cc,rr)
            if best: X,Y=px_to_world(best[1],best[2]); src=f"land(snap {math.sqrt(best[0]):.0f}px)"
        # a town without houses must not sit in another town's lap (Shadar Logoth's icon is 26 px from Baerlon's)
        if not k:
            for o in out.values():
                d=math.hypot(X-o["x"],Y-o["y"])
                if d<2600:
                    ux,uy=((X-o["x"])/d,(Y-o["y"])/d) if d>1 else (0.7,-0.7)
                    X,Y=o["x"]+ux*2800,o["y"]+uy*2800; src+="+spaced"
        # yaw: toward the open land around the spot (the portal and its player start sit on that side of the houses)
        c,r=world_to_px(X,Y); c=int(round(c)); r=int(round(r))
        r0,r1,c0,c1=max(0,r-15),min(H,r+16),max(0,c-15),min(W,c+16)
        ys,xs=np.mgrid[r0:r1,c0:c1]; m=land[r0:r1,c0:c1]
        yaw=math.degrees(math.atan2((ys[m]-r).mean(),(xs[m]-c).mean())) if m.any() and (abs((ys[m]-r).mean())+abs((xs[m]-c).mean()))>0.3 else 0.0
        if k:   # beside the houses, not on top of them
            X+=math.cos(math.radians(yaw))*450; Y+=math.sin(math.radians(yaw))*450
        out[n]={"x":round(X),"y":round(Y),"yaw":round(yaw),"source":src,"cluster":[round(k[0]),round(k[1])] if k else None}
    json.dump(out,open(os.path.join(HERE,"ow_sites.json"),"w"),indent=1)
    for n,s in out.items(): print(f"{n:13s} ({s['x']:7d},{s['y']:7d}) yaw {s['yaw']:5d}  {s['source']}")
if __name__=="__main__": main()
