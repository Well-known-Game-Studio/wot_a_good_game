#!/usr/bin/env python3
"""Offline render of a plaza's placement: roads, street mouths, registered footprints, house slots, street-line props and the
furniture the rules placed. usage: plaza_debug.py <Town> [plaza index ...]   -> <Town>_plaza<i>.png next to this script"""
import math, json, time, sys, os
from PIL import Image, ImageDraw
HERE=os.path.dirname(os.path.abspath(__file__))
town=sys.argv[1]; idxs=[int(a) for a in sys.argv[2:]]
sys.argv=['x']
exec(open(os.path.join(HERE,'run_town.py')).read().split("def main():")[0])
g={"execute_tool":lambda n,a:(_ for _ in ()).throw(RuntimeError("no editor")),"math":math,"json":json,"time":time}
exec(base(),g)
s=g["CITY_SPECS"][town](); R=s["_relief"]
for pi in (idxs or range(len(s["plazas"]))):
    pz=s["plazas"][pi]; poly=[(p[0],p[1]) for p in pz["pts"]]
    xs=[p[0] for p in poly]; ys=[p[1] for p in poly]; pad=900
    x0,x1,y0,y1=min(xs)-pad,max(xs)+pad,min(ys)-pad,max(ys)+pad; sc=900.0/max(x1-x0,y1-y0)
    W,H=int((x1-x0)*sc)+1,int((y1-y0)*sc)+1; im=Image.new("RGB",(W,H),(235,235,225)); d=ImageDraw.Draw(im)
    T=lambda x,y:((x-x0)*sc,(y-y0)*sc)
    d.polygon([T(*p) for p in poly],outline=(0,0,0),fill=(215,215,200))
    segs,mouths=R._plaza_roads(s,poly)
    for (a,b,hw) in segs: d.line([T(*a),T(*b)],fill=(120,120,120),width=max(1,int(2*hw*sc)))
    for (mx,my,hw) in mouths: d.ellipse([T(mx-100,my-100),T(mx+100,my+100)],fill=(255,0,0))
    for (ox,oy,orad,k) in getattr(R,"_occ",()):
        col={"prop":(0,90,200),"stall":(200,120,0),"lamp":(230,200,0),"other":(0,150,0),"monument":(150,0,150)}.get(k,(0,0,0))
        d.ellipse([T(ox-orad,oy-orad),T(ox+orad,oy+orad)],outline=col,width=2)
    for (px,py) in getattr(R,"_slot_pts",()):
        if x0<px<x1 and y0<py<y1: d.rectangle([T(px-60,py-60),T(px+60,py+60)],fill=(0,0,0))
    for (px,py) in getattr(R,"_line_pts",()):
        if x0<px<x1 and y0<py<y1: d.ellipse([T(px-40,py-40),T(px+40,py+40)],fill=(0,200,200))
    for (kx,ky,kr) in getattr(R,"_keep",()):
        if x0<kx<x1 and y0<ky<y1: d.ellipse([T(kx-kr,ky-kr),T(kx+kr,ky+kr)],outline=(255,0,255),width=1)
    for f in s["furniture_sets"]:
        if not f["name"].startswith(f"plaza {pi} "): continue
        for (x,y,z,yaw) in f["pts"]:
            d.ellipse([T(x-60,y-60),T(x+60,y+60)],fill=(255,120,0))
            d.line([T(x,y),T(x+200*math.cos(math.radians(yaw+180)),y+200*math.sin(math.radians(yaw+180)))],fill=(255,0,0),width=2)
    d.text((5,5),f"{town} plaza {pi}: grey roads, red mouths, blue props, black house slots, cyan line props, magenta keepouts, orange furniture",fill=(0,0,0))
    out=os.path.join(HERE,f"{town}_plaza{pi}.png"); im.save(out); print(out)
