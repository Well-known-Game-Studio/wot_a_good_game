#!/usr/bin/env python3
"""Share one animation set across every humanoid (player, main cast, Tam, Lan, Fade, Trolloc), through the editor MCP server.

1. the humanoid skeletons list each other as compatible (same bone names: root/head/arm_r/arm_l/leg_l/leg_r + _end)
2. every humanoid Blueprint's mesh uses the shared Anim Blueprint (ABP) and AnimationBlueprint mode
3. the shared ABP's locomotion blend space is BS_Humanoid_Locomotion (idle / walk / run on Speed)
4. bone translation retargeting = Skeleton on every humanoid skeleton (each mesh keeps its own proportions)
Run wot.Anim.Build (editor console / commands_editor.txt) first so the clips and DA_HumanoidAnims exist.
usage: setup_humanoids.py [--dry]"""
import json, os, subprocess, sys, time
S=os.path.dirname(os.path.abspath(__file__)); PROJ=os.path.abspath(os.path.join(S,"..",".."))
MCP=os.path.join(S,"..","towns","mcp.py"); DRY="--dry" in sys.argv
OB="editor_toolset.toolsets.object.ObjectTools."; AS="editor_toolset.toolsets.asset.AssetTools."; BP="editor_toolset.toolsets.blueprint.BlueprintTools."; AC="editor_toolset.toolsets.actor.ActorTools."; LOGS="EditorToolset.LogsToolset."
SHARED_ABP="/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_animbp"          # the shared humanoid Anim Blueprint (child of Template_AnimBP)
ANIMS="/Game/Wot/Characters/Anims"
SKELETONS=["/Game/Wot/Characters/Rand/MediumFlat/rand_medium_flat_sk","/Game/Wot/Characters/Main/Moiraine/moiraine_medium_flat_Skeleton",
           "/Game/Wot/Characters/Lan/MediumFlat/lan_medium_flat_Skeleton","/Game/Wot/Characters/Tam/medium_flat/tam_medium_flat_skeleton",
           "/Game/Wot/Enemies/fade/medium_flat/fade_medium_flat_Skeleton","/Game/Wot/Enemies/trolloc_boar/MediumFlat/trolloc_boar_Skeleton"]
HUMANOID_BPS=["/Game/Wot/Characters/BP_WotCharacter","/Game/Wot/NPCs/Tam_NPC","/Game/Wot/NPCs/Lan_NPC"]+[f"/Game/Wot/NPCs/Main/{n}_NPC" for n in ("Egwene","Mat","Moiraine","Nynaeve","Perrin","Thom")]
def call(tool,args):
    ts,name=tool.rsplit(".",1)
    out=subprocess.run([sys.executable,MCP,"call","call_tool",json.dumps({"toolset_name":ts,"tool_name":name,"arguments":args})],capture_output=True,text=True,timeout=600).stdout
    try: j=json.loads(out[out.index("{"):out.rindex("}")+1])
    except Exception: return out
    v=j.get("returnValue",j)
    if isinstance(v,str):
        try: v=json.loads(v)
        except Exception: pass
    return v
def ref(p): return {"refPath":p}
def obj(p): return ref(p+"."+p.split("/")[-1])
def props(p,names): 
    v=call(OB+"get_properties",{"instance":obj(p),"properties":names}); return v if isinstance(v,dict) else {}
def setp(p,values):
    if DRY: print("  set",p.split("/")[-1],json.dumps(values)[:140]); return
    r=call(OB+"set_properties",{"instance":ref(p) if (":" in p or "Default__" in p) else obj(p),"values":json.dumps(values)}); 
    if isinstance(r,str) and "rror" in r: print("  !!",p,r[:200])
def save(paths):
    if DRY: return
    for p in paths: call(AS+"save_assets",{"asset_paths":[p]})
# 1. compatible skeletons
for sk in SKELETONS:
    cur=props(sk,["CompatibleSkeletons"]).get("CompatibleSkeletons") or []
    have={(c["refPath"] if isinstance(c,dict) else str(c)).split(".")[0] for c in cur}
    want=[o for o in SKELETONS if o!=sk]
    missing=[o for o in want if o not in have]
    print(f"{sk.split('/')[-1]}: compatible {len(have)} -> +{len(missing)}")
    if missing: setp(sk,{"CompatibleSkeletons":[obj(o) for o in want]})
