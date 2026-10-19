#!/usr/bin/env python3
"""Dumps the hand-placed Overworld settlement meshes (foliage instances of small_prop_house / small_prop_big_building) to
ow_houses.json as world positions, for ow_fit.py to snap the portals to. Needs the editor MCP server; loads the Overworld."""
import json, os, subprocess, sys
HERE=os.path.dirname(os.path.abspath(__file__)); MCP=os.path.join(HERE,"mcp.py")
LVL="/Game/Wot/Maps/Overworld/Overworld"; L=LVL+".Overworld:PersistentLevel"
def call(ts,name,args):
    out=subprocess.run([sys.executable,MCP,"call","call_tool",json.dumps({"toolset_name":ts,"tool_name":name,"arguments":args})],capture_output=True,text=True,timeout=600).stdout
    j=json.loads(out[out.index("{"):out.rindex("}")+1]); v=j.get("returnValue",j)
    if isinstance(v,str):
        try: return json.loads(v)
        except ValueError: return v   # a plain string (a level path)
    return v
def main():
    if call("editor_toolset.toolsets.scene.SceneTools","get_current_level",{})!=LVL:
        call("editor_toolset.toolsets.scene.SceneTools","load_level",{"level_path":LVL})
    comps=call("editor_toolset.toolsets.actor.ActorTools","get_components",{"actor":{"refPath":L+".InstancedFoliageActor_0"}}) or []
    root=call("editor_toolset.toolsets.object.ObjectTools","get_properties",{"instance":{"refPath":L+".InstancedFoliageActor_0.RootComponent0"},"properties":["RelativeLocation"]}).get("RelativeLocation",{"x":0,"y":0})
    houses=[]
    for c in comps:
        p=c["refPath"]
        if "FoliageInstancedStaticMeshComponent" not in p: continue
        v=call("editor_toolset.toolsets.object.ObjectTools","get_properties",{"instance":{"refPath":p},"properties":["StaticMesh","PerInstanceSMData"]})
        mesh=(v.get("StaticMesh") or {}).get("refPath","").split("/")[-1].split(".")[0]
        if not ("house" in mesh or "building" in mesh): continue
        for i in v.get("PerInstanceSMData") or []:
            w=i["transform"]["wPlane"]; houses.append({"x":round(w["x"]+root["x"]),"y":round(w["y"]+root["y"]),"mesh":mesh})
    json.dump(houses,open(os.path.join(HERE,"ow_houses.json"),"w"))
    print(len(houses),"settlement meshes ->",os.path.join(HERE,"ow_houses.json"))
if __name__=="__main__": main()
