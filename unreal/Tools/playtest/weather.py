#!/usr/bin/env python3
"""Places (or updates) an Ultra Dynamic Weather actor in every level with a per-town weather preset, and saves.
Rain, snow, dust / sand / ash particles spawn around the camera, cloud cover and fog follow the preset, and the
materials gather snow / dust and get wet through UDW's material functions (wot.Mat.AddWeather put them in the voxel
base material). Ash is the Sand_Dust preset with the dust particles recoloured near-black.
usage: weather.py [Map ...]   (default: every map in TABLE)"""
import json, os, subprocess, sys, time
HERE=os.path.dirname(os.path.abspath(__file__)); MCP=os.path.join(HERE,"..","towns","mcp.py")
UDW="/Game/UltraDynamicSky/Blueprints/Ultra_Dynamic_Weather"; PRE="/Game/UltraDynamicSky/Blueprints/Weather_Effects/Weather_Presets"
OURS="/Game/Wot/Weather"; CUSTOM={"Ash_Fall","Sand_Breeze"}   # our own presets (the UDS folder is a submodule): copies of Sand_Dust_Calm, lighter, ash without surface dust
MAPS={"Overworld":"/Game/Wot/Maps/Overworld/Overworld","Diorama":"/Game/Wot/Maps/Diorama/Diorama"}
for t in ("EmondsField","Baerlon","Whitebridge","ShadarLogoth","FalDara","Caemlyn","Cairhien","Tear","TarValon","Tanchico","Illian"): MAPS[t]=f"/Game/Wot/Maps/{t}/{t}"
ASH={"Dust Particle Color":{"r":0.06,"g":0.05,"b":0.05,"a":1.0}}
SAND={"Dust Particle Color":{"r":0.85,"g":0.72,"b":0.5,"a":1.0}}
# preset per town (the books' weather), extra UDW settings, wind direction (degrees)
TABLE={"EmondsField":("Partly_Cloudy",{},200),"Baerlon":("Cloudy",{},210),"Whitebridge":("Rain_Light",{},190),
       "ShadarLogoth":("Ash_Fall",ASH,160),"FalDara":("Snow",{},330),"Caemlyn":("Clear_Skies",{},180),"Cairhien":("Overcast",{},120),
       "Tear":("Rain",{},150),"TarValon":("Clear_Skies",{},200),"Tanchico":("Sand_Breeze",SAND,250),"Illian":("Rain_Light",{},170),
       "Overworld":("Partly_Cloudy",{"Use Random Weather Variation":True},200),"Diorama":("Partly_Cloudy",{},200)}
def call(ts,name,args):
    out=subprocess.run([sys.executable,MCP,"call","call_tool",json.dumps({"toolset_name":ts,"tool_name":name,"arguments":args})],capture_output=True,text=True,timeout=900).stdout
    try: j=json.loads(out[out.index("{"):out.rindex("}")+1])
    except ValueError: return out
    v=j.get("returnValue",j)
    if isinstance(v,str):
        try: return json.loads(v)
        except ValueError: return v
    return v
SC="editor_toolset.toolsets.scene.SceneTools"; OB="editor_toolset.toolsets.object.ObjectTools"; AS="editor_toolset.toolsets.asset.AssetTools"; AC="editor_toolset.toolsets.actor.ActorTools"
def main():
    names=sys.argv[1:] or list(TABLE)
    for n in names:
        lvl=MAPS[n]; preset,extra,wind=TABLE[n]
        if call(SC,"get_current_level",{})!=lvl: call(SC,"load_level",{"level_path":lvl}); time.sleep(4)
        # find_actors matches labels, so search the class name in the object path of every actor; one weather actor per level
        actors=[a["refPath"] for a in (call(SC,"find_actors",{"name":"","tag":"","collision_channels":[]}) or []) if "Ultra_Dynamic_Weather_C" in a["refPath"]]
        for dup in actors[1:]: call(SC,"remove_from_scene",{"actor":{"refPath":dup}})
        actors=actors[:1]
        if not actors:
            a=call(SC,"add_to_scene_from_asset",{"asset_path":UDW,"name":"Weather","xform":{"location":{"x":0,"y":0,"z":0},"rotation":{"pitch":0,"yaw":0,"roll":0},"scale":{"x":1,"y":1,"z":1}}})
            actors=[a["refPath"] if isinstance(a,dict) else a]
        for p in actors:
            folder=OURS if preset in CUSTOM else PRE
            vals={"Weather":{"refPath":f"{folder}/{preset}.{preset}"},"Wind Direction":wind,"Material Effects Take Time to Change":True}
            vals.update(extra)
            r=call(OB,"set_properties",{"instance":{"refPath":p},"values":json.dumps(vals)})
            got=call(OB,"get_properties",{"instance":{"refPath":p},"properties":["Weather","Wind Direction"]})
            print(f"{n:13s} {p.split('.')[-1]} -> {preset}  {str(got)[:120]}  {'' if r is True else str(r)[-160:]}")
        call(AS,"save_assets",{"asset_paths":[lvl]})
if __name__=="__main__": main()
