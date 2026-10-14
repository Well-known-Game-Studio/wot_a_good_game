#!/usr/bin/env python3
"""PIE test of the quest UI: resets the story so the auto-start quest is active, captures the tracker, advances the
first objective, captures again, then opens the journal. usage: quest_ui_test.py [Town=EmondsField]"""
import json, os, re, subprocess, sys, time
town = sys.argv[1] if len(sys.argv) > 1 else "EmondsField"
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(5)
if os.path.exists(CMDFILE): os.remove(CMDFILE)
def quest_lines():
    r = call(LOGS + "GetLogEntries", {"category": "LogWotQuest", "pattern": "", "maxEntries": 0})
    e = r.get("returnValue", r) if isinstance(r, dict) else r
    return [x.get("message", str(x)) if isinstance(x, dict) else str(x) for x in (e or [])]
before = len(quest_lines())
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
send("wot.Quest.Reset", "wot.Quest.List"); time.sleep(2)
send("wot.PlayTest.Screenshot quest_tracker 1"); time.sleep(3)
# first active quest from the List dump, its first objective id from the quest data asset
lines = quest_lines()[before:]
dump = "\n".join(lines)
m = re.search(r"Quest started: ([A-Za-z0-9_]+)", dump) or re.search(r"([A-Za-z0-9_]+) \[ACTIVE\]", dump)
oid = None
if m:
    qid = m.group(1)
    OB = "editor_toolset.toolsets.object.ObjectTools."
    assets = call(AS + "find_assets", {"folder_path": "/Game/Wot/Quests", "name": "", "asset_type": {"refPath": "/Script/VoxelRPG.WotQuestDefinition"}}).get("returnValue") or []
    for a in assets:
        name = a.split("/")[-1]
        props = call(OB + "get_properties", {"instance": {"refPath": f"{a}.{name}"}, "properties": ["QuestId", "Objectives"]}).get("returnValue") or {}
        if isinstance(props, str):
            props = json.loads(props)
        if str(props.get("QuestId")) == qid and props.get("Objectives"):
            oid = props["Objectives"][0].get("objectiveId") or props["Objectives"][0].get("ObjectiveId"); break
if m and oid:
    print("advancing", qid, oid)
    send(f"wot.Quest.Advance {qid} {oid} 1"); time.sleep(1.5)
    send("wot.PlayTest.Screenshot quest_tracker_advanced 1"); time.sleep(3)
else:
    print("no active quest/objective found:\n" + dump[:600])
send("wot.Menu.Journal"); time.sleep(1.5)
send("wot.PlayTest.Screenshot quest_journal 1"); time.sleep(3)
send("wot.Menu.CloseAll"); time.sleep(1)
for l in quest_lines()[before:][:30]:
    print(l[l.find("LogWotQuest"):][:200])
call(APP + "StopPIE", {})
