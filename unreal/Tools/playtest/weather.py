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
# the voxel particles (AWotVoxelWeather): kind and intensity per town; UDW's own sprite particles are switched off
VOXEL={"EmondsField":("Leaves",0.6),"Baerlon":("Leaves",0.5),"Whitebridge":("Rain",0.5),"ShadarLogoth":("Ash",1.0),"FalDara":("Snow",1.0),
       "Caemlyn":None,"Cairhien":("Leaves",0.4),"Tear":("Rain",1.0),"TarValon":None,"Tanchico":("Sand",1.0),"Illian":("Rain",0.5),"Overworld":None,"Diorama":("Leaves",0.4)}
VOXEL_ACTOR="/Script/VoxelRPG.WotVoxelWeather"
WATER_ACTOR="/Script/VoxelRPG.WotVoxelWater"
# downstream hint (unit-ish vector) for the flow lines: the brook / river / canal direction in each town; towns without water skip it
WATER_FLOW={"EmondsField":(0.1,-1.0),"Baerlon":(1.0,0.0),"Whitebridge":(0.0,-1.0),"Caemlyn":(1.0,0.0),"Cairhien":(0.0,-1.0),"Tear":(1.0,0.2),"TarValon":(0.0,-1.0),"Illian":(1.0,0.0),"Tanchico":(1.0,0.0),"Diorama":(1.0,0.0)}
UDW_PARTICLES_OFF={"Enable Rain Particles":False,"Enable Snow Particles":False,"Enable Dust Particles":False,"Enable Wind Debris":False}   # the voxel weather draws these
# ground materials (voxel_ground_base: MI_VoxelGrass, MI_GroundStone, G_Snow) run UDW's Dynamic Landscape Weather Effects: snow / dust
# depth with footprints, puddles with ripples. PM_Ground on those materials tells the DLWE Interaction components (character feet) to react.
# the surface state UDW starts from: with "Material Effects Take Time to Change" the snow / wetness / dust on materials
# ramps from the saved value, so a snowed-in or rain-soaked town must start that way instead of bare ground for the first minutes
MAT_STATE={"FalDara":{"Material Snow Coverage":1.0},"Tear":{"Material Wetness":1.0},"Illian":{"Material Wetness":0.7},"Whitebridge":{"Material Wetness":0.7},
           "Tanchico":{"Material Dust Coverage":0.4},"ShadarLogoth":{"Material Dust Coverage":0.0}}
# the render target the footprints / ripples draw into follows UDW's control point: with "Player Pawn Location" it never
# recentres in our levels (the pawn spawns after UDW starts), with the camera it does
UDW_GROUND={"Enable Dynamic Landscape Weather Effects":True,"Enable WOV Material Effect Target":False,"Control Point Location Source":"Player Camera Location","Physical Materials which enable DLWE Interactions on non-Landscapes":[{"refPath":"/Game/Wot/Materials/PM_Ground.PM_Ground"}]}
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
            vals.update(extra); vals.update(UDW_PARTICLES_OFF); vals.update(UDW_GROUND); vals.update(MAT_STATE.get(n,{}))
            r=call(OB,"set_properties",{"instance":{"refPath":p},"values":json.dumps(vals)})
            got=call(OB,"get_properties",{"instance":{"refPath":p},"properties":["Weather","Wind Direction","Material Snow Coverage","Material Wetness","Material Dust Coverage"]})
            print(f"{n:13s} {p.split('.')[-1]} -> {preset}  {str(got)[:120]}  {'' if r is True else str(r)[-160:]}")
        # the voxel weather actor: one per level, configured for the town (removed where the town has none)
        vox=[a["refPath"] for a in (call(SC,"find_actors",{"name":"","tag":"","collision_channels":[]}) or []) if "WotVoxelWeather" in a["refPath"]]
        want=VOXEL.get(n)
        if want:
            kind,inten=want
            if not vox:
                a=call(SC,"add_to_scene_from_class",{"actor_type":{"refPath":VOXEL_ACTOR},"name":"VoxelWeather","xform":{"location":{"x":0,"y":0,"z":0},"rotation":{"pitch":0,"yaw":0,"roll":0},"scale":{"x":1,"y":1,"z":1}}})
                vox=[a["refPath"] if isinstance(a,dict) else a]
            for dup in vox[1:]: call(SC,"remove_from_scene",{"actor":{"refPath":dup}})
            import math
            wx,wy=math.cos(math.radians(wind))*(260 if kind=="Sand" else 120),math.sin(math.radians(wind))*(260 if kind=="Sand" else 120)
            r=call(OB,"set_properties",{"instance":{"refPath":vox[0]},"values":json.dumps({"Kind":kind,"Intensity":inten,"Material":None,"Wind":f"(X={wx:.0f},Y={wy:.0f},Z=0)"})})
            print(f"{'':13s} voxel {kind} x{inten} {'' if r is True else str(r)[-120:]}")
        else:
            for p in vox: call(SC,"remove_from_scene",{"actor":{"refPath":p}})
        # the voxel water actor: flow lines on every water voxel and ripples round anyone wading (it finds the water itself)
        wat=[a["refPath"] for a in (call(SC,"find_actors",{"name":"","tag":"","collision_channels":[]}) or []) if "WotVoxelWater" in a["refPath"]]
        flow=WATER_FLOW.get(n)
        if flow:
            if not wat:
                a=call(SC,"add_to_scene_from_class",{"actor_type":{"refPath":WATER_ACTOR},"name":"VoxelWater","xform":{"location":{"x":0,"y":0,"z":0},"rotation":{"pitch":0,"yaw":0,"roll":0},"scale":{"x":1,"y":1,"z":1}}})
                wat=[a["refPath"] if isinstance(a,dict) else a]
            for dup in wat[1:]: call(SC,"remove_from_scene",{"actor":{"refPath":dup}})
            r=call(OB,"set_properties",{"instance":{"refPath":wat[0]},"values":json.dumps({"Flow":f"(X={flow[0]:.2f},Y={flow[1]:.2f},Z=0)"})})
            print(f"{'':13s} water flow {flow} {'' if r is True else str(r)[-120:]}")
        else:
            for p in wat: call(SC,"remove_from_scene",{"actor":{"refPath":p}})
        call(AS,"save_assets",{"asset_paths":[lvl]})
if __name__=="__main__": main()
