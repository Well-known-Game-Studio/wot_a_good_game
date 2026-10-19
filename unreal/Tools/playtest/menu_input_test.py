#!/usr/bin/env python3
"""PIE test that every menu gives input back: open and close the inventory (via Escape, the path that used to break
input), the pause menu and the journal, logging the input gates each time, then walk to a nearby point.
usage: menu_input_test.py [Town=EmondsField]"""
import os, re, sys, time
town = sys.argv[1] if len(sys.argv) > 1 else "EmondsField"
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
def status_lines(before):
    r = call(LOGS + "GetLogEntries", {"category": "LogTemp", "pattern": "MENUSTATUS", "maxEntries": 0})
    e = r.get("returnValue", r) if isinstance(r, dict) else r
    return [x.get("message", str(x)) if isinstance(x, dict) else str(x) for x in (e or [])][before:]
base = len(status_lines(0))
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
spawn = [l for l in pt_lines() if "SPAWN pawn" in l][-1]
m = re.search(r"loc=\((-?\d+) (-?\d+) (-?\d+)\)", spawn); x, y = int(m.group(1)), int(m.group(2))
steps = [("inventory open", "wot.Menu.Inventory"), ("inventory esc", "wot.Menu.Esc"),
         ("pause open", "wot.Menu.Pause"), ("pause close", "wot.Menu.Esc"),
         ("journal open", "wot.Menu.Journal"), ("journal close", "wot.Menu.Journal"),
         ("inventory open", "wot.Menu.Inventory"), ("closeall", "wot.Menu.CloseAll")]
for label, cmd in steps:
    send(cmd); time.sleep(1.2); send("wot.Menu.Status"); time.sleep(1.2)
    st = status_lines(0)
    print(f"{label:15s} -> {st[-1][st[-1].find('MENUSTATUS'):] if st else '?'}")
send(f"wot.PlayTest.Goto {x + 600} {y + 300} 40")
lines = wait_for("[PT] SUMMARY", 60, before)
for l in lines[before:]:
    if "SUMMARY" in l or "REACHED" in l: print(l[l.find("[PT]"):][:160])
call(APP + "StopPIE", {})
