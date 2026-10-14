#!/usr/bin/env python3
"""Editor-world navmesh rebuild + probes along the first bridge of the Ways, then save the level."""
import json, os, sys, time
sys.argv = ['x', 'Ways']
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
ECMD = os.path.join(PROJ, "Saved", "PlayTest", "commands_editor.txt")
level = "/Game/Wot/Maps/Ways/Ways"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
before = len(pt_lines())
# island 0 at (0,0) r=1400, island 1 at (-2861,920): probe island, rim, bridge, far rim, far island
pts = [(-600,190),(-1200,385),(-1400,450),(-1600,515),(-1800,580),(-2100,675),(-2400,770),(-2861,920),
       (-4637,3743),(-8348,3521),(-10925,5567),(-13442,8153),(-16467,8766),(-18997,5942),(-20257,2877)]
with open(ECMD, "w") as f:
    f.write("wot.Nav.Rebuild\n")
time.sleep(25)
with open(ECMD, "w") as f:
    f.write("wot.Nav.Probe " + " ".join(f"{x} {y}" for x, y in pts) + "\n")
lines = wait_for("PROBE (-20257", 60, before)
for l in lines[before:]:
    if "PROBE" in l or "NAV" in l: print(l[l.find('[PT]'):][:140])
print(call("editor_toolset.toolsets.asset.AssetTools.save_assets", {"asset_paths": [level]}))
