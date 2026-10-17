#!/usr/bin/env python3
"""PIE navmesh probe along a line: nav_probe_test.py [--rebuild] <Town> x0 y0 x1 y1 [steps=24] — prints which sample points have navmesh (optionally after a runtime navmesh rebuild)."""
import os, sys, time, re
rebuild = "--rebuild" in sys.argv; sys.argv = [a for a in sys.argv if a != "--rebuild"]
town = sys.argv[1]; x0, y0, x1, y1 = map(float, sys.argv[2:6]); steps = int(sys.argv[6]) if len(sys.argv) > 6 else 24
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
if rebuild:
    send("wot.Nav.Rebuild"); time.sleep(25)
pts = [(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps) for i in range(steps + 1)]
send("wot.Nav.Probe " + " ".join(f"{x:.0f} {y:.0f}" for x, y in pts))
lines = wait_for(f"PROBE ({pts[-1][0]:.0f} {pts[-1][1]:.0f})", 30, before)
for l in lines[before:]:
    if "PROBE" in l: print(l[l.find("[PT]"):][:120])
call(APP + "StopPIE", {})