save(SKELETONS)
# 2. shared ABP on every humanoid Blueprint
for bp in HUMANOID_BPS:
    if not call(AS+"exists",{"path":bp}): print("missing",bp); continue
    cdo=call(BP+"get_default_object",{"blueprint":obj(bp)}); cdo=cdo.get("refPath") if isinstance(cdo,dict) else None
    if not cdo: print("no CDO",bp); continue
    comps=call(AC+"get_components",{"actor":ref(cdo),"component_type":ref("/Script/Engine.SkeletalMeshComponent")}) or []
    for c in comps:
        cp=c["refPath"]; cur=call(OB+"get_properties",{"instance":ref(cp),"properties":["AnimClass"]})
        print(f"{bp.split('/')[-1]}: {cp.split(':')[-1]} AnimClass {str(cur)[-60:]}")
        setp(cp,{"AnimationMode":"AnimationBlueprint","AnimClass":ref(SHARED_ABP+"."+SHARED_ABP.split("/")[-1]+"_C")})
    if not DRY: call(BP+"compile_blueprint",{"blueprint":obj(bp)})
save(HUMANOID_BPS)
# 3. the shared ABP's locomotion blend space (an asset override on the Anim Blueprint's class defaults)
bs=f"{ANIMS}/BS_Humanoid_Locomotion"
if call(AS+"exists",{"path":bs}):
    cdo=SHARED_ABP+".Default__"+SHARED_ABP.split("/")[-1]+"_C"
    setp(cdo,{"idle / Walk Blendspace":obj(bs),"heavy Attack Animation":obj(f"{ANIMS}/A_Humanoid_HeavyAttack")})
    if not DRY: call(BP+"compile_blueprint",{"blueprint":obj(SHARED_ABP)}); save([SHARED_ABP])
    print("shared ABP locomotion ->",bs.split("/")[-1])
else: print("blend space missing (run wot.Anim.Build first):",bs)
# 5. villagers: generic bodies from the humanoid pool (cast meshes wearing each other's palettes, Tam and Lan), one per actor
C="/Game/Wot/Characters"
LOOKS=[(f"{C}/Tam/medium_flat/tam_medium_flat_sk",[]),(f"{C}/Lan/MediumFlat/lan_medium_flat",[]),
       (f"{C}/Main/Perrin/perrin_medium_flat",[f"{C}/Main/Mat/Mat_palette"]),(f"{C}/Main/Mat/mat_medium_flat",[f"{C}/Main/Thom/Thom_palette"]),
       (f"{C}/Main/Thom/thom_medium_flat",[f"{C}/Main/Perrin/Perrin_palette"]),(f"{C}/Main/Nynaeve/nynaeve_medium_flat",[f"{C}/Main/Egwene/Egwene_palette"]),
       (f"{C}/Main/Egwene/egwene_medium_flat",[f"{C}/Main/Nynaeve/Nynaeve_palette"]),(f"{C}/Main/Moiraine/moiraine_medium_flat",[f"{C}/Main/Thom/Thom_palette"])]
VILLAGER="/Game/Wot/NPCs/NPC_Villager_BP"
cdo=VILLAGER+".Default__NPC_Villager_BP_C"
setp(cdo,{"RandomLooks":[{"Mesh":obj(m),"Materials":[obj(x) for x in mats]} for (m,mats) in LOOKS],"RandomLookAnimClass":ref(SHARED_ABP+"."+SHARED_ABP.split("/")[-1]+"_C")})
if not DRY: call(BP+"compile_blueprint",{"blueprint":obj(VILLAGER)}); save([VILLAGER])
print("villager looks:",len(LOOKS))
# 4. translation retargeting (editor console command through the harness)
ECMD=os.path.join(PROJ,"Saved","PlayTest","commands_editor.txt")
if not DRY:
    with open(ECMD,"a") as f:
        for sk in SKELETONS: f.write(f"wot.Skeleton.SetTranslationRetargeting {sk} Skeleton\n")
    time.sleep(6); save(SKELETONS)
print("done")
