#!/bin/zsh
# Start / stop / restart the one Unreal editor instance the tools drive (MCP server on port 8000), by PID only.
# Other Unreal editors may be running on this machine: never match by process name.
#   editor.sh start      launch the editor with the MCP server (records the PID in Saved/PlayTest/editor.pid)
#   editor.sh stop       SIGTERM that PID (after checking its command line), wait for it to exit
#   editor.sh restart    stop, then start
#   editor.sh status     print the PID and whether the MCP port answers
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; PROJ="$(cd "$HERE/../.." && pwd)"; UPROJECT="$PROJ/VoxelRPG.uproject"
PIDFILE="$PROJ/Saved/PlayTest/editor.pid"; mkdir -p "$(dirname "$PIDFILE")"
UE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
LOG="${EDITOR_LOG:-$PROJ/Saved/PlayTest/editor_launch.log}"
ours() {  # the PID we own, verified by its command line; empty if none
  local pid=""; [ -f "$PIDFILE" ] && pid=$(cat "$PIDFILE")
  if [ -z "$pid" ] || ! ps -p "$pid" >/dev/null 2>&1; then
    pid=$(pgrep -f "VoxelRPG.uproject.*-ModelContextProtocolStartServer" | head -1 || true)
  fi
  [ -n "$pid" ] && ps -o command= -p "$pid" 2>/dev/null | grep -q "VoxelRPG.uproject.*-ModelContextProtocolStartServer" && echo "$pid" || true
}
reaper() {  # SIGTERM makes the editor spawn a CrashReportClient for OUR pid that inherits the MCP socket; close that one only
  sleep 2
  for c in $(pgrep -f "CrashReportClient.*VoxelRPG-pid-$1-" 2>/dev/null); do kill -TERM "$c" 2>/dev/null && echo "closed crash reporter $c (pid $1)"; done
}
case "${1:-status}" in
  start)
    if [ -n "$(ours)" ]; then echo "already running: pid $(ours)"; exit 0; fi
    # a crashed VoxelRPG editor leaves a CrashReportClient that inherited the MCP socket: close that one (ours by project), nothing else
    for c in $(lsof -nP -iTCP:8000 -sTCP:LISTEN -t 2>/dev/null); do
      if ps -o command= -p "$c" | grep -q "CrashReportClient.*VoxelRPG-pid-"; then kill -TERM "$c" && echo "closed crash reporter $c holding port 8000"; sleep 2; fi
    done
    if lsof -nP -iTCP:8000 -sTCP:LISTEN >/dev/null 2>&1; then echo "port 8000 is held by another process; not starting:"; lsof -nP -iTCP:8000 -sTCP:LISTEN; exit 1; fi
    rm -f ~/.ue_mcp_session
    nohup "$UE" "$UPROJECT" -ModelContextProtocolStartServer -ModelContextProtocolPort=8000 -log > "$LOG" 2>&1 &
    echo $! > "$PIDFILE"; echo "started pid $!"
    for i in $(seq 1 40); do sleep 5; curl -s -m 2 -o /dev/null -w "%{http_code}" http://127.0.0.1:8000/mcp 2>/dev/null | grep -q "^[0-9]" && { echo "mcp up after $((i*5))s"; exit 0; }; done
    echo "mcp did not answer within 200s"; exit 1;;
  stop)
    pid="$(ours)"; [ -z "$pid" ] && { echo "no editor of ours running"; exit 0; }
    # clean shutdown first: the play-test harness polls this file in the editor world and runs QUIT_EDITOR (no crash
    # report, no "re-open asset editors?" dialog on the next start); fall back to SIGTERM if it does not exit
    echo "QUIT_EDITOR" >> "$PROJ/Saved/PlayTest/commands_editor.txt"
    for i in $(seq 1 20); do ps -p "$pid" >/dev/null 2>&1 || { echo "stopped pid $pid (clean)"; rm -f "$PIDFILE"; exit 0; }; sleep 3; done
    rm -f "$PROJ/Saved/PlayTest/commands_editor.txt"
    kill -TERM "$pid"; for i in $(seq 1 60); do ps -p "$pid" >/dev/null 2>&1 || { echo "stopped pid $pid"; rm -f "$PIDFILE"; reaper "$pid"; exit 0; }; sleep 3; done
    echo "pid $pid did not exit after 180s (hung shutdown); sending SIGKILL to that PID only"; kill -9 "$pid" || true; rm -f "$PIDFILE";;
  restart) "$0" stop; "$0" start;;
  status)
    pid="$(ours)"; echo "editor pid: ${pid:-none}"; curl -s -m 2 -o /dev/null -w "mcp http %{http_code}\n" http://127.0.0.1:8000/mcp 2>/dev/null || echo "mcp not answering";;
  *) echo "usage: editor.sh start|stop|restart|status"; exit 2;;
esac
