#!/usr/bin/env python3
"""PIE test of the Ways: spawn, navmesh, walk from the entry Waygate to the exit Waygate, screenshots, channeling."""
import json, os, subprocess, sys, time, re
sys.argv = ['x', 'Ways']
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
AC = "editor_toolset.toolsets.actor.ActorTools."
level = "/Game/Wot/Maps/Ways/Ways"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
acts = call(SC + "find_actors", {"name": "WaysStart_FalDara", "tag": "", "collision_channels": []}).get("returnValue") or []
exit_loc = None
if acts:
    t = call(AC + "get_actor_transform", {"actor": {"refPath": acts[0]["refPath"]}}).get("returnValue")
    exit_loc = t["location"]
print("exit start at", exit_loc)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
send("wot.PlayTest.Screenshot ways_entry"); time.sleep(3)
send("wot.Channel.Weave Fire", "wot.Channel.Begin"); time.sleep(1.2)
send("wot.PlayTest.Screenshot ways_channel"); time.sleep(2)
send("wot.Channel.Release"); time.sleep(2)
if exit_loc:
    send(f"wot.PlayTest.Goto {exit_loc['x']:.0f} {exit_loc['y']:.0f} 240")
    lines = wait_for("[PT] SUMMARY", 270, before)
    send("wot.PlayTest.Screenshot ways_exit"); time.sleep(3)
lines = pt_lines()
for l in lines[before:]:
    if any(k in l for k in ("SPAWN ", "NAV ", "NOPATH", "SUMMARY", "STUCK", "UNREACHED", "REACHED", "FELL", "SCREENSHOT")): print(l[l.find('[PT]'):][:170])
for cat in ("LogWotChannel",):
    r = call(LOGS + "GetLogEntries", {"category": cat, "pattern": "", "maxEntries": 10})
    for x in (r.get("returnValue", []) if isinstance(r, dict) else []): print(x[x.find(cat):][:160])
call(APP + "StopPIE", {})
