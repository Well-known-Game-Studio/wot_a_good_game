import json, re
PC="PCGToolset.PCGToolset."; OB="editor_toolset.toolsets.object.ObjectTools."; SC="editor_toolset.toolsets.scene.SceneTools."; AS="editor_toolset.toolsets.asset.AssetTools."; AC="editor_toolset.toolsets.actor.ActorTools."
def gv(d,k,default=None):
    try:
        return d[k] if (isinstance(d,dict) and k in d) else default
    except Exception:
        return default
def T(name,args):
    r=execute_tool(name, json.dumps(args))
    v=gv(r,"returnValue",r) if isinstance(r,dict) else r
    if isinstance(v,str):
        try: v=json.loads(v)
        except Exception: pass
    return v
def ref(p): return {"refPath":p}
def create_graph(name,folder):
    if T(AS+"exists",{"path":f"{folder}/{name}"}):
        T(AS+"delete",{"path":f"{folder}/{name}"})
    return T(PC+"CreateGraph",{"name":name,"path":folder})["refPath"]
NOCOLLISION_MESH=re.compile(r"grass|flower|shrub|reed|mush|snowpatch|fern|weed|moss|pebble|ivy|vine",re.I)
def mesh_blocks(mesh_path):
    """True when instances of this mesh should block the player."""
    return not NOCOLLISION_MESH.search(mesh_path.split("/")[-1])
WALKABLE_MESH=re.compile(r"BasicShapes/Cube|Stairs|Bridge|Ladder|Tile|Plaza|Dock|Pier",re.I)
def mesh_walkable(mesh_path):
    """True when the navmesh should be generated on top of this mesh (terrain cubes, road tiles, stairs, bridges)."""
    return bool(WALKABLE_MESH.search(mesh_path))
def collision_descriptor(mesh_path):
    """Descriptor fields that give PCG-spawned instances collision and navigation.
    PCG defaults every instance to NoCollision and bForceNavigationObstacle, which
    leaves the player falling through terrain and the navmesh empty."""
    if not mesh_blocks(mesh_path): return {"bUseDefaultCollision":False,"bCanEverAffectNavigation":False,"bForceNavigationObstacle":False,"bodyInstance":{"collisionProfileName":"NoCollision","collisionEnabled":"NoCollision"}}
    # export real geometry (bForceNavigationObstacle only paints a high-cost area and skips the geometry,
    # which left navmesh inside every house); walkable meshes get navmesh on top, buildings carve it
    return {"bUseDefaultCollision":False,"bCanEverAffectNavigation":True,"bForceNavigationObstacle":False,
            "bodyInstance":{"collisionProfileName":"BlockAll","collisionEnabled":"QueryAndPhysics","objectType":"ECC_WorldStatic"}}

class G:
    def __init__(self,graph): self.g=graph; self.nodes={}; self.col=0
    def node(self,ntype,name,params=None,title="",x=None,y=0,comment=""):
        a={"graph":ref(self.g),"nativeNodeType":ntype,"nodeName":name,"jsonParams":json.dumps(params or {}),"nodeTitle":title or name,"nodeComment":comment}
        if x is None: x=self.col; self.col+=1
        a["xPositionIdx"]=x; a["yPositionIdx"]=y
        r=T(PC+"AddNode",a); self.nodes[name]=r["refPath"]; return r["refPath"]
    def sub(self,subgraph,name,params=None,title="",x=None,y=0):
        a={"graph":ref(self.g),"subGraphForNode":ref(subgraph),"nodeName":name,"jsonParams":json.dumps(params or {}),"nodeTitle":title or name,"nodeComment":""}
        if x is None: x=self.col; self.col+=1
        a["xPositionIdx"]=x; a["yPositionIdx"]=y
        r=T(PC+"AddSubgraphNode",a); self.nodes[name]=r["refPath"]; return r["refPath"]
    def link(self,a,ap,b,bp):
        return T(PC+"ConnectNodePins",{"fromNode":ref(self.nodes[a]),"fromPinLabel":ap,"toNode":ref(self.nodes[b]),"toPinLabel":bp})
    def chain(self,*steps):
        # steps: (nodeName, outPin, inPin) sequential: link step[i].out -> step[i+1].in
        for i in range(len(steps)-1):
            a,ap,_=steps[i]; b,_,bp=steps[i+1]; self.link(a,ap,b,bp)
    def update(self,name,params): return T(PC+"UpdateNode",{"node":ref(self.nodes[name]),"jsonParams":json.dumps(params),"nodeTitle":""})
    def info(self,name): return T(PC+"GetNodeInfo",{"node":ref(self.nodes[name])})
    def selector_path(self,name):
        inf=self.info(name)
        po=gv(inf,"paramOverrides") or gv(inf,"params") or {}
        if isinstance(po,str):
            try: po=json.loads(po)
            except Exception: po={}
        sel=gv(gv(po,"meshSelectorParameters") or {},"refPath")
        if not sel:
            sel=self.nodes[name]+".PCGStaticMeshSpawnerSettings_0.DefaultSelectorInstance"
        return sel
    def set_meshes(self,name,entries):
        """entries: list of (meshPath, weight, overrideMaterials list or None).
        PCG spawner descriptors default to NoCollision; everything except soft
        ground cover gets BlockAll so the player can stand on terrain and bump
        into buildings (see mesh_blocks)."""
        sel=self.selector_path(name)
        me=[]
        for m,w,mats in entries:
            d={"staticMesh":ref(m)}
            if mats: d["overrideMaterials"]=[ref(x) if x else "None" for x in mats]
            d.update(collision_descriptor(m))
            me.append({"weight":w,"descriptor":d})
        return T(OB+"set_properties",{"instance":ref(sel),"values":json.dumps({"MeshEntries":me})}), sel
