"""The Ways: a procedural level of dark stone islands floating in a void, linked by decayed bridges, lit only by
Guidings and the Waygates, and haunted by Machin Shin. Runs inside the editor's ProgrammaticToolset together with
the town tooling (pcg_lib / town_builder / kit_builder are concatenated in front of this file by run_ways.py).

Design follows the Amazon series: islands of once-holy stone "touched by a terminal illness", bridges without
railings, tall pale Guidings carved with Ogier script, oppressive darkness, the Black Wind as a murmuration of stones.
"""
import math

WAYS_FOLDER="/Game/Wot/Maps/Ways"; WAYS_LEVEL=WAYS_FOLDER+"/Ways"; TEMPLATE="/Game/Wot/Maps/RandHouse/RandHouse"
MATF="/Game/Wot/Towns/Common/Materials"
ISLANDS={"SM_WayIsland":1400,"SM_WayIsland2":1100,"SM_WayIsland3":1700}
BRIDGE_LEN=1200.0; BRIDGE_W=500.0
WAY_TAG="WaysProp"

def way_materials():
    """Palette for the Ways (linear colours) and the emissive Guiding script."""
    MI="editor_toolset.toolsets.material_instance.MaterialInstanceTools."
    def lin(c): return [v**2.2 for v in c]
    def mi(name,parent,color,emissive=None,strength=None):
        path=f"{MATF}/{name}"
        if not T(AS+"exists",{"path":path}): T(MI+"create",{"folder_path":MATF,"asset_name":name,"parent":ref(parent)})
        c=lin(color); T(MI+"set_vector_parameter",{"instance":ref(path+"."+name),"name":"Color","value":{"r":c[0],"g":c[1],"b":c[2],"a":1}})
        if emissive:
            e=lin(emissive)
            try: T(MI+"set_vector_parameter",{"instance":ref(path+"."+name),"name":"Emissive Color","value":{"r":e[0],"g":e[1],"b":e[2],"a":1}})
            except Exception: pass
            try: T(MI+"set_scalar_parameter",{"instance":ref(path+"."+name),"name":"Emissive Strength","value":strength or 4.0})
            except Exception: pass
        return path
    STONE="/Game/Wot/Materials/MI_VoxelStone.MI_VoxelStone"; FIRE="/Game/Wot/Materials/MI_VoxelFire.MI_VoxelFire"
    made=[mi("P_WayStone",STONE,(0.21,0.20,0.23)),mi("P_WayStoneDark",STONE,(0.09,0.085,0.10)),mi("P_GuidingPale",STONE,(0.74,0.71,0.64)),
          mi("P_GuidingScript",FIRE,(0.55,0.75,0.95),(0.55,0.75,1.0),6.0)]
    save(made); return made

def setp_soft(obj,values):
    """set_properties one key at a time so a rejected property does not abort the build."""
    for k,v in values.items():
        try: T(OB+"set_properties",{"instance":ref(obj),"values":json.dumps({k:v})})
        except Exception: pass

RIM={k:v*1.05+80 for k,v in ISLANDS.items()}          # outermost rim voxel an island of that kind can have
LAND_IN=lambda kind: ISLANDS[kind]*0.86-260              # bridge landings start this far inside the island (under the crumbled band)
MAX_SLOPE=0.25                                            # rise/run of the pitched middle span (about 14 degrees)
def island_layout(seed,count):
    """A 3-D random walk of island centres. The kind (size) is chosen with the island so the gap between two rims is
    long enough for the climb between them at MAX_SLOPE; z changes by whole steps so bridges stay gentle."""
    r=WotRandom(seed); pts=[(0.0,0.0,0.0)]; kinds=["SM_WayIsland"]; heading=r.uniform(0,360); tries=0
    while len(pts)<count and tries<300:
        tries+=1
        kind=r.choice(list(ISLANDS.keys())) if len(pts)<count-1 else "SM_WayIsland"
        dzc=r.choice((0,0,120,-120,200,-200))
        heading+=r.uniform(-70,70)
        d=RIM[kinds[-1]]+RIM[kind]+700+abs(dzc)/MAX_SLOPE+r.uniform(0,900)
        x=pts[-1][0]+math.cos(math.radians(heading))*d; y=pts[-1][1]+math.sin(math.radians(heading))*d; z=pts[-1][2]+dzc
        # keep islands from overlapping earlier ones
        if any(math.hypot(x-px,y-py)<RIM[kinds[j]]+RIM[kind]+400 for j,(px,py,pz) in enumerate(pts[:-1])): heading+=120; continue
        pts.append((x,y,z)); kinds.append(kind)
    return pts,kinds,r

