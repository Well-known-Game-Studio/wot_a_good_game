# Play-test drivers

Start and stop the tools' editor only with `editor.sh start|stop|restart|status`: it records the PID it launched and
stops that PID alone after checking its command line, because other Unreal editors run on this machine. Never kill
`UnrealEditor` by name.

Scripts that drive the C++ play-test harness (`Source/VoxelRPG/Testing/WotPlayTestSubsystem`) through the editor's MCP
server (`towns/mcp.py`, editor launched with `-ModelContextProtocolStartServer -ModelContextProtocolPort=8000`). The
subsystem polls `Saved/PlayTest/commands.txt` (PIE world) and `commands_editor.txt` (editor world) and runs each line as a
console command; results are `[PT]` lines in the `LogWotTest` category, read back with `LogsToolset.GetLogEntries`.

    python3 pietest.py <Town> [targets=6] [seconds=150] [interact=1]   # spawn, navmesh walk to random targets, talk to NPCs, capture
    python3 systems_test.py [Town]                                    # pause menu, inventory screen, save and load
    python3 ways_nav.py                                               # editor-world navmesh rebuild + probes along the first Ways bridge
    python3 ways_test.py                                              # the Ways: spawn, channel Fire, walk from the entry to the exit Waygate
    python3 channel_test.py [Town]                                    # every weave for saidar and saidin (captures), then tie off Fire and Earth
    python3 talk_test.py <Town> <NPC label>                          # walk up to one NPC and talk (harness steps the dialogue)
    python3 badrow_test.py [Town] [NPC label]                        # give an NPC a row that is not in the table: no lock, warning logged, player still walks
    python3 menu_input_test.py [Town]                                # open/close inventory (Esc), pause and journal, log the input gates, then walk
    python3 quest_ui_test.py [Town]                                   # quest tracker + journal captures (reset, advance an objective, open the journal)
    python3 record_clips.py [walk|ride|climb|channel ...]            # record PIE clips with wot.PlayTest.Record, then: make_clips.sh <outdir> [name ...] (ffmpeg -> mp4 + gif)
    python3 nav_rebuild_all.py [Town ...]                             # rebuild + save every town's editor navmesh (run after any town rebuild)
    python3 nav_probe_test.py [--rebuild] <Town> x0 y0 x1 y1 [steps]  # navmesh presence along a line in PIE
    python3 goto_test.py <Town> x y [z] [seconds]                     # route to a point in PIE, harness summary

Useful console commands: `wot.PlayTest.Spawn|Walk|Goto x y z|Stop|Status|Mount|Screenshot <name> [ui]|Record <name> [fps] [seconds] [ui]|RecordStop`,
`wot.Nav.Rebuild|Status|Probe x y ...`, `wot.Quest.List|Start|Advance|Reset`, `wot.Save|Load|Saves|NewGame`, `wot.Menu.Pause|Journal|Inventory|Esc|Status|Save|Load|Main|CloseAll`,
`wot.Channel.Weave <Fire|Air|Water|Earth>|Begin|Release|Tie|Half <Saidin|Saidar>`.
Captures and logs are written to `out/` (git-ignored); `wot.PlayTest.Screenshot` writes to `Saved/Screenshots`.
