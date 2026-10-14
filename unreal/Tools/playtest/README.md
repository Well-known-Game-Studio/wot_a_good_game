# Play-test drivers

Scripts that drive the C++ play-test harness (`Source/VoxelRPG/Testing/WotPlayTestSubsystem`) through the editor's MCP
server (`towns/mcp.py`, editor launched with `-ModelContextProtocolStartServer -ModelContextProtocolPort=8000`). The
subsystem polls `Saved/PlayTest/commands.txt` (PIE world) and `commands_editor.txt` (editor world) and runs each line as a
console command; results are `[PT]` lines in the `LogWotTest` category, read back with `LogsToolset.GetLogEntries`.

    python3 pietest.py <Town> [targets=6] [seconds=150] [interact=1]   # spawn, navmesh walk to random targets, talk to NPCs, capture
    python3 systems_test.py [Town]                                    # pause menu, inventory screen, save and load
    python3 ways_nav.py                                               # editor-world navmesh rebuild + probes along the first Ways bridge
    python3 ways_test.py                                              # the Ways: spawn, channel Fire, walk from the entry to the exit Waygate
    python3 channel_test.py [Town]                                    # every weave for saidar and saidin (captures), then tie off Fire and Earth

Useful console commands: `wot.PlayTest.Spawn|Walk|Goto x y z|Stop|Status|Mount|Screenshot <name> [ui]`,
`wot.Nav.Rebuild|Status|Probe x y ...`, `wot.Quest.*`, `wot.Save|Load|Saves|NewGame`, `wot.Menu.*`,
`wot.Channel.Weave <Fire|Air|Water|Earth>|Begin|Release|Tie|Half <Saidin|Saidar>`.
Captures and logs are written to `out/` (git-ignored); `wot.PlayTest.Screenshot` writes to `Saved/Screenshots`.
