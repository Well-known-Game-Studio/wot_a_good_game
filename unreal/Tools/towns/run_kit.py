def run():
    out={}
    cur=T(SC+"get_current_level",{}); dst="/Game/Wot/Maps/EmondsField/EmondsField"
    if cur!=dst: save([cur]); T(SC+"load_level",{"level_path":dst})
    for name in [f"SM_Stairs{r}" for r in (200,240,280,320,360,400,480,600)]+["SM_WhiteBridge"]:
        try: out[name]=build_kit_mesh(name,KIT[name]())
        except Exception as e: out[name]="ERR "+str(e)[:160]
    for old in ("SM_Stairs300",):
        if T(AS+"exists",{"path":f"{KIT_FOLDER}/{old}"}): T(AS+"delete",{"path":f"{KIT_FOLDER}/{old}"}); out[old]="deleted"
    save([f"{KIT_FOLDER}/{n}" for n in out if "ERR" not in str(out[n]) and out[n]!="deleted"])
    return out
