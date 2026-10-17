#!/usr/bin/env python3
"""PIE test of the pause menu, inventory screen, save and load. usage: systems_test.py [Town]"""
import json, os, subprocess, sys, time, re
sys.argv = ['x', sys.argv[1] if len(sys.argv) > 1 else 'EmondsField']
town = sys.argv[1]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level})
if os.path.exists(CMDFILE): os.remove(CMDFILE)
before = len(pt_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 4}})
send("wot.PlayTest.Spawn"); time.sleep(2)
send("wot.PlayTest.Goto 0 0 60"); lines = wait_for("[PT] SUMMARY", 80, before)
send("wot.PlayTest.Status", "wot.Save systest"); time.sleep(3)
send("wot.Menu.Pause"); time.sleep(2); send("wot.PlayTest.Screenshot menu_pause 1"); time.sleep(3); send("wot.Menu.CloseAll"); time.sleep(1)
send("wot.Menu.Save"); time.sleep(2); send("wot.PlayTest.Screenshot menu_save 1"); time.sleep(3); send("wot.Menu.CloseAll"); time.sleep(1)
send("wot.Menu.Inventory"); time.sleep(2); send("wot.PlayTest.Screenshot menu_inventory 1"); time.sleep(3); send("wot.Menu.CloseAll"); time.sleep(1)
send("wot.Menu.Main"); time.sleep(2); send("wot.PlayTest.Screenshot menu_main 1"); time.sleep(3); send("wot.Menu.CloseAll"); time.sleep(1)
# walk away, then load the save: the player must be back at the saved spot
send("wot.PlayTest.Goto 1800 1500 60"); lines = wait_for("[PT] SUMMARY", 80, len(lines))
send("wot.PlayTest.Status", "wot.Load systest"); time.sleep(12)
send("wot.PlayTest.Status", "wot.Saves"); time.sleep(3)
lines = pt_lines()
for l in lines[before:]:
    if any(k in l for k in ("STATUS", "SCREENSHOT", "EXEC wot.Save", "EXEC wot.Load", "SPAWN ")): print(l[l.find('[PT]'):][:160])
for cat in ("LogWotSave",):
    r = call(LOGS + "GetLogEntries", {"category": cat, "pattern": "", "maxEntries": 20})
    for x in (r.get("returnValue", []) if isinstance(r, dict) else []):
        print(x[x.find(cat):][:200])
call(APP + "StopPIE", {})
