#!/usr/bin/env python3
"""Regenerate a town through the editor's MCP server.

Prereq: editor running with the MCP server, e.g.
  UnrealEditor unreal/VoxelRPG.uproject -ModelContextProtocolStartServer -ModelContextProtocolPort=8000
Usage:
  python3 run_town.py <Town>        Town = EmondsField | Caemlyn | Cairhien | Tear | TarValon | FalDara | Tanchico | Illian | ShadarLogoth | Whitebridge | Baerlon
  python3 run_town.py --kit         rebuild the merged kit meshes (SM_*) from kit_builder.py
  python3 run_town.py --overworld   (re)place Overworld portals/player starts/landmarks for all towns
"""
import json, subprocess, sys, os
HERE=os.path.dirname(os.path.abspath(__file__))
def rd(f): return open(os.path.join(HERE,f)).read()
def call_script(script):
    args=json.dumps({"toolset_name":"editor_toolset.toolsets.programmatic.ProgrammaticToolset","tool_name":"execute_tool_script","arguments":{"script":script}})
    r=subprocess.run(["python3",os.path.join(HERE,"mcp.py"),"call","call_tool",args],capture_output=True,text=True)
    print(r.stdout[:3000]); print(r.stderr[-800:])
def main():
    arg=sys.argv[1]
    base="\n".join(rd(f) for f in ("pcg_lib.py","town_builder.py","kit_builder.py","make_city.py","city_specs.py","city_terrain.py","ef_spec.py"))
    if arg=="--kit":
        call_script(base+"\n"+rd("run_kit.py")); return
    if arg=="--overworld":
        call_script(base+"\n"+rd("ow_portals.py")); return
    call_script(base+"\nTOWNS=["+json.dumps(arg)+"]\n"+rd("run_cities.py"))
if __name__=="__main__": main()
