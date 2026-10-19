#!/usr/bin/env python3
"""PIE: give one NPC a dialogue row that is not in the table, interact, and prove the player is not locked and can
still walk; then restore the row. usage: badrow_test.py [Town=EmondsField] [NPC label=Villager_Cenn]"""
import json, os, sys, time
town = sys.argv[1] if len(sys.argv) > 1 else "EmondsField"; who = sys.argv[2] if len(sys.argv) > 2 else "Villager_Cenn"
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
AC = "editor_toolset.toolsets.actor.ActorTools."; OB = "editor_toolset.toolsets.object.ObjectTools."
def rv(r): return r.get("returnValue") if isinstance(r, dict) else None
level = f"/Game/Wot/Maps/{town}/{town}"
if rv(call(SC + "get_current_level", {})) != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
acts = None
for _ in range(3):
    acts = rv(call(SC + "find_actors", {"name": who, "tag": "", "collision_channels": []}))
    if acts: break
    time.sleep(2)
if not acts: print("no actor", who); sys.exit(1)
ref = {"refPath": acts[0]["refPath"]}
L = rv(call(AC + "get_actor_transform", {"actor": ref}))["location"]
good = rv(call(OB + "get_properties", {"instance": ref, "properties": ["CurrentRowName"]}))
good = (json.loads(good) if isinstance(good, str) else good)["CurrentRowName"]
call(OB + "set_properties", {"instance": ref, "values": json.dumps({"CurrentRowName": "EF_Nope_1"})})
print(f"{who}: row {good} -> EF_Nope_1 (temporary)")
try:
    if os.path.exists(CMDFILE): os.remove(CMDFILE)
    before = len(pt_lines())
    call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
    send("wot.PlayTest.Spawn"); time.sleep(2)
    send(f"wot.PlayTest.Goto {L['x'] - 200:.0f} {L['y']:.0f} 60"); wait_for("[PT] SUMMARY", 90, before)
    b2 = len(pt_lines())
    send("wot.PlayTest.Walk 1 30 1"); wait_for("[PT] SUMMARY", 60, b2)
    send("wot.Menu.Status"); time.sleep(1.5)
    for l in pt_lines()[b2:]:
        if any(k in l for k in ("INTERACT", "REACHED", "SUMMARY")): print("  " + l[l.find("[PT]"):][:180])
    e = rv(call(LOGS + "GetLogEntries", {"category": "LogTemp", "pattern": "MENUSTATUS", "maxEntries": 1})) or []
    print("  " + str(e[-1])[-90:] if e else "  no status")
    e = rv(call(LOGS + "GetLogEntries", {"category": "LogBlueprintUserMessages", "pattern": "row not found", "maxEntries": 1})) or []
    print("  warning logged:", bool(e))
    b3 = len(pt_lines())
    send(f"wot.PlayTest.Goto {L['x'] - 900:.0f} {L['y'] + 300:.0f} 30"); wait_for("[PT] SUMMARY", 45, b3)
    for l in pt_lines()[b3:]:
        if "SUMMARY" in l: print("  after: " + l[l.find("[PT]"):][:150])
    call(APP + "StopPIE", {}); time.sleep(2)
finally:
    call(OB + "set_properties", {"instance": ref, "values": json.dumps({"CurrentRowName": good})})
    print("restored", good, "saved:", rv(call(AS + "save_assets", {"asset_paths": [level]})))
