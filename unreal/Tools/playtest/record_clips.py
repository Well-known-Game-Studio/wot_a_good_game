#!/usr/bin/env python3
"""Record PIE clips for the PR with the harness frame recorder (wot.PlayTest.Record), then cut them with ffmpeg
(make_clips.sh). usage: record_clips.py [walk|ride|climb|channel ...]  (default: all)"""
import os, re, sys, time
which = [a for a in sys.argv[1:]] or ["walk", "ride", "climb", "channel"]
sys.argv = ['x', 'EmondsField']
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
SHOTS = os.path.join(PROJ, "Saved", "Screenshots", "MacEditor")
def level(town):
    lvl = f"/Game/Wot/Maps/{town}/{town}"
    if call(SC + "get_current_level", {}).get("returnValue") != lvl:
        call(SC + "load_level", {"level_path": lvl}); time.sleep(6)
def session(town, name, seconds, ui, body):
    level(town)
    if os.path.exists(CMDFILE): os.remove(CMDFILE)
    before = len(pt_lines())
    call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
    send("wot.PlayTest.Spawn"); time.sleep(3)
    send(f"wot.PlayTest.Record {name} 6 {seconds} {1 if ui else 0}")
    body()
    time.sleep(1.5)
    send("wot.PlayTest.RecordStop"); time.sleep(2)
    n = len([f for f in os.listdir(SHOTS) if f.startswith(f"rec_{name}_")])
    summ = [l[l.find("[PT]"):][:160] for l in pt_lines()[before:] if "SUMMARY" in l or "RECORD" in l or "MOUNT" in l or "RIDE" in l]
    print(f"{name}: {n} frames"); [print("   " + s) for s in summ[-6:]]
    call(APP + "StopPIE", {}); time.sleep(2)
if "walk" in which:
    # Emond's Field: stroll to four villagers and talk to each (dialogue shows in the UI, so record with it)
    def body():
        send("wot.PlayTest.Walk 4 70 1"); wait_for("[PT] SUMMARY", 90, len(pt_lines()))
    session("EmondsField", "walk", 80, True, body)
if "ride" in which:
    def body():
        send("wot.PlayTest.Mount 12 ride"); wait_for("[PT] SUMMARY", 80, len(pt_lines()))
    session("EmondsField", "ride", 60, False, body)
if "climb" in which:
    # Caemlyn: from the east gate through the wall, up the terraces and flights to the palace NPCs
    def body():
        send("wot.PlayTest.Walk 7 100 0"); wait_for("[PT] SUMMARY", 130, len(pt_lines()))
    session("Caemlyn", "climb", 110, False, body)
if "channel" in which:
    def body():
        send("wot.Channel.Half saidar", "wot.Channel.Weave Fire", "wot.Channel.Begin"); time.sleep(3.5)
        send("wot.Channel.Release"); time.sleep(2.5)
        send("wot.Channel.Weave Air", "wot.Channel.Begin"); time.sleep(3.5)
        send("wot.Channel.Tie"); time.sleep(4)
        send("wot.Channel.Weave Earth", "wot.Channel.Begin"); time.sleep(3)
        send("wot.Channel.Tie"); time.sleep(5)
    session("Caemlyn", "channel", 30, False, body)
