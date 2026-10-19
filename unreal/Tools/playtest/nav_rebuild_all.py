#!/usr/bin/env python3
"""Rebuild and save the editor-world navmesh of every town (run after any town rebuild: the dynamic runtime navmesh
starts from the saved tiles and does not regenerate stale ones). usage: nav_rebuild_all.py [Town ...]"""
import os, sys, time
sys.argv = ['x'] + sys.argv[1:]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
TOWNS = sys.argv[1:] or ["EmondsField", "Baerlon", "Whitebridge", "ShadarLogoth", "FalDara", "Caemlyn", "Cairhien", "Tear", "TarValon", "Tanchico", "Illian", "Ways/Ways"]
ECMD = os.path.join(PROJ, "Saved", "PlayTest", "commands_editor.txt")
for town in TOWNS:
    level = f"/Game/Wot/Maps/{town}" if "/" in town else f"/Game/Wot/Maps/{town}/{town}"
    if call(SC + "get_current_level", {}).get("returnValue") != level:
        call(SC + "load_level", {"level_path": level}); time.sleep(6)
    before = len(pt_lines())
    with open(ECMD, "w") as f:
        f.write("wot.Nav.Rebuild\n")
    lines = wait_for("NAV after-build-request", 120, before)
    # wait until no tiles are building, then save
    for _ in range(40):
        time.sleep(3)
        with open(ECMD, "w") as f:
            f.write("wot.Nav.Status\n")
        time.sleep(2)
        st = [l for l in pt_lines()[before:] if "NAV status" in l or "building=" in l]
        if st and "building=0" in st[-1]:
            break
    r = call(AS + "save_assets", {"asset_paths": [level]})
    tiles = next((l[l.find("tiles="):][:40] for l in reversed(pt_lines()[before:]) if "tiles=" in l), "?")
    print(f"{town:13s} navmesh rebuilt ({tiles}) saved={r.get('returnValue', r)}")
