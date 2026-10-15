import math
CUBE="/Engine/BasicShapes/Cube.Cube"
CYL="/Engine/BasicShapes/Cylinder.Cylinder"
WATER="/Game/Wot/Materials/MI_VoxelWater.MI_VoxelWater"
GRASSMAT="/Game/Wot/Materials/MI_VoxelGrass.MI_VoxelGrass"
WOODMAT="/Game/Wot/Materials/MI_VoxelWood.MI_VoxelWood"
TOWNPROP="TownProp"
def MAT(n): return f"/Game/Wot/Towns/Common/Materials/{n}.{n}"
M="/Game/Wot/Meshes/"
MESH={
 "house":M+"Buildings/rand_house/rand_house.rand_house",
 "tree1":M+"Foliage/trees/tree_1/tree_1.tree_1","tree2":M+"Foliage/trees/tree_2/tree_2.tree_2",
 "tree3":M+"Foliage/trees/tree_3/tree_3.tree_3","tree4":M+"Foliage/trees/tree_4/tree_4.tree_4",
 "oldtree":M+"Foliage/trees/old_tree/custom_tree.custom_tree",
 "bush1":M+"Foliage/bushes/bush_1/bush_1.bush_1","bush2":M+"Foliage/bushes/bush_2/bush_2.bush_2",
 "grass":M+"Foliage/grass/grass_1.grass_1",
 "mush1":M+"Foliage/mushrooms/mushroom_1/mushroom_1.mushroom_1","mush2":M+"Foliage/mushrooms/mushroom_2/mushroom_2.mushroom_2","mush3":M+"Foliage/mushrooms/mushroom_3/mushroom_3.mushroom_3",
 "well":M+"Buildings/small/small_well.small_well","fence":M+"Buildings/fence/fence.fence","gate":M+"Buildings/fence/gate.gate",
 "campfire":M+"Buildings/campfire/campfire.campfire",
 "wall":M+"Buildings/components/wall.wall","walldoor":M+"Buildings/components/will_with_door.will_with_door",
 "tower":M+"small/small_white_tower.small_white_tower","stone":M+"small/small_stone_of_tear.small_stone_of_tear",
 "mountain":M+"small/small_prop_mountain.small_prop_mountain","dragonmount":M+"small/small_dragonmount.small_dragonmount",
}
def circle(cx,cy,r,n=16,start=0.0):
    return [(cx+r*math.cos(start+2*math.pi*i/n), cy+r*math.sin(start+2*math.pi*i/n)) for i in range(n)]
def rect(x0,y0,x1,y1):
    return [(x0,y0),(x1,y0),(x1,y1),(x0,y1)]

GROUND_TAG="TownGround"
def add_ground(B,src,src_pin,name,x,y,zoff=0):
    """Lift points, raycast down onto actors tagged TownGround, then re-apply a z offset. Returns the last node name."""
    B.node("Transform Points",f"{name}Lift",{"offsetMin":{"x":0,"y":0,"z":3000},"offsetMax":{"x":0,"y":0,"z":3000},"bAbsoluteOffset":True},x=x,y=y)
    B.node("World Raycast",f"{name}Ray",{"raycastMode":"Infinite","bOverrideRayDirections":True,"rayDirection":{"x":0,"y":0,"z":-1},"bKeepOriginalPointOnMiss":False,"bUnbounded":True,
        "worldQueryParams":{"bIgnorePCGHits":True,"bIgnoreSelfHits":True,"collisionChannel":"ECC_WorldStatic","actorTagFilter":"Require","actorTagsList":GROUND_TAG,"bTraceComplex":False}},title=f"{name}: ground",x=x+1,y=y)
    B.link(src,src_pin,f"{name}Lift","In"); B.link(f"{name}Lift","Out",f"{name}Ray","Origins")
    last=f"{name}Ray"
    if zoff:
        B.node("Transform Points",f"{name}Z",{"offsetMin":{"x":0,"y":0,"z":zoff},"offsetMax":{"x":0,"y":0,"z":zoff},"bAbsoluteOffset":True},x=x+2,y=y)
        B.link(last,"Out",f"{name}Z","In"); last=f"{name}Z"
    return last
