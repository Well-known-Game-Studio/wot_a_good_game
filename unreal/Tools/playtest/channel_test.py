#!/usr/bin/env python3
"""PIE test of channeling: draws each element's threads from the environment, captures them, releases, then ties a weave off.
usage: channel_test.py [Town=EmondsField]"""
import json, os, subprocess, sys, time, re
town = sys.argv[1] if len(sys.argv) > 1 else "EmondsField"
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
def chan_lines():
    r = call(LOGS + "GetLogEntries", {"category": "LogWotChannel", "pattern": "", "maxEntries": 0})
    e = r.get("returnValue", r) if isinstance(r, dict) else r
    return [x.get("message", str(x)) if isinstance(x, dict) else str(x) for x in (e or [])]
before = len(chan_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
for half in ("saidar", "saidin"):
    send(f"wot.Channel.Half {half}")
    for weave in ("Fire", "Air", "Water", "Earth"):
        send(f"wot.Channel.Weave {weave}", "wot.Channel.Begin"); time.sleep(1.4)
        send(f"wot.PlayTest.Screenshot weave_{half}_{weave}"); time.sleep(2.0)
        send("wot.Channel.Release"); time.sleep(1.2)
# tie off a Fire ward and an Earth wall, then look at them
for weave in ("Fire", "Earth"):
    send(f"wot.Channel.Weave {weave}", "wot.Channel.Begin"); time.sleep(1.3)
    send("wot.Channel.Tie"); time.sleep(1.5)
    send(f"wot.PlayTest.Screenshot tied_{weave}"); time.sleep(2.0)
# step back so the knots are in clear view of the camera
spawn = [l for l in pt_lines() if "SPAWN pawn" in l]
m = re.search(r"loc=\((-?\d+) (-?\d+) (-?\d+)\)", spawn[-1]) if spawn else None
if m:
    x, y = int(m.group(1)), int(m.group(2))
    send(f"wot.PlayTest.Goto {x - 420} {y + 120} 150"); time.sleep(6)
    send("wot.PlayTest.Screenshot tied_view"); time.sleep(2.5)
for l in chan_lines()[before:]:
    print(l[l.find("LogWotChannel"):][:200])
call(APP + "StopPIE", {})
