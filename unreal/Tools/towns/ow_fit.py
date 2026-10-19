#!/usr/bin/env python3
"""Fits the Overworld portals to the map texture the PCG landscape samples; writes ow_sites.json
(town -> x, y, yaw in UE cm, +X = east, +Y = south; "level" false = a city without a level yet: marker only).

Reference map -> texture. The Overworld texture blender/overworld-full.png (761 x 762) is a cut of
maps/newwotbase-1.png (8848 x 6832): texel (c, r) = base pixel (20 + 11.6 c, -60 + 9.05 r), found by matching the sea
and mountain masks of both images (anisotropic: the source was squashed to a square; 84% agreement). City positions are the icon centres read off the base map (ow_cities.json: a black star in a
white ring for cities, a black dot for towns), so a town lands where the map draws it.

Texture -> world. PCG_Landscape's Get Texture Data node maps the texture onto a surface centred on BP_LandscapeLayers
(-230, -360) with half-size = the actor's local bounds extent (its closed spline: 4484 x 4736), advanced tiling
(0.29, 0.30), centre offset (0.27, -0.16): UV = frac((local01 - 0.5 - offset) * tiling + 0.5), so texel (0, 0) is at
(-13271, -17662) and (761, 762) at (17653, 13911). Checked against the generated mountains (44% of them on mountain
texels; 21% under the old fixed fit).

Hand-placed settlements. ow_houses.json (ow_houses.py) holds the house / big-building foliage instances. A town whose
map position is within SNAP cm of one of those clusters is moved onto the cluster edge so the portal stands by the
houses; otherwise the map position is kept and the distance to the nearest cluster is reported.
Needs Pillow + numpy: python/env/bin/python unreal/Tools/towns/ow_fit.py
"""
import json, math, os
from PIL import Image
import numpy as np
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.abspath(os.path.join(HERE,"..","..",".."))
# base map -> texture: the texture is an anisotropic cut (the source was squashed to a square), fitted on the sea and
# mountain masks of both images (84% agreement; an isotropic fit only reaches 70%)
BX,BY,SX,SY=20.0,-60.0,11.6,9.05
# texture -> world (the PCG texture surface)
AX,AY=-230.0,-360.0; EX,EY=4484.0,4736.0; TX,TY=0.29,0.30; CX,CY=0.27,-0.16; W,H=761,762
SNAP=1500.0; CLUSTER=900.0
def base_to_px(bx,by): return (bx-BX)/SX,(by-BY)/SY
def px_to_world(c,r):
    u,v=c/W,r/H; lx=0.5+CX+(u-0.5)/TX; ly=0.5+CY+(v-0.5)/TY
    return (AX-EX)+lx*2*EX,(AY-EY)+ly*2*EY
def world_to_px(x,y):
    lx=(x-(AX-EX))/(2*EX); ly=(y-(AY-EY))/(2*EY)
    return ((lx-0.5-CX)*TX+0.5)*W,((ly-0.5-CY)*TY+0.5)*H
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
    sea=(abs(img[...,0]-121)<20)&(abs(img[...,1]-167)<20)&(abs(img[...,2]-227)<20); land=~sea
    cities=json.load(open(os.path.join(HERE,"ow_cities.json")))
    hp=os.path.join(HERE,"ow_houses.json"); cl=clusters(json.load(open(hp))) if os.path.exists(hp) else []
    out={}; used=set()
    # a cluster serves one town: hand the clusters out in order of how close a town's map spot is to them
    def near_dist(info):
        c,r=base_to_px(*info["px"]); X,Y=px_to_world(min(max(c,20.0),W-21.0),min(max(r,20.0),H-21.0))
        return min((math.hypot(q[0]-X,q[1]-Y) for q in cl),default=1e9)
    for n,info in sorted(cities.items(),key=lambda kv: near_dist(kv[1])):
        c,r=base_to_px(*info["px"]); snap=SNAP; src="map"; k=None
        if not (0<=c<W and 0<=r<H):
            if not info["level"]: print(f"{n:13s} off the texture (texel {c:.0f},{r:.0f}): skipped"); continue
            # a town with a level that the map puts beyond the landscape (Fal Dara, east of the cut): the edge, and
            # the nearest hand-placed settlement if there is one within reach
            c=min(max(c,20.0),W-21.0); r=min(max(r,20.0),H-21.0); snap=3600.0; src="map edge"
        X,Y=px_to_world(c,r)
        if cl:
            k=min(cl,key=lambda q:math.hypot(q[0]-X,q[1]-Y)); d=math.hypot(k[0]-X,k[1]-Y)
            if d<=snap and id(k) not in used: used.add(id(k)); src+=f", houses {d:.0f} cm away"
            else: src+=f" (nearest houses {d:.0f} cm)"; k=None
        # yaw: toward the most land around the spot (open ground for the portal and its player start)
        ci,ri=min(max(int(round(c)),0),W-1),min(max(int(round(r)),0),H-1); r0,r1,c0,c1=max(0,ri-15),min(H,ri+16),max(0,ci-15),min(W,ci+16)
        ys,xs=np.mgrid[r0:r1,c0:c1]; m=land[r0:r1,c0:c1]
        gy,gx=((ys[m]-ri).mean(),(xs[m]-ci).mean()) if m.any() else (0.0,0.0)
        yaw=math.degrees(math.atan2(gy,gx)) if abs(gx)+abs(gy)>0.3 else 0.0
        if k: X,Y=k[0]+math.cos(math.radians(yaw))*450,k[1]+math.sin(math.radians(yaw))*450   # beside the houses
        out[n]={"x":round(X),"y":round(Y),"yaw":round(yaw),"level":bool(info["level"]),"source":src,"cluster":[round(k[0]),round(k[1])] if k else None}
    json.dump(out,open(os.path.join(HERE,"ow_sites.json"),"w"),indent=1)
    for n,s in out.items(): print(f"{n:13s} ({s['x']:7d},{s['y']:7d}) yaw {s['yaw']:5d} {'level' if s['level'] else 'marker'}  {s['source']}")
if __name__=="__main__": main()
