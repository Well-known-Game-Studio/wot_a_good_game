#!/usr/bin/env python3
"""Regenerate a town through the editor's MCP server.

Prereq: editor running with the MCP server, e.g.
  UnrealEditor unreal/VoxelRPG.uproject -ModelContextProtocolStartServer -ModelContextProtocolPort=8000
Usage:
  python3 run_town.py <Town> [<Town> ...]   Town = EmondsField | Caemlyn | Cairhien | Tear | TarValon | FalDara | Tanchico | Illian | ShadarLogoth | Whitebridge | Baerlon
  python3 run_town.py --all                 every town, sequentially
  python3 run_town.py --dry <Town>          build the spec and relief locally (no editor) and print terrain/road statistics
  python3 run_town.py --kit                 rebuild the merged kit meshes (SM_*) from kit_builder.py
  python3 run_town.py --overworld           (re)place Overworld portals/player starts/landmarks from ow_sites.json (see ow_fit.py)
"""
import json, subprocess, sys, os, math, time
HERE=os.path.dirname(os.path.abspath(__file__))
TOWNS=["EmondsField","Baerlon","Whitebridge","ShadarLogoth","FalDara","Caemlyn","Cairhien","Tear","TarValon","Tanchico","Illian"]
FILES=("pcg_lib.py","town_builder.py","kit_builder.py","relief.py","specs_common.py","specs_a.py","specs_b.py")
def rd(f): return open(os.path.join(HERE,f)).read()
def base(): return "import time\n"+"\n".join(rd(f) for f in FILES)
def call_script(script):
    args=json.dumps({"toolset_name":"editor_toolset.toolsets.programmatic.ProgrammaticToolset","tool_name":"execute_tool_script","arguments":{"script":script}})
    r=subprocess.run(["python3",os.path.join(HERE,"mcp.py"),"call","call_tool",args],capture_output=True,text=True)
    print(r.stdout[:3000]); print(r.stderr[-800:])
def dry(towns):
    g={"execute_tool":lambda n,a:(_ for _ in ()).throw(RuntimeError("no editor in dry run")),"math":math,"json":json,"time":time}
    exec(base(),g)
    for town in towns:
        s=g["CITY_SPECS"][town](); R=s["_relief"]; stairs=[p for p in s["props"] if "Stairs_" in p[2]]
        print(f"{town:13s} cells {R.n*R.n:6d} boxes {len(s['terrain']['boxes']):5d} lattice {len(s['lattice']):5d} stairs {len(stairs):3d} roads {len(s['roads']):3d} props {len(s['props'])}")
def main():
    args=sys.argv[1:]
    if not args: print(__doc__); return
    if args[0]=="--kit": call_script(base()+"\n"+rd("run_kit.py")); return
    if args[0]=="--overworld":
        sites=json.load(open(os.path.join(HERE,"ow_sites.json")))
        call_script(base()+"\n"+rd("ow_portals.py").replace("__SITES__",json.dumps(sites))); return
    if args[0]=="--dry": dry(args[1:] or TOWNS); return
    towns=TOWNS if args[0]=="--all" else args
    for town in towns:
        print("===",town); call_script(base()+"\nTOWNS=["+json.dumps(town)+"]\n"+rd("run_cities.py"))
if __name__=="__main__": main()
