#!/usr/bin/env python3
"""Drive a PIE walk test of one town through the editor MCP server.

usage: pietest.py <Town> [targets=6] [seconds=150] [interact=1]
Writes [PT] log lines to stdout and a viewport capture to pie_<Town>.png.
"""
import json, os, subprocess, sys, time, re

S = os.path.dirname(os.path.abspath(__file__))
PROJ = os.path.abspath(os.path.join(S, "..", ".."))                 # <repo>/unreal
MCP = os.path.join(S, "..", "towns", "mcp.py")
OUT = os.path.join(S, "out")                                           # captures and logs land here (git-ignored)
CMDFILE = os.path.join(PROJ, "Saved", "PlayTest", "commands.txt")
APP = "EditorToolset.EditorAppToolset."
LOGS = "EditorToolset.LogsToolset."
SC = "editor_toolset.toolsets.scene.SceneTools."
AS = "editor_toolset.toolsets.asset.AssetTools."


def call(tool, args):
    ts, name = tool.rsplit(".", 1)
    payload = json.dumps({"toolset_name": ts, "tool_name": name, "arguments": args})
    out = subprocess.run([sys.executable, MCP, "call", "call_tool", payload],
                         capture_output=True, text=True, timeout=1700).stdout
    try:
        j = json.loads(out[out.index("{"):out.rindex("}") + 1])
    except Exception:
        return out
    return j


def pt_lines(pattern="\\[PT\\]"):
    r = call(LOGS + "GetLogEntries", {"category": "LogWotTest", "pattern": pattern, "maxEntries": 0})
    entries = r.get("returnValue", r) if isinstance(r, dict) else r
    lines = []
    if isinstance(entries, list):
        for e in entries:
            if isinstance(e, dict):
                lines.append(e.get("message") or e.get("text") or json.dumps(e))
            else:
                lines.append(str(e))
    elif isinstance(entries, str):
        lines = entries.splitlines()
    return lines


def send(*cmds):
    """Queue console commands for the harness; appends when the previous batch has not been consumed yet
    (the engine can stall for seconds while a screenshot is written)."""
    os.makedirs(os.path.dirname(CMDFILE), exist_ok=True)
    with open(CMDFILE, "a") as f:
        f.write("\n".join(cmds) + "\n")


def wait_for(marker, timeout, seen_before):
    t0 = time.time()
    while time.time() - t0 < timeout:
        lines = pt_lines()
        new = lines[seen_before:]
        for l in new:
            if marker in l:
                return lines
        time.sleep(3)
    return pt_lines()


def main():
    os.makedirs(OUT, exist_ok=True)
    town = sys.argv[1]
    targets = sys.argv[2] if len(sys.argv) > 2 else "6"
    seconds = sys.argv[3] if len(sys.argv) > 3 else "150"
    interact = sys.argv[4] if len(sys.argv) > 4 else "1"
    level = f"/Game/Wot/Maps/{town}/{town}"
    cur = call(SC + "get_current_level", {}).get("returnValue")
    if cur != level:
        if call(AS + "is_dirty", {"asset_path": cur}).get("returnValue"):
            call(AS + "save_assets", {"asset_paths": [cur]})
        r = call(SC + "load_level", {"level_path": level})
        print("load_level:", r)
    if call(APP + "IsPIERunning", {}).get("returnValue"):
        call(APP + "StopPIE", {})
        time.sleep(2)
    before = len(pt_lines())
    if os.path.exists(CMDFILE):
        os.remove(CMDFILE)
    r = call(APP + "StartPIE", {"options": {"bSimulate": False, "warmupSeconds": 4}})
    print("StartPIE:", str(r)[:200])
    send("wot.PlayTest.Spawn")
    time.sleep(3)
    # phase 1: walk from the entrance to the town centre (roads, ramps, stairs)
    send("wot.PlayTest.Goto 0 0 120")
    lines = wait_for("[PT] SUMMARY", 150, before)
    mid = len(lines)
    # phase 2: visit the nearest NPCs and talk to them
    send(f"wot.PlayTest.Walk {targets} {seconds} {interact}")
    lines = wait_for("[PT] SUMMARY", float(seconds) + 30, mid)
    mid = len(lines)
    # phase 3: ride the nearest horse (if any), then list quest state
    send("wot.PlayTest.Goto 2400 2000 60")
    lines = wait_for("[PT] SUMMARY", 90, mid)
    mid = len(lines)
    send(f"wot.PlayTest.Mount 8 {town}")
    lines = wait_for("[PT] SUMMARY", 150, mid)
    send("wot.Quest.List")
    time.sleep(2)
    lines = pt_lines()
    new = lines[before:]
    for cat in ("LogWotMount", "LogWotQuest"):
        r = call(LOGS + "GetLogEntries", {"category": cat, "pattern": "", "maxEntries": 40})
        v = r.get("returnValue", []) if isinstance(r, dict) else []
        new += [x for x in v if isinstance(x, str)]
    for l in new:
        print(l)
    cap = call(APP + "CaptureViewport", {"annotations": {"gridSpacing": 0, "gridExtent": 0, "gridHeight": 0, "maxLabelDistance": 0}, "bShowUI": True})
    m = re.search(r"(/[^\s\"]+\.png)", json.dumps(cap) if not isinstance(cap, str) else cap)
    print("capture raw:", str(cap)[:300])
    if m:
        subprocess.run(["cp", m.group(1), os.path.join(OUT, f"pie_{town}.png")])
        print("capture:", m.group(1))
    else:
        print("capture:", str(cap)[:300])
    call(APP + "StopPIE", {})
    with open(os.path.join(OUT, f"pie_{town}.log"), "w") as f:
        f.write("\n".join(new))


if __name__ == "__main__":
    main()