def point(x,y,z=0,yaw=0,scale=1.0,ext=50,density=1.0,seed=0):
    s=scale if isinstance(scale,(list,tuple)) else (scale,scale,scale)
    return {"transform":{"location":{"x":x,"y":y,"z":z},"rotation":{"pitch":0,"yaw":yaw,"roll":0},"scale":{"x":s[0],"y":s[1],"z":s[2]}},
            "density":density,"boundsMin":{"x":-ext,"y":-ext,"z":-ext},"boundsMax":{"x":ext,"y":ext,"z":ext},"color":{"x":1,"y":1,"z":1,"w":1},"steepness":1,"seed":seed,"metadataEntry":-1}
def save(paths): return T(AS+"save_assets",{"asset_paths":paths})

# ---------------- shared subgraphs ----------------
SUB_FOLDER="/Game/Wot/Towns/Common/PCG"
def sub_path(name): return f"{SUB_FOLDER}/{name}.{name}"
class SubG(G):
    """Builds one shared subgraph: extra input/output pins, graph parameters bound to node overrides.
    Parameters are named after the node property they override (Ratio, LowerBound, BoundsMin...) because a
    'Get Graph Parameter' node feeds the target node's Overrides pin with an attribute of that name."""
    def __init__(self,name,description):
        super().__init__(create_graph(name,SUB_FOLDER)); self.name=name
        st=T(PC+"GetGraphStructure",{"graph":ref(self.g)})
        for n in st["nodes"]:
            if n["nodeType"]=="Input Node": self.inp=n["path"]; self.inpins=n["paramOverrides"]["pins"]
            if n["nodeType"]=="Output Node": self.outp=n["path"]; self.outpins=n["paramOverrides"]["pins"]
        self.nodes["Input"]=self.inp; self.nodes["Output"]=self.outp; self.params=[]
        T(PC+"SetGraphDescription",{"graph":ref(self.g),"description":description})
    def _pin(self,template,label,tooltip=""):
        p=json.loads(json.dumps(template)); p["label"]=label; p["tooltip"]=tooltip; return p
    def inputs(self,*labels):
        """Replaces the input pins (the default 'In' stays first when listed)."""
        pins=[self._pin(self.inpins[0],l) for l in labels]; self.inpins=pins
        T(PC+"UpdateNode",{"node":ref(self.inp),"jsonParams":json.dumps({"pins":pins}),"nodeTitle":""})
    def outputs(self,*labels):
        pins=[self._pin(self.outpins[0],l) for l in labels]; self.outpins=pins
        T(PC+"UpdateNode",{"node":ref(self.outp),"jsonParams":json.dumps({"pins":pins}),"nodeTitle":""})
    def param(self,name,ptype,default,attr=None,x=0,y=0):
        """Declares a graph parameter and a Get Graph Parameter node (named P_<name>) that emits it as attribute attr (default: name)."""
        self.params.append({"name":name,"type":ptype,"defaultValue":json.dumps(default) if isinstance(default,(dict,list)) else str(default)})
        T(PC+"SetGraphParams",{"graph":ref(self.g),"params":[self.params[-1]]})
        return self.node("Get Graph Parameter",f"P_{name}",{"propertyPath":name,"outputAttributeName":attr or name},title=f"param {name}",x=x,y=y)
    def bind(self,param,node): return self.link(f"P_{param}","Out",node,"Overrides")
    def done(self):
        save([self.g.split(".")[0]]); return self.g
def comment_box(B,names,text,color=(0.25,0.35,0.55)):
    nodes=[ref(B.nodes[n]) for n in names if n in B.nodes]
    if nodes: return T(PC+"AddCommentBox",{"graph":ref(B.g),"nodes":nodes,"comment":text,"color":{"r":color[0],"g":color[1],"b":color[2],"a":1}})
