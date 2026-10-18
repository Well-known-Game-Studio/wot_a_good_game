# Shared PCG subgraphs under /Game/Wot/Towns/Common/PCG, built once by `run_town.py --subgraphs` and used by every
# town graph (town_builder). Fixing or improving one of these improves all eleven towns on their next rebuild.
# Per-use parameters are passed as overrides on the subgraph node (G.sub(..., params={...})).
V=lambda x,y,z:{"x":x,"y":y,"z":z}
R=lambda p,y,r:{"pitch":p,"yaw":y,"roll":r}
def build_subgraphs():
    out={}
    # --- plaza: closed outline points -> spline -> surface (an exclusion shape) and interior tile points
    B=SubG("PCGS_Plaza","Plaza from an outline: Surface = the closed spline's surface (use as an exclusion), Tiles = interior points every 100 cm, flattened to 6 cm slabs. Param bLinear: straight edges (rectangles) or a smooth loop (circles).")
    B.inputs("Outline"); B.outputs("Surface","Tiles")
    B.param("bLinear","Boolean",True,x=0,y=1)
    B.node("Create Spline","Spline",{"bClosedLoop":True,"bLinear":True},x=1,y=0); B.bind("bLinear","Spline")
    B.node("Create Surface From Spline","Surface",{},x=2,y=0)
    B.node("Spline Sampler","Interior",{"samplerParams":{"dimension":"OnInterior","interiorSampleSpacing":100,"interiorBorderSampleSpacing":100,"bUnbounded":True,"bTreatSplineAsPolyline":True}},x=2,y=1)
    B.node("Transform Points","Slab",{"scaleMin":V(1,1,0.06),"scaleMax":V(1,1,0.06),"bAbsoluteScale":True},title="6 cm slabs",x=3,y=1)
    B.link("Input","Outline","Spline","In"); B.link("Spline","Out","Surface","In"); B.link("Spline","Out","Interior","Spline"); B.link("Interior","Out","Slab","In")
    B.link("Surface","Out","Output","Surface"); B.link("Slab","Out","Output","Tiles"); out["Plaza"]=B.done()
    # --- road: path points + one tile row -> spline -> samples every DistanceIncrement -> the row copied onto each sample
    B=SubG("PCGS_Road","Road tiles: Path points become a spline (bClosedLoop / bLinear), sampled every DistanceIncrement cm; the Row points (one tile per lane) are copied onto every sample. KeepRatio < 1 leaves gaps. Outputs: Tiles, Spline, Samples.")
    B.inputs("Path","Row"); B.outputs("Tiles","Spline","Samples")
    B.param("bClosedLoop","Boolean",False,x=0,y=1); B.param("bLinear","Boolean",False,x=0,y=2); B.param("DistanceIncrement","Double",100.0,x=1,y=2); B.param("KeepRatio","Double",1.0,attr="Ratio",x=3,y=2)
    B.node("Create Spline","Spline",{"bClosedLoop":False,"bLinear":False},x=1,y=0); B.bind("bClosedLoop","Spline"); B.bind("bLinear","Spline")
    B.node("Spline Sampler","Samples",{"samplerParams":{"dimension":"OnSpline","mode":"Distance","distanceIncrement":100,"bUnbounded":True}},x=2,y=0); B.bind("DistanceIncrement","Samples")
    B.node("Copy Points","Tiles",{"rotationInheritance":"Source","bApplyTargetRotationToPositions":True,"scaleInheritance":"Source","bApplyTargetScaleToPositions":False,"attributeInheritance":"TargetFirst"},x=3,y=0)
    B.node("Random Choice","Keep",{"bFixedMode":False,"ratio":1.0},title="gaps",x=4,y=0); B.bind("KeepRatio","Keep")
    B.link("Input","Path","Spline","In"); B.link("Spline","Out","Samples","Spline"); B.link("Input","Row","Tiles","Source"); B.link("Samples","Out","Tiles","Target"); B.link("Tiles","Out","Keep","In")
    B.link("Keep","Chosen","Output","Tiles"); B.link("Spline","Out","Output","Spline"); B.link("Samples","Out","Output","Samples"); out["Road"]=B.done()
    # --- houses: slots -> jitter + scale -> keep ratio -> core must not touch an exclusion -> footprint -> no overlaps
    B=SubG("PCGS_Houses","Houses on slots: jitter (OffsetMin/Max, RotationMin/Max) and scale (ScaleMin/Max, uniform), keep Ratio of them, drop any whose core (CoreMin/CoreMax) touches an exclusion (roads, plazas, walls, earlier houses), give the rest their footprint (FootMin/FootMax) and prune overlaps. Output points carry the footprint bounds, so they can be used as exclusions.")
    B.inputs("Slots","Exclusions"); B.outputs("Houses")
    B.param("OffsetMin","Vector",V(-120,-40,0),x=0,y=1); B.param("OffsetMax","Vector",V(120,40,0),x=0,y=2); B.param("RotationMin","Rotator",R(0,-4,0),x=0,y=3); B.param("RotationMax","Rotator",R(0,4,0),x=0,y=4)
    B.param("ScaleMin","Vector",V(0.7,0.7,0.7),x=0,y=5); B.param("ScaleMax","Vector",V(0.9,0.9,0.9),x=0,y=6); B.param("Ratio","Double",0.9,x=2,y=2)
    B.param("CoreMin","Vector",V(-407,-231,0),attr="BoundsMin",x=3,y=2); B.param("CoreMax","Vector",V(308,308,700),attr="BoundsMax",x=3,y=3)
    B.param("FootMin","Vector",V(-740,-420,0),attr="BoundsMin",x=5,y=2); B.param("FootMax","Vector",V(560,560,700),attr="BoundsMax",x=5,y=3)
    B.node("Transform Points","Jitter",{"offsetMin":V(-120,-40,0),"offsetMax":V(120,40,0),"rotationMin":R(0,-4,0),"rotationMax":R(0,4,0),"scaleMin":V(0.7,0.7,0.7),"scaleMax":V(0.9,0.9,0.9),"bUniformScale":True,"bRecomputeSeed":True},x=1,y=0)
    for p in ("OffsetMin","OffsetMax","RotationMin","RotationMax","ScaleMin","ScaleMax"): B.bind(p,"Jitter")
    B.node("Random Choice","Keep",{"bFixedMode":False,"ratio":0.9},x=2,y=0); B.bind("Ratio","Keep")
    B.node("Bounds Modifier","Core",{"mode":"Set","boundsMin":V(-407,-231,0),"boundsMax":V(308,308,700)},title="core (exclusion test)",x=3,y=0); B.bind("CoreMin","Core"); B.bind("CoreMax","Core")
    B.node("Difference","Clear",{"densityFunction":"Binary","mode":"Inferred"},title="not on roads/plazas/walls/houses",x=4,y=0)
    B.node("Bounds Modifier","Foot",{"mode":"Set","boundsMin":V(-740,-420,0),"boundsMax":V(560,560,700)},title="footprint",x=5,y=0); B.bind("FootMin","Foot"); B.bind("FootMax","Foot")
    B.node("Self Pruning","Prune",{"parameters":{"pruningType":"LargeToSmall","bRandomizedPruning":True}},x=6,y=0)
    B.chain(("Input","Slots","In"),("Jitter","Out","In"),("Keep","Chosen","In"),("Core","Out","In"),("Clear","Out","Source"),("Foot","Out","In"),("Prune","Out","In"),("Output","Houses","Houses"))
    B.link("Input","Exclusions","Clear","Differences"); out["Houses"]=B.done()
    # --- lattice: ground points -> jitter -> density = distance from the town centre -> minus exclusions
    B=SubG("PCGS_Lattice","Ground lattice: jitters the terrain points (OffsetMin/Max), sets density = distance from Center over MaximumDistance (0 at the centre, 1 at the edge), and subtracts the exclusions (roads, plazas, walls, houses, hand-placed). Feed the result to PCGS_Scatter bands.")
    B.inputs("Lattice","Center","Exclusions"); B.outputs("Ground")
    B.param("OffsetMin","Vector",V(-110,-110,0),x=0,y=1); B.param("OffsetMax","Vector",V(110,110,0),x=0,y=2); B.param("MaximumDistance","Double",6600.0,x=1,y=2)
    B.node("Transform Points","Jitter",{"offsetMin":V(-110,-110,0),"offsetMax":V(110,110,0),"bRecomputeSeed":True},x=1,y=0); B.bind("OffsetMin","Jitter"); B.bind("OffsetMax","Jitter")
    B.node("Distance","Dist",{"bSetDensity":True,"maximumDistance":6600,"sourceShape":"Center","targetShape":"Center","bOutputToAttribute":True,"outputAttribute":"Dist"},title="density = distance from centre",x=2,y=0); B.bind("MaximumDistance","Dist")
    B.node("Difference","Excl",{"densityFunction":"Minimum","mode":"Inferred"},title="minus roads/houses/plazas",x=3,y=0)
    B.link("Input","Lattice","Jitter","In"); B.link("Jitter","Out","Dist","Source"); B.link("Input","Center","Dist","Target"); B.link("Dist","Out","Excl","Source"); B.link("Input","Exclusions","Excl","Differences"); B.link("Excl","Out","Output","Ground")
    out["Lattice"]=B.done()
    # --- scatter: a density band of the ground lattice -> keep Ratio -> random yaw + uniform scale -> prune by radius
    B=SubG("PCGS_Scatter","One scatter band: keeps ground points whose density lies in LowerBound..UpperBound, then Ratio of those, gives each a random yaw (RotationMax) and uniform scale (ScaleMin..ScaleMax), sets their bounds (BoundsMin/Max = the prune radius) and prunes overlaps. Spawn the output with a Static Mesh Spawner in the town graph.")
    B.inputs("Ground"); B.outputs("Points")
    B.param("LowerBound","Double",0.0,x=0,y=1); B.param("UpperBound","Double",1.0,x=0,y=2); B.param("Ratio","Double",0.1,x=1,y=2)
    B.param("RotationMax","Rotator",R(0,360,0),x=2,y=2); B.param("ScaleMin","Vector",V(0.9,0.9,0.9),x=2,y=3); B.param("ScaleMax","Vector",V(1.4,1.4,1.4),x=2,y=4)
    B.param("BoundsMin","Vector",V(-50,-50,0),x=3,y=2); B.param("BoundsMax","Vector",V(50,50,50),x=3,y=3)
    B.node("Density Filter","Band",{"lowerBound":0.0,"upperBound":1.0},x=1,y=0); B.bind("LowerBound","Band"); B.bind("UpperBound","Band")
    B.node("Random Choice","Pick",{"bFixedMode":False,"ratio":0.1},x=2,y=0); B.bind("Ratio","Pick")
    B.node("Transform Points","Xform",{"rotationMin":R(0,0,0),"rotationMax":R(0,360,0),"scaleMin":V(0.9,0.9,0.9),"scaleMax":V(1.4,1.4,1.4),"bUniformScale":True,"bRecomputeSeed":True},x=3,y=0)
    for p in ("RotationMax","ScaleMin","ScaleMax"): B.bind(p,"Xform")
    B.node("Bounds Modifier","Bnd",{"mode":"Set","boundsMin":V(-50,-50,0),"boundsMax":V(50,50,50)},title="prune radius",x=4,y=0); B.bind("BoundsMin","Bnd"); B.bind("BoundsMax","Bnd")
    B.node("Self Pruning","Prune",{"parameters":{"pruningType":"LargeToSmall","bRandomizedPruning":True}},x=5,y=0)
    B.chain(("Input","Ground","In"),("Band","Out","In"),("Pick","Chosen","In"),("Xform","Out","In"),("Bnd","Out","In"),("Prune","Out","In"),("Output","Points","Points"))
    out["Scatter"]=B.done()
    return out
