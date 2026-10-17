#!/usr/bin/env python3
"""PIE: walk up to one named NPC and talk (the harness steps the dialogue), printing every harness line about it.
usage: talk_test.py <Town> <NPC label substring>"""
import os, re, sys, time
town, who = sys.argv[1], sys.argv[2]
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
AC = "editor_toolset.toolsets.actor.ActorTools."
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
acts = call(SC + "find_actors", {"name": who, "tag": "", "collision_channels": []}).get("returnValue") or []
if not acts: print("no actor matching", who); sys.exit(1)
t = call(AC + "get_actor_transform", {"actor": {"refPath": acts[0]["refPath"]}}).get("returnValue"); L = t["location"]
print("target", acts[0]["refPath"].split(".")[-1], L)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(2)
send(f"wot.PlayTest.Goto {L['x'] - 250:.0f} {L['y']:.0f} 60"); wait_for("[PT] SUMMARY", 90, before)
b2 = len(pt_lines())
send("wot.PlayTest.Walk 1 40 1"); wait_for("[PT] SUMMARY", 70, b2)
send("wot.Menu.Status"); time.sleep(1.5)
for l in pt_lines()[b2:]:
    if any(k in l for k in ("REACHED", "INTERACT", "DIALOG", "Interactions", "SUMMARY", "MISSING", "STUCK", "TALK")): print(l[l.find("[PT]"):][:220])
r = call(LOGS + "GetLogEntries", {"category": "LogTemp", "pattern": "MENUSTATUS", "maxEntries": 1})
e = r.get("returnValue", r); print([x.get("message", str(x))[-110:] if isinstance(x, dict) else str(x)[-110:] for x in (e or [])])
r = call(LOGS + "GetLogEntries", {"category": "LogBlueprintUserMessages", "pattern": "", "maxEntries": 8}); e = r.get("returnValue", r)
print("BP messages:", [str(x)[-160:] for x in (e or [])][-8:])
r = call(LOGS + "GetLogEntries", {"category": "LogScript", "pattern": "", "maxEntries": 6}); e = r.get("returnValue", r)
print("LogScript:", [str(x)[-200:] for x in (e or [])][-6:])
call(APP + "StopPIE", {})
