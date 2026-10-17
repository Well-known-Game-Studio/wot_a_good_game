#!/usr/bin/env python3
"""Renders a PCG graph (town graph or PCGS_* subgraph) to a PNG from the editor's GetGraphStructure, through the MCP server.
usage: pcg_graph_render.py <graph asset path> <out.png>   e.g. /Game/Wot/Towns/Common/PCG/PCGS_Scatter docs/images/pcg_scatter.png
Nodes are drawn at their editor positions (title, type, overrides summary), edges as curves, subgraph nodes highlighted."""
import json, os, subprocess, sys, textwrap
from PIL import Image, ImageDraw, ImageFont
HERE=os.path.dirname(os.path.abspath(__file__))
def call(tool,args):
    ts,name=tool.rsplit(".",1)
    out=subprocess.run([sys.executable,os.path.join(HERE,"mcp.py"),"call","call_tool",json.dumps({"toolset_name":ts,"tool_name":name,"arguments":args})],capture_output=True,text=True,timeout=900).stdout
    j=json.loads(out[out.index("{"):out.rindex("}")+1]); v=j.get("returnValue",j)
    if isinstance(v,str):
        try: v=json.loads(v)
        except Exception: pass
    return v
COLORS={"Create Points":(70,110,160),"Static Mesh Spawner":(150,80,80),"Merge Points":(90,90,110),"Input Node":(60,130,90),"Output Node":(60,130,90),
        "Get Graph Parameter":(130,110,50),"Difference":(120,70,120),"Bounds Modifier":(100,100,70),"Transform Points":(80,120,120),"Self Pruning":(120,90,60),
        "Random Choice":(110,80,130),"Density Filter":(80,110,80),"Create Spline":(70,120,140),"Create Surface From Spline":(70,120,140),"Spline Sampler":(70,120,140),"Copy Points":(90,100,130),"Distance":(100,120,90),"To Point":(90,90,90)}
def render(graph,out,scale=0.62):
    name=graph.split("/")[-1]; st=call("PCGToolset.PCGToolset.GetGraphStructure",{"graph":{"refPath":f"{graph}.{name}"}})
    desc=st.get("description") or ""; nodes=st["nodes"]; edges=st["edges"]
    pos={}
    for n in nodes:
        x,y=[float(v) for v in n["position"].strip("()").split(",")]; pos[n["name"]]=(x,y)
    W_NODE,H_NODE=300,78
    xs=[p[0] for p in pos.values()]; ys=[p[1] for p in pos.values()]
    x0,y0=min(xs)-40,min(ys)-40; w=int((max(xs)-x0+W_NODE+60)*scale); h=int((max(ys)-y0+H_NODE+60)*scale)+70
    im=Image.new("RGB",(max(w,900),h),(38,38,42)); d=ImageDraw.Draw(im)
    try: F=ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf",int(15*scale/0.62)); FS=ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf",int(12*scale/0.62)); FT=ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf",18)
    except Exception: F=FS=FT=ImageFont.load_default()
    P=lambda x,y:((x-x0)*scale,(y-y0)*scale+60)
    byname={n["name"]:n for n in nodes}
    for e in edges:
        if e["srcNode"] not in pos or e["destNode"] not in pos: continue
        ax,ay=P(pos[e["srcNode"]][0]+W_NODE,pos[e["srcNode"]][1]+H_NODE/2); bx,by=P(pos[e["destNode"]][0],pos[e["destNode"]][1]+H_NODE/2)
        pts=[]; n_=24
        for k in range(n_+1):
            t=k/n_; mx=ax+(bx-ax)*t; cy=ay+(by-ay)*(3*t*t-2*t*t*t); pts.append((mx,cy))
        d.line(pts,fill=(180,180,190) if e["destPin"]!="Overrides" else (200,170,90),width=2)
    for n in nodes:
        x,y=P(*pos[n["name"]]); ww,hh=W_NODE*scale,H_NODE*scale
        typ=n["nodeType"]; col=COLORS.get(typ,(110,80,60) if typ.startswith("Subgraph") else (85,85,95))
        d.rounded_rectangle([x,y,x+ww,y+hh],radius=6,fill=col,outline=(220,220,220) if typ.startswith("Subgraph") else (20,20,20),width=2 if typ.startswith("Subgraph") else 1)
        title=(n.get("title") or n["name"])[:34]; d.text((x+6,y+4),title,fill=(255,255,255),font=F)
        sub=typ if typ!="Subgraph" else typ
        po=n.get("paramOverrides") or {}
        if typ.startswith("Subgraph") and isinstance(po,dict): sub=typ+"  "+", ".join(f"{k}={v if not isinstance(v,dict) else '…'}" for k,v in list(po.items())[:3])
        elif typ=="Get Graph Parameter": sub=f"param {po.get('propertyPath','')} → {po.get('outputAttributeName','')}"
        d.text((x+6,y+hh-int(16*scale/0.62)-4),sub[:48],fill=(225,225,225),font=FS)
    d.text((10,8),name,fill=(255,255,255),font=FT)
    for i,line in enumerate(textwrap.wrap(desc,160)[:2]): d.text((10,30+i*16),line,fill=(200,200,200),font=FS)
    os.makedirs(os.path.dirname(os.path.abspath(out)),exist_ok=True); im.save(out); print(out,im.size,len(nodes),"nodes")
if __name__=="__main__": render(sys.argv[1],sys.argv[2],float(sys.argv[3]) if len(sys.argv)>3 else 0.62)