def build_ways(seed=11,count=9):
    """Creates /Game/Wot/Maps/Ways from the RandHouse template and fills it."""
    # 1. level from template
    if not T(AS+"exists",{"path":WAYS_LEVEL}):
        T(AS+"create_folder",{"path":WAYS_FOLDER}); T(AS+"duplicate",{"path":TEMPLATE,"new_path":WAYS_LEVEL}); save([WAYS_LEVEL])
    try:
        if T(AS+"is_dirty",{"asset_path":WAYS_LEVEL}): save([WAYS_LEVEL])
    except Exception: pass
    cur=T(SC+"get_current_level",{})
    if cur!=WAYS_LEVEL+"."+"Ways" and cur!=WAYS_LEVEL:
        if T(AS+"is_dirty",{"asset_path":cur}): save([cur])
        T(SC+"load_level",{"level_path":WAYS_LEVEL})
    # 2. clear the template's contents (keep nothing but the world settings)
    removed=0
    for a in (T(SC+"find_actors",{"name":"","tag":"","collision_channels":[]}) or []):
        p=a["refPath"]; cls=p.split(".")[-1]
        if cls.startswith(("WorldSettings","Brush","DefaultPhysicsVolume","AbstractNavData","GameplayDebugger")): continue
        try: T(SC+"remove_from_scene",{"actor":ref(p)}); removed+=1
        except Exception: pass
    ws=f"{WAYS_LEVEL}.Ways:PersistentLevel.WorldSettings"
    try: T(OB+"set_properties",{"instance":ref(ws),"values":json.dumps({"KillZ":-6000.0,"DefaultGameMode":ref("/Game/Wot/MediumFlatGameModeBP.MediumFlatGameModeBP_C")})})
    except Exception: pass
    # 3. islands, bridges, guidings, props
    pts,kinds,r=island_layout(seed,count)
    # bridges between consecutive islands (and one or two extra loops)
    links=[(i,i+1) for i in range(len(pts)-1)]
    for _ in range(2):
        a=r.randrange(len(pts)); b=r.randrange(len(pts))
        dist=math.hypot(pts[a][0]-pts[b][0],pts[a][1]-pts[b][1]); run=dist-RIM[kinds[a]]-RIM[kinds[b]]
        if abs(a-b)>=2 and dist<5600 and run>=700+abs(pts[a][2]-pts[b][2])/MAX_SLOPE and (min(a,b),max(a,b)) not in links: links.append((min(a,b),max(a,b)))
    def clear_of_bridges(i,px,py):
        """True when (px,py) is not within 460 cm of a bridge centreline leaving island i (landings stay walkable)."""
        (ix,iy,_)=pts[i]
        for (a,b) in links:
            if i not in (a,b): continue
            (ox,oy,_)=pts[b if a==i else a]; dx,dy=ox-ix,oy-iy; L=math.hypot(dx,dy); ux,uy=dx/L,dy/L
            along=(px-ix)*ux+(py-iy)*uy; lateral=abs((px-ix)*uy-(py-iy)*ux)
            if along>0 and lateral<460: return False
        return True
    def spot(i,x,y,dmin,dmax,rad):
        for _ in range(12):
            pa=r.uniform(0,360); pd=rad*r.uniform(dmin,dmax); px=x+math.cos(math.radians(pa))*pd; py=y+math.sin(math.radians(pa))*pd
            if clear_of_bridges(i,px,py): return px,py
        return px,py
    placed=0
    for i,((x,y,z),kind) in enumerate(zip(pts,kinds)):
        place_mesh(kit_mesh(kind),f"Island_{i}",(x,y,z),r.uniform(0,360),1.0,None,tag=WAY_TAG); placed+=1
        rad=ISLANDS[kind]
        gx,gy=spot(i,x,y,0.4,0.5,rad)
        place_mesh(kit_mesh("SM_Guiding"),f"Guiding_{i}",(gx,gy,z),r.uniform(0,360),1.0,None,tag=WAY_TAG); placed+=1
        ap=place_class("/Script/Engine.PointLight",f"GuidingLight_{i}",(gx,gy,z+420),0,1.0,tag=WAY_TAG)
        for c in T(AC+"get_components",{"actor":ref(ap),"component_type":ref("/Script/Engine.PointLightComponent")}) or []:
            setp_soft(c["refPath"],{"Intensity":2600.0,"AttenuationRadius":1500.0,"LightColor":{"r":150,"g":200,"b":255,"a":255},"CastShadows":False})
        for k in range(r.randint(1,2)):
            px,py=spot(i,x,y,0.3,0.7,rad)
            place_mesh(kit_mesh(r.choice(("SM_WayPillar","SM_WayPillar2"))),f"Pillar_{i}_{k}",(px,py,z),r.uniform(0,360),1.0,None,tag=WAY_TAG); placed+=1
        for k in range(r.randint(2,4)):
            px,py=spot(i,x,y,0.2,0.75,rad)
            place_mesh(kit_mesh("SM_WayRubble"),f"Rubble_{i}_{k}",(px,py,z),r.uniform(0,360),r.uniform(0.7,1.3),None,tag=WAY_TAG); placed+=1
    bridges=0
    def span(a,b):
        """(start, end, z0, z1) pieces along the a->b line, distances measured from island a's centre."""
        (ax,ay,az),(bx,by,bz)=pts[a],pts[b]; L=math.hypot(bx-ax,by-ay)
        s0,s1=LAND_IN(kinds[a]),RIM[kinds[a]]; e1,e0=L-RIM[kinds[b]],L-LAND_IN(kinds[b])
        if e1<=s1: s1=e1=(s1+e1)/2                       # rims touch: no pitched piece, the landings meet
        return [(s0,s1,az,az),(s1,e1,az,bz),(e1,e0,bz,bz)]
    for (a,b) in links:
        (ax,ay,az),(bx,by,bz)=pts[a],pts[b]; dx,dy=bx-ax,by-ay; L=math.hypot(dx,dy); ux,uy=dx/L,dy/L
        yaw=math.degrees(math.atan2(uy,ux))
        for (s,e,z0,z1) in span(a,b):
            run=e-s; dz=z1-z0
            if run<40: continue
            length=math.hypot(run,dz); n=max(1,int(round(length/BRIDGE_LEN))); seg=length/n
            pitch=math.degrees(math.atan2(dz,run))
            for k in range(n):
                px=ax+ux*(s+run*k/n); py=ay+uy*(s+run*k/n); pz=z0+dz*k/n+3   # 3 cm above the island top: no z-fighting on the landings
                place_mesh(kit_mesh(r.choice(("SM_WayBridge","SM_WayBridge2"))),f"Bridge_{a}_{b}_{bridges}",(px,py,pz),(pitch,yaw,0),(seg/BRIDGE_LEN,1.0,1.0),None,tag=WAY_TAG); bridges+=1
    # 4. the Waygates: entry on the first island (from Shadar Logoth), exit on the last (to Fal Dara)
    def gate(i,label,to_level,portal_name,start_tag):
        x,y,z=pts[i]; j=i+1 if i==0 else i-1; ang=math.degrees(math.atan2(pts[j][1]-y,pts[j][0]-x))+180   # gate faces away from the bridge
        gx=x+math.cos(math.radians(ang))*ISLANDS[kinds[i]]*0.5; gy=y+math.sin(math.radians(ang))*ISLANDS[kinds[i]]*0.5
        fx=-math.cos(math.radians(ang)); fy=-math.sin(math.radians(ang))        # the opening looks toward the island centre
        place_mesh(kit_mesh("SM_Waygate"),f"Waygate_{label}",(gx,gy,z),ang+180,1.0,None,tag=WAY_TAG)
        place_mesh(PORTAL_BP,f"WaysExit_{label}",(gx+fx*130,gy+fy*130,z+20),ang+180,1.0,None,tag=WAY_TAG,props={"ToLevel":to_level,"PortalName":portal_name,"Enabled":True})
        ap=place_class("/Script/Engine.PlayerStart",f"WaysStart_{label}",(gx+fx*380,gy+fy*380,z+95),ang,1.0,tag=WAY_TAG)
        T(OB+"set_properties",{"instance":ref(ap),"values":json.dumps({"PlayerStartTag":start_tag})})
        ap=place_class("/Script/Engine.PointLight",f"GateLight_{label}",(gx+fx*200,gy+fy*200,z+380),0,1.0,tag=WAY_TAG)
        for c in T(AC+"get_components",{"actor":ref(ap),"component_type":ref("/Script/Engine.PointLightComponent")}) or []:
            setp_soft(c["refPath"],{"Intensity":4000.0,"AttenuationRadius":1800.0,"LightColor":{"r":255,"g":190,"b":120,"a":255}})
    gate(0,"ShadarLogoth","ShadarLogoth","WaysShadarLogoth","ShadarLogothWays")
    gate(len(pts)-1,"FalDara","FalDara","WaysFalDara","FalDaraWays")
    # 5. darkness: black fog that swallows light within a few metres, no sky
    fog=place_class("/Script/Engine.ExponentialHeightFog","WaysFog",(0,0,0),0,1.0,tag=WAY_TAG)
    for c in T(AC+"get_components",{"actor":ref(fog),"component_type":ref("/Script/Engine.ExponentialHeightFogComponent")}) or []:
        setp_soft(c["refPath"],{"FogDensity":0.9,"FogHeightFalloff":0.0001,"FogInscatteringLuminance":{"r":0.004,"g":0.004,"b":0.006,"a":1},"FogMaxOpacity":1.0,"StartDistance":300.0,"FogCutoffDistance":9000.0})
    sky=place_class("/Script/Engine.SkyLight","WaysSkyLight",(0,0,2000),0,1.0,tag=WAY_TAG)
    for c in T(AC+"get_components",{"actor":ref(sky),"component_type":ref("/Script/Engine.SkyLightComponent")}) or []:
        setp_soft(c["refPath"],{"Intensity":0.08,"LightColor":{"r":120,"g":140,"b":200,"a":255}})
    # fixed exposure: auto-exposure would adapt to the black void and white out every lit surface
    ppv=place_class("/Script/Engine.PostProcessVolume","WaysPostProcess",(0,0,0),0,1.0,tag=WAY_TAG)
    setp_soft(ppv,{"bUnbound":True,"bEnabled":True})
    setp_soft(ppv,{"Settings":{"bOverride_AutoExposureMethod":True,"AutoExposureMethod":"AEM_Histogram","bOverride_AutoExposureMinBrightness":True,"AutoExposureMinBrightness":3.0,
                               "bOverride_AutoExposureMaxBrightness":True,"AutoExposureMaxBrightness":3.0,"bOverride_AutoExposureBias":True,"AutoExposureBias":0.0,
                               "bOverride_VignetteIntensity":True,"VignetteIntensity":0.8,"bOverride_SceneColorTint":True,"SceneColorTint":{"r":0.85,"g":0.9,"b":1.0,"a":1.0},
                               "bOverride_BloomIntensity":True,"BloomIntensity":0.9}})
    # 6. Machin Shin patrols the islands
    mid=len(pts)//2
    ms=place_class("/Script/VoxelRPG.WotMachinShin","MachinShin",(pts[mid][0],pts[mid][1],pts[mid][2]+200),0,1.0,tag=WAY_TAG)
    T(OB+"set_properties",{"instance":ref(ms),"values":json.dumps({"PatrolPoints":[{"x":p[0],"y":p[1],"z":p[2]+160} for p in pts]})})
    # 7. nav + bounds around everything
    xs=[p[0] for p in pts]; ys=[p[1] for p in pts]; zs=[p[2] for p in pts]
    cx=(min(xs)+max(xs))/2; cy=(min(ys)+max(ys))/2; ex=(max(xs)-min(xs))/2+2500; ey=(max(ys)-min(ys))/2+2500
    nav=place_class("/Script/NavigationSystem.NavMeshBoundsVolume","NavMeshBounds",(cx,cy,(min(zs)+max(zs))/2),0,(ex/100.0,ey/100.0,(max(zs)-min(zs)+2000)/100.0),tag=WAY_TAG)
    lb=place_mesh("/Game/Wot/Actors/BP_LevelBounds","BP_LevelBounds",(cx,cy,0),0,(ex/100.0+10,ey/100.0+10,60),None,tag=WAY_TAG)
    save([WAYS_LEVEL])
    return {"islands":len(pts),"bridges":bridges,"props":placed,"removed":removed,"centres":pts}
