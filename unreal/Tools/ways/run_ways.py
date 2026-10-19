#!/usr/bin/env python3
"""Build the Ways through the editor MCP server.
  python3 run_ways.py mats     create the Ways palette materials
  python3 run_ways.py kit      build the Ways kit meshes (islands, bridges, Guiding, Waygate, pillars, rubble)
  python3 run_ways.py level    generate /Game/Wot/Maps/Ways/Ways (seed/count optional: level 11 9)
  python3 run_ways.py all"""
import json, os, subprocess, sys, time
HERE = os.path.dirname(os.path.abspath(__file__)); TOWNS = os.path.join(os.path.dirname(HERE), "towns")
FILES = ("pcg_lib.py", "town_builder.py", "kit_builder.py", "relief.py", "specs_common.py")
def rd(f, base=TOWNS): return open(os.path.join(base, f)).read()
def base(): return "import time\n" + "\n".join(rd(f) for f in FILES) + "\n" + rd("ways_builder.py", HERE)
def call_script(script):
    args = json.dumps({"toolset_name": "editor_toolset.toolsets.programmatic.ProgrammaticToolset", "tool_name": "execute_tool_script", "arguments": {"script": script}})
    t = time.time()
    r = subprocess.run(["python3", os.path.join(TOWNS, "mcp.py"), "call", "call_tool", args], capture_output=True, text=True)
    print(f"{time.time()-t:.0f}s", r.stdout[:1500], r.stderr[-300:])
STAGES = {
    "mats": "def run():\n    return {'made': way_materials()}\n",
    "kit": "def run():\n    out=[]\n    names=('SM_WayIsland','SM_WayIsland2','SM_WayIsland3','SM_WayBridge','SM_WayBridge2','SM_Guiding','SM_Waygate','SM_WayPillar','SM_WayPillar2','SM_WayRubble')\n    for n in names:\n        out.append(build_kit_mesh(n,list(KIT[n]())))\n    SM='editor_toolset.toolsets.static_mesh.StaticMeshTools.'\n    for n in names:\n        if n=='SM_Waygate': continue\n        # islands and bridge spans get a single hull so the walking surface is one flat plane for the navmesh\n        hulls=1 if n.startswith(('SM_WayIsland','SM_WayBridge')) else 8\n        try: T(SM+'generate_convex_collisions',{'mesh':ref(KIT_FOLDER+'/'+n+'.'+n),'hull_count':hulls,'max_hull_verts':24,'hull_precision':100000})\n        except Exception as e: out.append(str(e)[:80])\n    save([KIT_FOLDER+'/'+n for n in names])\n    return {'built': [str(o)[:60] for o in out]}\n",
}
def main():
    stage = sys.argv[1] if len(sys.argv) > 1 else "all"
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 11
    count = int(sys.argv[3]) if len(sys.argv) > 3 else 9
    STAGES["level"] = f"def run():\n    return build_ways({seed},{count})\n"
    for st in (["mats", "kit", "level"] if stage == "all" else [stage]):
        print("===", st); call_script(base() + "\n" + STAGES[st])
if __name__ == "__main__":
    main()