def build_town_graph(spec):
    """Creates the per-town PCG graph from a spec. Returns (graphPath, info)."""
    g=create_graph(spec["graph_name"], spec["folder"]); B=G(g); info={"graph":g}
    y=0
    excl=[]
    if gv(spec,"exclusions"):
        pts=[]
        for (ex,ey,hx,hy) in spec["exclusions"]:
            p=point(ex,ey,0,0,1.0,50); p["boundsMin"]={"x":-hx,"y":-hy,"z":-2500}; p["boundsMax"]={"x":hx,"y":hy,"z":3500}; pts.append(p)
        B.node("Create Points","ExclPts",{"pointsToCreate":pts,"coordinateSpace":"World"},title="Hand-placed exclusions",x=0,y=y); excl.append(("ExclPts","Out")); y+=1
    plaza_surfs=[]
    for i,pz in enumerate(gv(spec,"plazas") or []):
        pts=[point(pt[0],pt[1],(pt[2] if len(pt)>2 else gv(pz,"z",0)),0,1.0,50) for pt in pz["pts"]]
        lin=gv(pz,"linear",False)
        B.node("Create Points",f"PlazaPts{i}",{"pointsToCreate":pts,"coordinateSpace":"World"},title=f"Plaza {i} outline",x=0,y=y)
        B.node("Create Spline",f"PlazaSpl{i}",{"bClosedLoop":True,"bLinear":lin},x=1,y=y)
        B.node("Create Surface From Spline",f"PlazaSurf{i}",{},x=2,y=y)
        B.node("Spline Sampler",f"PlazaSamp{i}",{"samplerParams":{"dimension":"OnInterior","interiorSampleSpacing":100,"interiorBorderSampleSpacing":100,"bUnbounded":True,"bTreatSplineAsPolyline":lin}},x=2,y=y+1)
        B.node("Transform Points",f"PlazaScale{i}",{"scaleMin":{"x":1,"y":1,"z":0.06},"scaleMax":{"x":1,"y":1,"z":0.06},"bAbsoluteScale":True},x=3,y=y+1)
        B.chain((f"PlazaPts{i}","Out","In"),(f"PlazaSpl{i}","Out","In"),(f"PlazaSurf{i}","Out","In"))
        B.chain((f"PlazaSpl{i}","Out","In"),(f"PlazaSamp{i}","Out","Spline"),(f"PlazaScale{i}","Out","In"))
        if not gv(spec,"_relief") or gv(pz,"tiles",False):
            B.node("Static Mesh Spawner",f"PlazaSpawn{i}",{},title=f"Plaza {i} tiles",x=6,y=y+1)
            B.set_meshes(f"PlazaSpawn{i}",[(CUBE,1,[pz["mat"]])])
            B.link(f"PlazaScale{i}","Out",f"PlazaSpawn{i}","In")
        pg=f"PlazaSamp{i}"
        for si,(smesh,sratio,smin_,smax_,smats) in enumerate(gv(pz,"scatter") or []):
            B.node("Random Choice",f"PlazaPick{i}_{si}",{"bFixedMode":False,"ratio":sratio},x=3,y=y+2+si)
            B.node("Transform Points",f"PlazaSXf{i}_{si}",{"rotationMin":{"pitch":0,"yaw":0,"roll":0},"rotationMax":{"pitch":0,"yaw":360,"roll":0},"scaleMin":{"x":smin_,"y":smin_,"z":smin_},"scaleMax":{"x":smax_,"y":smax_,"z":smax_},"bUniformScale":True,"bAbsoluteScale":True,"bRecomputeSeed":True},x=4,y=y+2+si)
            B.node("Bounds Modifier",f"PlazaSBnd{i}_{si}",{"mode":"Set","boundsMin":{"x":-250,"y":-250,"z":0},"boundsMax":{"x":250,"y":250,"z":300}},x=5,y=y+2+si)
            B.node("Self Pruning",f"PlazaSPrune{i}_{si}",{"parameters":{"pruningType":"LargeToSmall","bRandomizedPruning":True}},x=6,y=y+2+si)
            B.node("Static Mesh Spawner",f"PlazaSSpawn{i}_{si}",{},title=f"plaza {i} scatter {si}",x=7,y=y+2+si)
            B.set_meshes(f"PlazaSSpawn{i}_{si}",[(smesh,1,smats)])
            B.chain((pg,"Out","In"),(f"PlazaPick{i}_{si}","Chosen","In"),(f"PlazaSXf{i}_{si}","Out","In"),(f"PlazaSBnd{i}_{si}","Out","In"),(f"PlazaSPrune{i}_{si}","Out","In"),(f"PlazaSSpawn{i}_{si}","Out","In"))
        plaza_surfs.append((f"PlazaSurf{i}","Out")); y+=2+len(gv(pz,"scatter") or [])
    rows={}; groups={}; gorder=[]; house_samplers=[]; wall_nodes=[]
    for i,rd in enumerate(spec["roads"]):
        w=gv(rd,"width",7); ts=tuple(gv(rd,"tile_scale",(1,1,0.06))); tz=gv(rd,"tile_z",0); step=gv(rd,"tile_step",100); pitch=gv(rd,"row_pitch",100)
        key=(w,ts,tz,pitch)
        if key not in rows:
            name=f"Row{len(rows)}"
            B.node("Create Points",name,{"pointsToCreate":[point(0,(k-(w-1)/2.0)*pitch,tz,0,ts,50) for k in range(w)],"coordinateSpace":"World"},title=f"Tile row w={w}",x=0,y=y)
            rows[key]=name; y+=1
        pts=[point(pt[0],pt[1],(pt[2] if len(pt)>2 else 0),0,1.0,50) for pt in rd["pts"]]
        closed=gv(rd,"closed",False); kind=gv(rd,"kind","road")
        B.node("Create Points",f"RoadPts{i}",{"pointsToCreate":pts,"coordinateSpace":"World"},title=f"{kind} {i}: {gv(rd,'name','')}",x=0,y=y)
        B.node("Create Spline",f"RoadSpl{i}",{"bClosedLoop":closed,"bLinear":gv(rd,"linear",False)},x=1,y=y)
        B.node("Spline Sampler",f"RoadSamp{i}",{"samplerParams":{"dimension":"OnSpline","mode":"Distance","distanceIncrement":step,"bUnbounded":True}},x=2,y=y)
        B.node("Copy Points",f"RoadTiles{i}",{"rotationInheritance":"Target" if gv(rd,"align",False) else "Source","bApplyTargetRotationToPositions":True,"scaleInheritance":"Source","bApplyTargetScaleToPositions":False,"attributeInheritance":"TargetFirst"},x=3,y=y)
        B.chain((f"RoadPts{i}","Out","In"),(f"RoadSpl{i}","Out","In"),(f"RoadSamp{i}","Out","Spline"))
        B.link(rows[key],"Out",f"RoadTiles{i}","Source"); B.link(f"RoadSamp{i}","Out",f"RoadTiles{i}","Target")
        tiles_node=f"RoadTiles{i}"
        if gv(rd,"keep") is not None:
            B.node("Random Choice",f"RoadKeep{i}",{"bFixedMode":False,"ratio":rd["keep"]},title="gaps",x=4,y=y)
            B.link(tiles_node,"Out",f"RoadKeep{i}","In"); tiles_node=f"RoadKeep{i}"
        mat=gv(rd,"mat",spec["road_mat"]); mesh=gv(rd,"mesh",CUBE)
        # relief paints carved roads (and water channels with a water plane) into the terrain: their tile slabs would only
        # z-fight / float at the steps, so such groups keep their points for exclusions but spawn no tiles
        painted=bool(gv(spec,"_relief")) and not gv(rd,"nocarve",False) and kind in ("road","water")
        gk=(mat,mesh,kind,painted)
        if gk not in groups: groups[gk]=[]; gorder.append(gk)
        groups[gk].append(tiles_node)
        if gv(rd,"houses",False): house_samplers.append((i,rd))
        for li,ln in enumerate(([] if gv(spec,"line_sets") is not None else gv(rd,"lines")) or []):
            sc=gv(ln,"scale",(1,1,1)); off=ln["offset"]; mats=[gv(ln,"mat")] if gv(ln,"mat") else None
            B.node("Spline Sampler",f"LineSamp{i}_{li}",{"samplerParams":{"dimension":"OnSpline","mode":"Distance","distanceIncrement":ln["step"],"bUnbounded":True}},x=2,y=y+1)
            B.link(f"RoadSpl{i}","Out",f"LineSamp{i}_{li}","Spline")
            sides=[off] if gv(ln,"sides","both")=="left" else ([-off] if gv(ln,"sides","both")=="right" else [off,-off])
            for si,sg in enumerate(sides):
                yaw=gv(ln,"yaw",0)+(90 if sg>0 else -90)
                B.node("Transform Points",f"LineXf{i}_{li}_{si}",{"offsetMin":{"x":0,"y":sg,"z":gv(ln,"z",0)},"offsetMax":{"x":0,"y":sg,"z":gv(ln,"z",0)},"rotationMin":{"pitch":0,"yaw":yaw,"roll":0},"rotationMax":{"pitch":0,"yaw":yaw,"roll":0},"scaleMin":{"x":sc[0],"y":sc[1],"z":sc[2]},"scaleMax":{"x":sc[0],"y":sc[1],"z":sc[2]},"bAbsoluteScale":True},x=3,y=y+1)
                B.node("Static Mesh Spawner",f"LineSpawn{i}_{li}_{si}",{},title=f"line {li} road {i}",x=5,y=y+1)
                B.set_meshes(f"LineSpawn{i}_{li}_{si}",[(ln["mesh"],1,mats)])
                B.link(f"LineSamp{i}_{li}","Out",f"LineXf{i}_{li}_{si}","In"); B.link(f"LineXf{i}_{li}_{si}","Out",f"LineSpawn{i}_{li}_{si}","In")
            y+=1
        tw=None if gv(spec,"tower_sets") is not None else gv(rd,"towers")
        if tw:
            sc=tw["scale"]
            B.node("Spline Sampler",f"TowerSamp{i}",{"samplerParams":{"dimension":"OnSpline","mode":"Distance","distanceIncrement":tw["step"],"bUnbounded":True}},x=2,y=y+1)
            B.node("Transform Points",f"TowerXf{i}",{"offsetMin":{"x":0,"y":0,"z":tw["z"]},"offsetMax":{"x":0,"y":0,"z":tw["z"]},"bAbsoluteOffset":True,"rotationMin":{"pitch":0,"yaw":0,"roll":0},"rotationMax":{"pitch":0,"yaw":0,"roll":0},"bAbsoluteRotation":True,"scaleMin":{"x":sc[0],"y":sc[1],"z":sc[2]},"scaleMax":{"x":sc[0],"y":sc[1],"z":sc[2]},"bAbsoluteScale":True},x=3,y=y+1)
            B.node("Static Mesh Spawner",f"TowerSpawn{i}",{},title=f"Towers {i}",x=5,y=y+1)
            B.set_meshes(f"TowerSpawn{i}",[(gv(tw,"mesh",CUBE),1,[tw["mat"]])])
            B.link(f"RoadSpl{i}","Out",f"TowerSamp{i}","Spline"); B.link(f"TowerSamp{i}","Out",f"TowerXf{i}","In"); B.link(f"TowerXf{i}","Out",f"TowerSpawn{i}","In")
            y+=1
        y+=1
    road_merges=[]; other_merges=[]; gsrc={}
    for j,gk in enumerate(gorder):
        mat,mesh,kind,painted=gk
        B.node("Merge Points",f"GrpMerge{j}",{},title=f"{kind} tiles"+(" (painted)" if painted else ""),x=4,y=y)
        for n in groups[gk]: B.link(n,"Chosen" if n.startswith("RoadKeep") else "Out",f"GrpMerge{j}","In")
        gsrc[gk]=f"GrpMerge{j}"
        if kind=="wall": wall_nodes.append(f"GrpMerge{j}")
        elif kind=="road": road_merges.append(f"GrpMerge{j}")
        else: other_merges.append(f"GrpMerge{j}")
        y+=1
    B.node("Merge Points","RoadAll",{},x=5,y=y)
    for n in road_merges: B.link(n,"Out","RoadAll","In")
    B.node("Bounds Modifier","RoadExcl",{"mode":"Set","boundsMin":{"x":-30,"y":-30,"z":-3000},"boundsMax":{"x":30,"y":30,"z":3000}},title="Road exclusion",x=6,y=y)
    B.link("RoadAll","Out","RoadExcl","In"); y+=1
    for j,gk in enumerate(gorder):
        mat,mesh,kind,painted=gk; src=gsrc[gk]
        if kind=="nobuild" or painted: y+=1; continue
        if kind=="wall":
            B.node("Difference",f"WallCut{j}",{"densityFunction":"Binary","mode":"Inferred"},title="Gates where roads cross",x=6,y=y)
            B.link(src,"Out",f"WallCut{j}","Source"); B.link("RoadExcl","Out",f"WallCut{j}","Differences")
            for (n_,p_) in excl: B.link(n_,p_,f"WallCut{j}","Differences")   # hand-placed exclusions and stair footprints open the wall too
            src=f"WallCut{j}"
        B.node("Static Mesh Spawner",f"GrpSpawn{j}",{},title=f"{kind} spawner",x=7,y=y)
        B.set_meshes(f"GrpSpawn{j}",[(mesh,1,[mat] if mat else None)])
        B.link(src,"Out",f"GrpSpawn{j}","In"); y+=1
    hs=gv(spec,"houses"); house_inputs=[]
    sets=gv(spec,"house_sets") or {}
    set_inputs={k:[] for k in sets}
    for (i,rd) in ([] if gv(spec,"house_slots") is not None else house_samplers):
        sp=gv(rd,"spacing",hs["spacing"]); sb=gv(rd,"setback",hs["setback"]); sides=gv(rd,"sides","both")
        B.node("Spline Sampler",f"HSamp{i}",{"samplerParams":{"dimension":"OnSpline","mode":"Distance","distanceIncrement":sp,"bUnbounded":True}},title=f"House slots {i}",x=2,y=y)
        B.link(f"RoadSpl{i}","Out",f"HSamp{i}","Spline")
        target=set_inputs[rd["set"]] if gv(rd,"set") in sets else house_inputs
        if sides in ("both","left"):
            B.node("Transform Points",f"HL{i}",{"offsetMin":{"x":0,"y":sb,"z":0},"offsetMax":{"x":0,"y":sb,"z":0},"rotationMin":{"pitch":0,"yaw":90,"roll":0},"rotationMax":{"pitch":0,"yaw":90,"roll":0}},title="Left side",x=3,y=y)
            B.link(f"HSamp{i}","Out",f"HL{i}","In"); target.append(f"HL{i}")
        if sides in ("both","right"):
            B.node("Transform Points",f"HR{i}",{"offsetMin":{"x":0,"y":-sb,"z":0},"offsetMax":{"x":0,"y":-sb,"z":0},"rotationMin":{"pitch":0,"yaw":-90,"roll":0},"rotationMax":{"pitch":0,"yaw":-90,"roll":0}},title="Right side",x=3,y=y+1)
            B.link(f"HSamp{i}","Out",f"HR{i}","In"); target.append(f"HR{i}")
        y+=2
    hp=gv(spec,"house_pts")
    if hp and gv(spec,"house_slots") is None:
        B.node("Create Points","HousePts",{"pointsToCreate":[point(x,yy,0,yaw,1.0,50) for (x,yy,yaw) in hp],"coordinateSpace":"World"},title="Hand-placed house slots",x=3,y=y)
        house_inputs.append("HousePts"); y+=1
    def chunked_points(prefix,pts,x,y,title="",chunk=1200):
        """Create Points nodes in chunks merged into one; returns (merge node name, node rows used)."""
        names=[]
        for ci in range(0,max(1,len(pts)),chunk):
            nm=f"{prefix}{ci//chunk}"; B.node("Create Points",nm,{"pointsToCreate":pts[ci:ci+chunk],"coordinateSpace":"World"},title=f"{title} {ci//chunk}",x=x,y=y+ci//chunk); names.append(nm)
        B.node("Merge Points",f"{prefix}Merge",{},title=title,x=x+1,y=y)
        for nm in names: B.link(nm,"Out",f"{prefix}Merge","In")
        return f"{prefix}Merge", max(1,len(names))
    hsl=gv(spec,"house_slots")
    if hsl is not None:
        for k,pts in hsl.items():
            if not pts: continue
            nm,cnt=chunked_points(f"Slots{k}",[point(x,yy,z,yaw,1.0,50) for (x,yy,z,yaw) in pts],2,y,title=f"house slots {k or 'default'}")
            (set_inputs[k] if k in sets else house_inputs).append(nm); y+=cnt
    for li,ls in enumerate(gv(spec,"line_sets") or []):
        if not ls["pts"]: continue
        sc=ls["scale"]; nm,cnt=chunked_points(f"Line{li}",[point(x,yy,z,yaw,(sc[0],sc[1],sc[2]),50) for (x,yy,z,yaw) in ls["pts"]],2,y,title=f"line {li} {ls['name']}")
        B.node("Static Mesh Spawner",f"LineSpawn{li}",{},title=f"line {li} {ls['name']}",x=4,y=y); B.set_meshes(f"LineSpawn{li}",[(ls["mesh"],1,ls["mats"])]); B.link(nm,"Out",f"LineSpawn{li}","In"); y+=cnt
    for ti,ts_ in enumerate(gv(spec,"tower_sets") or []):
        if not ts_["pts"]: continue
        sc=ts_["scale"]; nm,cnt=chunked_points(f"Tower{ti}",[point(x,yy,z,yaw,(sc[0],sc[1],sc[2]),50) for (x,yy,z,yaw) in ts_["pts"]],2,y,title=f"towers {ts_['name']}")
        B.node("Static Mesh Spawner",f"TowerSpawn{ti}",{},title=f"towers {ts_['name']}",x=4,y=y); B.set_meshes(f"TowerSpawn{ti}",[(ts_["mesh"],1,ts_["mats"])]); B.link(nm,"Out",f"TowerSpawn{ti}","In"); y+=cnt
    house_excl_nodes=[]
    def house_pipeline(tag,inputs,cfg):
        nonlocal y
        if not inputs or not cfg: return
        B.node("Merge Points",f"HouseMerge{tag}",{},x=4,y=y)
        for n in inputs: B.link(n,"Out",f"HouseMerge{tag}","In")
        smin,smax=cfg["scale"]; fp=gv(cfg,"footprint",(-560,-560,740,420)); jit=gv(cfg,"jitter",120)
        B.node("Transform Points",f"HouseJitter{tag}",{"offsetMin":{"x":-jit,"y":-40,"z":0},"offsetMax":{"x":jit,"y":40,"z":0},"rotationMin":{"pitch":0,"yaw":-4,"roll":0},"rotationMax":{"pitch":0,"yaw":4,"roll":0},"scaleMin":{"x":smin,"y":smin,"z":smin},"scaleMax":{"x":smax,"y":smax,"z":smax},"bUniformScale":True,"bRecomputeSeed":True},x=5,y=y)
        B.node("Random Choice",f"HouseChoice{tag}",{"bFixedMode":False,"ratio":gv(cfg,"keep",0.9)},x=6,y=y)
        sh=gv(cfg,"excl_shrink",0.55)   # the Difference is binary (any overlap culls), so test a shrunken core of the footprint
        B.node("Bounds Modifier",f"HouseCore{tag}",{"mode":"Set","boundsMin":{"x":fp[0]*sh,"y":fp[1]*sh,"z":0},"boundsMax":{"x":fp[2]*sh,"y":fp[3]*sh,"z":700}},title="House core (exclusion test)",x=7,y=y)
        B.node("Difference",f"HouseDiff{tag}",{"densityFunction":"Binary","mode":"Inferred"},title="Not on roads/plazas/walls",x=8,y=y)
        B.node("Bounds Modifier",f"HouseBounds{tag}",{"mode":"Set","boundsMin":{"x":fp[0],"y":fp[1],"z":0},"boundsMax":{"x":fp[2],"y":fp[3],"z":700}},title="House footprint",x=9,y=y)
        B.node("Self Pruning",f"HousePrune{tag}",{"parameters":{"pruningType":"LargeToSmall","bRandomizedPruning":True}},x=10,y=y)
        B.node("Static Mesh Spawner",f"HouseSpawn{tag}",{"bApplyMeshBoundsToPoints":True},title=f"Houses {tag}",x=11,y=y)
        B.set_meshes(f"HouseSpawn{tag}",cfg["entries"])
        B.chain((f"HouseMerge{tag}","Out","In"),(f"HouseJitter{tag}","Out","In"),(f"HouseChoice{tag}","Chosen","In"),(f"HouseCore{tag}","Out","In"),(f"HouseDiff{tag}","Out","Source"))
        B.chain((f"HouseDiff{tag}","Out","In"),(f"HouseBounds{tag}","Out","In"),(f"HousePrune{tag}","Out","In"),(f"HouseSpawn{tag}","Out","In"))
        B.link("RoadExcl","Out",f"HouseDiff{tag}","Differences")
        for n in wall_nodes+other_merges: B.link(n,"Out",f"HouseDiff{tag}","Differences")
        for (n,p) in plaza_surfs+excl: B.link(n,p,f"HouseDiff{tag}","Differences")
        for n in house_excl_nodes: B.link(n,"Out",f"HouseDiff{tag}","Differences")   # later sets avoid earlier houses
        B.node("Bounds Modifier",f"HouseExcl{tag}",{"mode":"Scale","boundsMin":{"x":1.0,"y":1.0,"z":1},"boundsMax":{"x":1.0,"y":1.0,"z":1}},title="House exclusion",x=12,y=y)
        B.link(f"HouseSpawn{tag}","Out",f"HouseExcl{tag}","In"); house_excl_nodes.append(f"HouseExcl{tag}")
        y+=1
    house_pipeline("",house_inputs,hs)
    for k in sets: house_pipeline(k,set_inputs[k],sets[k])
    terr=gv(spec,"terrain")
    if terr:
        groups={}
        for (m,tx,ty,tz,sx,sy,sz) in terr["boxes"]:
            if m not in groups: groups[m]=[]
            groups[m].append(point(tx,ty,tz,0,(sx/100.0,sy/100.0,sz/100.0),50))
        for gi,(m,pts) in enumerate(groups.items()):
            mat=terr["palette"][m] if m in terr["palette"] else m
            mg,cnt=chunked_points(f"Terr{gi}",pts,0,y,title=f"terrain {m.split('/')[-1]}")
            B.node("Static Mesh Spawner",f"TerrSpawn{gi}",{},title=f"terrain {m.split('/')[-1]}",x=2,y=y)
            B.set_meshes(f"TerrSpawn{gi}",[(CUBE,1,[mat])])
            B.link(mg,"Out",f"TerrSpawn{gi}","In"); y+=cnt
    fo=spec["foliage"]; E=fo["extent"]; cx,cy=spec["center"]
    if not gv(spec,"lattice"): B.node("Create Points Grid","Grid",{"gridExtents":{"x":E,"y":E,"z":1},"cellSize":{"x":100,"y":100,"z":100},"coordinateSpace":"World","pointPosition":"CellCenter","bSetPointsBounds":True},title="Ground lattice",x=0,y=y)
    B.node("Transform Points","GridJitter",{"offsetMin":{"x":-45,"y":-45,"z":0},"offsetMax":{"x":45,"y":45,"z":0},"bRecomputeSeed":True},x=1,y=y)
    B.node("Create Points","Center",{"pointsToCreate":[point(cx,cy,0,0,1.0,10)],"coordinateSpace":"World"},title="Town center",x=0,y=y+1)
    B.node("Distance","Dist",{"bSetDensity":True,"maximumDistance":fo["forest_radius"],"sourceShape":"Center","targetShape":"Center","bOutputToAttribute":True,"outputAttribute":"Dist"},title="Density = distance from center",x=2,y=y)
    B.node("Difference","Excl",{"densityFunction":"Minimum","mode":"Inferred"},title="Remove roads/houses/plazas",x=3,y=y)
    lat=gv(spec,"lattice"); terr=gv(spec,"terraces") or []; bz=gv(spec,"base_z",0)
    if lat:
        lp=[point(px,py,pz,0,1.0,50) for (px,py,pz) in lat]
        mg,cnt=chunked_points("Lat",lp,0,y+1,title="terrain lattice")
        B.update("GridJitter",{"offsetMin":{"x":-110,"y":-110,"z":0},"offsetMax":{"x":110,"y":110,"z":0},"bRecomputeSeed":True})
        B.link(mg,"Out","GridJitter","In"); B.link("GridJitter","Out","Dist","Source"); y+=1+cnt
    elif terr:
        B.node("Merge Points","GridMerge",{},title="lattice by terrace",x=2,y=y+1)
        B.node("Difference","GridBase",{"densityFunction":"Binary","mode":"Inferred"},title="base ground",x=1,y=y+2)
        B.link("Grid","Out","GridBase","Source")
        if bz:
            B.node("Transform Points","GridBaseZ",{"offsetMin":{"x":0,"y":0,"z":bz},"offsetMax":{"x":0,"y":0,"z":bz},"bAbsoluteOffset":True},x=2,y=y+2)
            B.link("GridBase","Out","GridBaseZ","In"); B.link("GridBaseZ","Out","GridMerge","In")
        else: B.link("GridBase","Out","GridMerge","In")
        for ti,tr in enumerate(terr):
            B.node("Create Points",f"TerrPts{ti}",{"pointsToCreate":[point(x,yy,0,0,1.0,50) for (x,yy) in tr["pts"]],"coordinateSpace":"World"},title=f"terrace {ti}",x=0,y=y+3+ti)
            B.node("Create Spline",f"TerrSpl{ti}",{"bClosedLoop":True,"bLinear":gv(tr,"linear",False)},x=1,y=y+3+ti)
            B.node("Create Surface From Spline",f"TerrSurf{ti}",{},x=2,y=y+3+ti)
            B.chain((f"TerrPts{ti}","Out","In"),(f"TerrSpl{ti}","Out","In"),(f"TerrSurf{ti}","Out","In"))
            B.node("Intersection",f"TerrCut{ti}",{},x=3,y=y+3+ti)
            B.link("Grid","Out",f"TerrCut{ti}","Primary Source"); B.link(f"TerrSurf{ti}","Out",f"TerrCut{ti}","Source1")
            B.node("Transform Points",f"TerrZ{ti}",{"offsetMin":{"x":0,"y":0,"z":tr["z"]},"offsetMax":{"x":0,"y":0,"z":tr["z"]},"bAbsoluteOffset":True},x=4,y=y+3+ti)
            B.link(f"TerrCut{ti}","Out",f"TerrZ{ti}","In"); B.link(f"TerrZ{ti}","Out","GridMerge","In")
            B.link(f"TerrSurf{ti}","Out","GridBase","Differences")
        B.link("GridMerge","Out","GridJitter","In"); B.link("GridJitter","Out","Dist","Source")
        y+=3+len(terr)
    else:
        B.chain(("Grid","Out","In"),("GridJitter","Out","In"),("Dist","Out","Source"))
    B.link("Center","Out","Dist","Target"); B.link("Dist","Out","Excl","Source")
    B.link("RoadExcl","Out","Excl","Differences")
    for n in wall_nodes+other_merges: B.link(n,"Out","Excl","Differences")
    for n in house_excl_nodes: B.link(n,"Out","Excl","Differences")
    for (n,p) in plaza_surfs+excl: B.link(n,p,"Excl","Differences")
    y+=2
    clip=gv(fo,"clip_density",1.0); dens=gv(spec,"lattice_density",1.0)
    def scatter(name,lo,hi,ratio,entries,smin,smax,prune=None,yawrand=True,x0=4):
        nonlocal y
        if ratio<=0 or not entries: return
        ratio=min(1.0,ratio*dens); hi=min(hi,clip)
        B.node("Density Filter",f"{name}Band",{"lowerBound":lo,"upperBound":hi},title=f"{name}: density {lo}-{hi}",x=x0,y=y)
        B.node("Random Choice",f"{name}Pick",{"bFixedMode":False,"ratio":ratio},x=x0+1,y=y)
        rot={"pitch":0,"yaw":0,"roll":0}; rot2={"pitch":0,"yaw":360 if yawrand else 0,"roll":0}
        B.node("Transform Points",f"{name}Xform",{"rotationMin":rot,"rotationMax":rot2,"scaleMin":{"x":smin,"y":smin,"z":smin},"scaleMax":{"x":smax,"y":smax,"z":smax},"bUniformScale":True,"bRecomputeSeed":True},x=x0+2,y=y)
        B.chain(("Excl","Out","In"),(f"{name}Band","Out","In"),(f"{name}Pick","Chosen","In"),(f"{name}Xform","Out","In"))
        last=f"{name}Xform"
        if prune:
            B.node("Bounds Modifier",f"{name}Bnd",{"mode":"Set","boundsMin":{"x":-prune,"y":-prune,"z":0},"boundsMax":{"x":prune,"y":prune,"z":prune}},x=x0+3,y=y)
            B.node("Self Pruning",f"{name}Prune",{"parameters":{"pruningType":"LargeToSmall","bRandomizedPruning":True}},x=x0+4,y=y)
            B.chain((last,"Out","In"),(f"{name}Bnd","Out","In"),(f"{name}Prune","Out","In")); last=f"{name}Prune"
        B.node("Static Mesh Spawner",f"{name}Spawn",{},title=name,x=x0+5,y=y)
        B.set_meshes(f"{name}Spawn",entries); B.link(last,"Out",f"{name}Spawn","In")
        y+=1
    trees=fo["trees"]
    ts=gv(fo,"tree_scale",(0.9,1.4))
    scatter("ForestTrees",fo["forest_band"][0],fo["forest_band"][1],fo["forest_ratio"],trees,ts[0],ts[1],prune=160)
    scatter("EdgeTrees",fo["edge_band"][0],fo["edge_band"][1],fo["edge_ratio"],trees,ts[0]*0.9,ts[1]*0.9,prune=200)
    scatter("TownTrees",fo["town_band"][0],fo["town_band"][1],fo["town_ratio"],gv(fo,"town_trees",trees),ts[0]*0.9,ts[1]*0.85,prune=300)
    scatter("Bushes",0.0,1.0,fo["bush_ratio"],[(MESH["bush1"],1,None),(MESH["bush2"],1,None)],0.7,1.2)
    scatter("Grass",0.0,1.0,fo["grass_ratio"],[(MESH["grass"],1,None)],0.9,1.6)
    scatter("Mushrooms",fo["forest_band"][0],1.0,gv(fo,"mush_ratio",0),[(MESH["mush1"],1,None),(MESH["mush2"],1,None),(MESH["mush3"],1,None)],0.8,1.3)
    for ex in gv(fo,"extra") or []:
        scatter(ex["name"],ex["band"][0],ex["band"][1],ex["ratio"],ex["entries"],ex["scale"][0],ex["scale"][1],prune=gv(ex,"prune"))
    save([g.split(".")[0]])
    info["nodes"]=len(B.nodes)
    return g,info

def _rot(yaw):
    if isinstance(yaw,(list,tuple)): return {"pitch":yaw[0],"yaw":yaw[1],"roll":yaw[2]}
    return {"pitch":0,"yaw":yaw,"roll":0}
def place_mesh(path,label,loc,yaw=0,scale=1.0,mats=None,tag=TOWNPROP,tags=(),props=None):
    s=scale if isinstance(scale,(list,tuple)) else (scale,scale,scale)
    a=T(SC+"add_to_scene_from_asset",{"asset_path":path,"name":label,"xform":{"location":{"x":loc[0],"y":loc[1],"z":loc[2] if len(loc)>2 else 0},"rotation":_rot(yaw),"scale":{"x":s[0],"y":s[1],"z":s[2]}}})
    ap=a["refPath"] if isinstance(a,dict) else a
    if mats:
        comps=T(AC+"get_components",{"actor":ref(ap),"component_type":ref("/Script/Engine.StaticMeshComponent")})
        for c in comps:
            T(OB+"set_properties",{"instance":ref(c["refPath"]),"values":json.dumps({"OverrideMaterials":[ref(m) if m else "None" for m in mats]})})
    if tag: T(AC+"add_tag",{"actor":ref(ap),"tag":tag})
    for t_ in tags: T(AC+"add_tag",{"actor":ref(ap),"tag":t_})
    if props:
        props=dict(props); extra_tags=props.pop("Tags",None) or []
        if props: T(OB+"set_properties",{"instance":ref(ap),"values":json.dumps(props)})
        for t_ in extra_tags: T(AC+"add_tag",{"actor":ref(ap),"tag":t_})  # Tags is an array: set_properties cannot grow it
    return ap
def place_class(cls,label,loc,yaw=0,scale=1.0,tag=TOWNPROP):
    s=scale if isinstance(scale,(list,tuple)) else (scale,scale,scale)
    a=T(SC+"add_to_scene_from_class",{"actor_type":ref(cls),"name":label,"xform":{"location":{"x":loc[0],"y":loc[1],"z":loc[2] if len(loc)>2 else 0},"rotation":_rot(yaw),"scale":{"x":s[0],"y":s[1],"z":s[2]}}})
    ap=a["refPath"] if isinstance(a,dict) else a
    if tag: T(AC+"add_tag",{"actor":ref(ap),"tag":tag})
    return ap
def move_actor(path,loc,yaw=None):
    xf={"location":{"x":loc[0],"y":loc[1],"z":loc[2]}}
    if yaw is not None: xf["rotation"]={"pitch":0,"yaw":yaw,"roll":0}
    return T(AC+"set_actor_transform",{"actor":ref(path),"xform":xf,"worldspace":True})
def find_by_label(label):
    return [a["refPath"] for a in (T(SC+"find_actors",{"name":label,"tag":"","collision_channels":[]}) or [])]
def find_by_tag(tag):
    return [a["refPath"] for a in (T(SC+"find_actors",{"name":"","tag":tag,"collision_channels":[]}) or [])]
def remove(path): return T(SC+"remove_from_scene",{"actor":ref(path)})
def clean_town(extra_labels=()):
    n=0
    for a in (T(SC+"find_actors",{"name":"","tag":"","collision_channels":[]}) or []):
        p=a["refPath"]; cls=p.split(".")[-1]
        if cls.startswith("PCG_") or cls.startswith("PCGVolume"): remove(p); n+=1
    for p in find_by_tag(TOWNPROP): remove(p); n+=1
    for lab in extra_labels:
        for p in find_by_label(lab): remove(p); n+=1
    return n
def ground_z(x,y):
    """Height of the topmost TownGround-ish collision at (x,y) via a world trace (falls back to 0)."""
    try:
        d=T(SC+"trace_world",{"start":{"x":x,"y":y,"z":4000},"end":{"x":x,"y":y,"z":-4000}})
        return 4000-float(d) if d is not None else 0
    except Exception: return 0
def spawn_and_run(graph,label,extent):
    inst=T(PC+"SpawnGraphInstance",{"graph":ref(graph),"name":label,"transform":{"location":{"x":0,"y":0,"z":0},"rotation":{"pitch":0,"yaw":0,"roll":0},"scale":{"x":extent/100.0,"y":extent/100.0,"z":10}},"jsonParams":""})
    vol=gv(gv(inst,"actor") or {},"refPath")
    msgs=T(PC+"ExecuteGraphInstance",{"pCGVolume":ref(vol)}) if vol else "no volume"
    return vol,msgs
def build_level(spec,extra_clean=()):
    out={"cleaned":clean_town(extra_clean)}
    g,info=build_town_graph(spec); out["info"]=info
    placed=0; R=gv(spec,"_relief")
    def lift(loc,absolute=False):
        if R is None or absolute: return loc
        return (loc[0],loc[1],R.zat(loc[0],loc[1])+(loc[2] if len(loc)>2 else 0))
    for item in gv(spec,"props") or []:
        absolute=len(item)>7 and item[7]=="abs"
        if item[0]=="mesh":
            path,label,loc,yaw,scale,mats=item[1:7]; place_mesh(path,label,lift(loc,absolute),yaw,scale,mats); placed+=1
        elif item[0]=="ground":
            if R is not None: continue  # the relief terrain is the ground; a flat plane underneath only confuses the navmesh
            path,label,loc,yaw,scale,mats=item[1:7]; place_mesh(path,label,loc,yaw,scale,mats,tags=(GROUND_TAG,)); placed+=1
        elif item[0]=="class":
            cls,label,loc,yaw,scale=item[1:6]; ap=place_class(cls,label,lift(loc,absolute),yaw,scale); placed+=1
            if len(item)>6 and item[6]: T(OB+"set_properties",{"instance":ref(ap),"values":json.dumps(item[6])})
        elif item[0]=="actor":
            path,label,loc,yaw,scale,props=item[1:7]; place_mesh(path,label,lift(loc,absolute),yaw,scale,None,props=props); placed+=1
        elif item[0]=="npc":
            label,loc,yaw,props=item[1:5]; cls=item[5] if len(item)>5 and item[5] else "/Game/Wot/NPCs/NPC_Villager_BP"
            z=(R.zat(loc[0],loc[1]) if R is not None else ground_z(loc[0],loc[1]))+96
            place_mesh(cls,label,(loc[0],loc[1],z),yaw,1.0,None,props=props); placed+=1
    out["placed"]=placed
    vol,msgs=spawn_and_run(g,spec["graph_name"],spec["foliage"]["extent"]*2); out["vol"]=vol; out["msgs"]=msgs
    dl=gv(spec,"dialogue")
    if dl:
        DT="editor_toolset.toolsets.data_table.DataTableTools."; table=ref("/Game/Wot/DataTables/DT_HybridRPG_Dialogue.DT_HybridRPG_Dialogue")
        existing=set(T(DT+"list_rows",{"data_table":table}))
        new=[k for k in dl if k not in existing]
        if new: T(DT+"add_rows",{"data_table":table,"row_names":new})
        T(DT+"set_rows",{"data_table":table,"values":json.dumps({k:{"dialogueText":v[0],"nextRow":v[1]} for k,v in dl.items()})})
        existing|=set(dl.keys())
        missing=[it[1] for it in (gv(spec,"props") or []) if it[0]=="npc" and it[4].get("CurrentRowName") and it[4]["CurrentRowName"] not in existing]
        if missing: out["dialogue_missing"]=missing   # an NPC whose first row is not in the table would lock the player with nothing to say
        save(["/Game/Wot/DataTables/DT_HybridRPG_Dialogue"]); out["dialogue_rows"]=len(dl)
    for (label,loc,yaw) in gv(spec,"moves") or []:
        for p in find_by_label(label): move_actor(p,loc,yaw)
    town=gv(spec,"town")
    if town:
        for p in find_by_label("OverworldPortal"):
            T(OB+"set_properties",{"instance":ref(p),"values":json.dumps({"ToLevel":"Overworld","PortalName":f"{town}Overworld","Enabled":True})})
        for p in find_by_label("OverworldPlayerStart"):
            T(OB+"set_properties",{"instance":ref(p),"values":json.dumps({"PlayerStartTag":f"Overworld{town}"})})
        out["portal_wired"]=True
    ls=gv(spec,"level_scale")
    if ls:
        for lab in ("NavMeshBounds","BP_LevelBounds"):
            for p in find_by_label(lab):
                T(AC+"set_actor_transform",{"actor":ref(p),"xform":{"location":{"x":0,"y":0,"z":500 if lab=="NavMeshBounds" else 0},"scale":{"x":ls[0],"y":ls[1],"z":ls[2]*3}},"worldspace":True})
    return out
