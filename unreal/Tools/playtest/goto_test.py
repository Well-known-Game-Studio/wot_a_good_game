#!/usr/bin/env python3
"""PIE route test: goto_test.py <Town> x y [z=150] [seconds=120] — spawn, path to the point on the navmesh, print the harness summary."""
import os, sys, time
town = sys.argv[1]; x, y = float(sys.argv[2]), float(sys.argv[3]); z = float(sys.argv[4]) if len(sys.argv) > 4 else 150; secs = int(sys.argv[5]) if len(sys.argv) > 5 else 120
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
send(f"wot.PlayTest.Goto {x:.0f} {y:.0f} {z:.0f}")
lines = wait_for("[PT] SUMMARY", secs + 30, before)
for l in lines[before:]:
    if any(k in l for k in ("PATH to", "NOPATH", "SUMMARY", "REACHED", "UNREACHED", "FELL", "JUMP")): print(l[l.find("[PT]"):][:200])
call(APP + "StopPIE", {})
