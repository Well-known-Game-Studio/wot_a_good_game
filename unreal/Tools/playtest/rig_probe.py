#!/usr/bin/env python3
"""Rig check. Editor world: dump the humanoid skeletons and the locomotion clips of the reference set and one rig set.
PIE: sample one NPC's live pose repeatedly while it wanders (root scale / height must not change between idle and walking).
usage: rig_probe.py [Town=EmondsField] [NPC=Nynaeve] [samples=10] [RigSkeletonName=moiraine_medium_flat_Skeleton]"""
import os, sys, time
town = sys.argv[1] if len(sys.argv) > 1 else "EmondsField"
npc = sys.argv[2] if len(sys.argv) > 2 else "Nynaeve"
samples = int(sys.argv[3]) if len(sys.argv) > 3 else 10
rig = sys.argv[4] if len(sys.argv) > 4 else "moiraine_medium_flat_Skeleton"
sys.argv = ['x', town]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pietest.py')).read().split("def main():")[0])
ECMD = os.path.join(PROJ, "Saved", "PlayTest", "commands_editor.txt")
level = f"/Game/Wot/Maps/{town}/{town}"
if call(SC + "get_current_level", {}).get("returnValue") != level:
    call(SC + "load_level", {"level_path": level}); time.sleep(8)
before = len(pt_lines())
R = "/Game/Wot/Characters/Rand/MediumFlat/"; A = "/Game/Wot/Characters/Anims"
SKELETONS = [R + "rand_medium_flat_sk", "/Game/Wot/Characters/Main/Moiraine/moiraine_medium_flat_Skeleton", "/Game/Wot/Characters/Lan/MediumFlat/lan_medium_flat_Skeleton",
             "/Game/Wot/Characters/Tam/medium_flat/tam_medium_flat_skeleton", "/Game/Wot/Enemies/fade/medium_flat/fade_medium_flat_Skeleton", "/Game/Wot/Enemies/trolloc_boar/MediumFlat/trolloc_boar_Skeleton"]
with open(ECMD, "a") as f:
    for a in SKELETONS + [R + "rand_medium_flat_idle_animseq", R + "rand_medium_flat_walking_animseq", A + "/A_Humanoid_Run", f"{A}/{rig}/A_Humanoid_Idle", f"{A}/{rig}/A_Humanoid_Walk", f"{A}/{rig}/A_Humanoid_Run"]:
        f.write(f"wot.Anim.Dump {a}\n")
time.sleep(6)
for l in pt_lines()[before:]:
    if "ANIMDUMP" in l: print(l[l.find("[PT]"):][:300])
before = len(pt_lines())
if os.path.exists(CMDFILE): os.remove(CMDFILE)
call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 5}})
send("wot.PlayTest.Spawn"); time.sleep(3)
for i in range(samples):
    send(f"wot.Anim.Status {npc}"); time.sleep(1.2)
time.sleep(1)
lines = pt_lines()[before:]
shown = 0
for i, l in enumerate(lines):
    m = l[l.find("[PT]"):]
    if "ANIM " in m and "mesh=" in m and shown == 0: print(m[:300]); shown = 1
    if "ANIM   asset" in m and shown == 1: print(m[:200])
    if "ANIM   vars" in m and shown == 1: shown = 2
    if "ANIM   motion" in m and "speed=0 " not in m:   # the pose while moving: print the two pose lines before it
        for k in (i - 2, i - 1, i): print(lines[k][lines[k].find("[PT]"):][:300])
# walk the player over to the NPC and take a picture of it
loc = None
for l in lines:
    if "[PT] NPC " in l and npc in l and "loc=(" in l:
        loc = [float(v) for v in l.split("loc=(")[1].split(")")[0].split()]
if loc:
    before2 = len(pt_lines())
    send(f"wot.PlayTest.Goto {loc[0] + 250:.0f} {loc[1] + 250:.0f} 45"); wait_for("[PT] SUMMARY", 60, before2)
    send(f"wot.PlayTest.Screenshot rig_{npc} 0"); time.sleep(4)
    send(f"wot.Anim.Status {npc}"); time.sleep(1.5)
    for l in pt_lines()[before2:]:
        if "SCREENSHOT" in l or "SUMMARY" in l or "ANIM   pose " in l: print(l[l.find("[PT]"):][:300])
else:
    print("no NPC line for", npc)
call(APP + "StopPIE", {})
