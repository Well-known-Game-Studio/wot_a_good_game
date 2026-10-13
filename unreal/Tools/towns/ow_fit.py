#!/usr/bin/env python3
"""Fits the Randland reference map onto the Overworld landscape and writes ow_sites.json (town -> x, y, yaw in UE cm).

The Overworld landscape was sculpted from blender/overworld-full.png, a square crop of maps/westlands-national-borders-and-cities.png
(processed as python/processed_map.png). Two fits were measured once and are baked in below:
  reference px (2198x1703) -> heightmap px (761x762):  col = x/2.23 - 55,  row = y/2.23 - 2
  heightmap px -> UE:                                  X = -13300 + col*41.1,  Y = -19500 + row*41.1   (+X = east, +Y = south)
Each city is then snapped to the nearest plain-land pixel so portals never end up in the sea or on a mountain.
Needs Pillow + numpy (python/env has both): python/env/bin/python unreal/Tools/towns/ow_fit.py
"""
import json, math, os
from PIL import Image
import numpy as np
ROOT=os.path.abspath(os.path.join(os.path.dirname(__file__),"..","..",".."))
# city icon positions on the reference map, in pixels of a 2000x1550 view of maps/westlands-national-borders-and-cities.png
CITIES={"TarValon":(1500,582),"Caemlyn":(1375,916),"Cairhien":(1614,733),"Tear":(1524,1288),"Illian":(1230,1428),"Tanchico":(430,1026),
        "FalDara":(1763,320),"ShadarLogoth":(838,752),"Baerlon":(829,778),"EmondsField":(798,850),"Whitebridge":(1062,913)}
def ref_to_heightmap(dx,dy):
    ax,ay=dx*2198/2000.0,dy*1703/1550.0
    return ax/2.23-55, ay/2.23-2
def heightmap_to_ue(c,r): return -13300+c*41.1, -19500+r*41.1
def main():
    b=np.array(Image.open(os.path.join(ROOT,"blender","overworld-full.png")).convert("RGB")).astype(int)
    green=(abs(b[...,0]-155)<25)&(abs(b[...,1]-189)<25)&(abs(b[...,2]-130)<30)
    forest=(b[...,1]>b[...,0]+30)&(b[...,1]>b[...,2]+30)&(b[...,1]<170)
    good=green|forest
    out={}
    for n,(dx,dy) in CITIES.items():
        c,r=ref_to_heightmap(dx,dy); c=min(max(c,0),735); r=min(max(r,5),757)   # col<=735 keeps X inside the landscape
        best=None
        for dc in range(-70,71):
            for dr in range(-70,71):
                cc,rr=int(round(c))+dc,int(round(r))+dr
                if 4<=cc<=735 and 4<=rr<757 and good[rr-2:rr+3,cc-2:cc+3].all():
                    d=dc*dc+dr*dr
                    if best is None or d<best[0]: best=(d,cc,rr)
        _,cc,rr=best
        r0,r1,c0,c1=max(0,rr-15),min(762,rr+16),max(0,cc-15),min(761,cc+16)
        ys,xs=np.mgrid[r0:r1,c0:c1]; m=good[r0:r1,c0:c1]
        X,Y=heightmap_to_ue(cc,rr); yaw=math.degrees(math.atan2((ys[m]-rr).mean(),(xs[m]-cc).mean()))
        out[n]={"x":round(X),"y":round(Y),"yaw":round(yaw),"snap_px":round(math.sqrt(best[0]),1)}
    json.dump(out,open(os.path.join(os.path.dirname(__file__),"ow_sites.json"),"w"),indent=1)
    print(json.dumps(out,indent=1))
if __name__=="__main__": main()
