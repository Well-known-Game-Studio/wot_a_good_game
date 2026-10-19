#!/usr/bin/env python3
"""Sets Ultra Dynamic Sky's Sky Mode (and the 2D cloud-shadow settings) on every level that has a UDS actor, and saves.
Volumetric clouds cost a lot of GPU for a top-down game that never looks at the sky; "2D Dynamic Clouds" keeps the sky
sphere's panning cloud layer and drives the sun's light function (2D_Cloud_Shadows) so cloud shadows still drift over
the land and modulate the lighting.  usage: sky_mode.py ["2D Dynamic Clouds"|"Volumetric Clouds"|"Static Clouds"] [Map ...]"""
import json, os, subprocess, sys, time
HERE=os.path.dirname(os.path.abspath(__file__)); MCP=os.path.join(HERE,"..","towns","mcp.py")
MAPS={"Overworld":"/Game/Wot/Maps/Overworld/Overworld","Diorama":"/Game/Wot/Maps/Diorama/Diorama","GameplayTest":"/Game/Wot/Maps/GameplayTest/Gameplay_TestMap","HerdingTest":"/Game/Wot/Maps/HerdingTest/Herding_TestMap"}
for t in ("EmondsField","Baerlon","Whitebridge","ShadarLogoth","FalDara","Caemlyn","Cairhien","Tear","TarValon","Tanchico","Illian"): MAPS[t]=f"/Game/Wot/Maps/{t}/{t}"
MODE=sys.argv[1] if len(sys.argv)>1 else "2D Dynamic Clouds"
SHADOWS={"2D Cloud Shadows Scale":250000,"2D Cloud Shadows Speed":1.5,"Cloud Shadows Intensity when Sunny":0.6,"Cloud Shadows Softness when Sunny":0.35}
def call(ts,name,args):
    out=subprocess.run([sys.executable,MCP,"call","call_tool",json.dumps({"toolset_name":ts,"tool_name":name,"arguments":args})],capture_output=True,text=True,timeout=900).stdout
    try: j=json.loads(out[out.index("{"):out.rindex("}")+1])
    except ValueError: return out
    v=j.get("returnValue",j)
    if isinstance(v,str):
        try: return json.loads(v)
        except ValueError: return v
    return v
SC="editor_toolset.toolsets.scene.SceneTools"; OB="editor_toolset.toolsets.object.ObjectTools"; AS="editor_toolset.toolsets.asset.AssetTools"
def main():
    names=sys.argv[2:] or list(MAPS)
    for n in names:
        lvl=MAPS[n]
        if call(SC,"get_current_level",{})!=lvl: call(SC,"load_level",{"level_path":lvl}); time.sleep(4)
        actors=call(SC,"find_actors",{"name":"Ultra_Dynamic_Sky","tag":"","collision_channels":[]}) or []
        skies=[a["refPath"] for a in actors if "Ultra_Dynamic_Sky" in a["refPath"]]
        for p in skies:
            call(OB,"set_properties",{"instance":{"refPath":p},"values":json.dumps({"Sky Mode":MODE})})
            if MODE=="2D Dynamic Clouds": call(OB,"set_properties",{"instance":{"refPath":p},"values":json.dumps(SHADOWS)})
            got=call(OB,"get_properties",{"instance":{"refPath":p},"properties":["Sky Mode"]})
            print(f"{n:13s} {p.split('.')[-1]} -> {got.get('Sky Mode') if isinstance(got,dict) else got}")
        if not skies: print(f"{n:13s} no sky actor")
        call(AS,"save_assets",{"asset_paths":[lvl]})
if __name__=="__main__": main()
