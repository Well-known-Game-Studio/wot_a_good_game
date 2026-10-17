#!/bin/zsh
# usage: ue.sh <toolset> <tool> '<json args>'
S=$(dirname "$0")
python3 $S/mcp.py call call_tool "{\"toolset_name\":\"$1\",\"tool_name\":\"$2\",\"arguments\":${3:-{\}}}"
