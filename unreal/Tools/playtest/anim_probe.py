#!/usr/bin/env python3
"""PIE: log how the player (idle, then walking) and a few NPCs are animated. usage: anim_probe.py [Town] [NPC substrings...]"""
import os, sys, time
town = sys.argv[1] if len(sys.argv) > 1 else "Caemlyn"
npcs = sys.argv[2:] or ["Egwene", "beggar", "Horse"]
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn", "wot.Anim.Status"); time.sleep(3)
send("wot.PlayTest.Walk 1 30 0"); time.sleep(2.5)
send("wot.Anim.Status"); time.sleep(1.5)
wait_for("[PT] SUMMARY", 40, before)
send("wot.PlayTest.Attack"); time.sleep(0.25); send("wot.Anim.Status"); time.sleep(1.5)
for n in npcs:
    send(f"wot.Anim.Status {n}"); time.sleep(1.5)
wait_for("[PT] SUMMARY", 40, before)
for l in pt_lines()[before:]:
    if "ANIM" in l or "SUMMARY" in l or "ATTACK" in l: print(l[l.find("[PT]"):][:400])
for cat in ("LogAnimation", "LogAnimBlueprint", "LogSkeletalMesh"):
    r = call(LOGS + "GetLogEntries", {"category": cat, "pattern": "", "maxEntries": 8}); e = r.get("returnValue", r) if isinstance(r, dict) else r
    if e and isinstance(e, list): print(cat, [(x.get("message") if isinstance(x, dict) else str(x))[-200:] for x in e][-8:])
call(APP + "StopPIE", {})
