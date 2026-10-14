# ---------- relief: deterministic stepped (voxel) heightfield terrain for the town levels ----------
Q=40            # height quantum (cm); MaxStepHeight is 45 so one step is always walkable
CELL=250        # terrain cell size (cm)
STAIR_RISES=(200,240,280,320,360,400,480,600)
LADDER_BP="/Game/Wot/GeometryScripting/BP_Ladder"; POTION="/Game/Wot/Actors/HealthPotion"
WF_TOP="/Game/Wot/Effects/NS_WaterfallTop"; WF_FOAM="/Game/Wot/Effects/NS_WaterfallFoam"

def _hash(i,j,seed):
    n=(i*73856093)^(j*19349663)^(seed*83492791); n=(n ^ (n>>13))*1274126177; n=n ^ (n>>16)
    return (n & 0xffffff)/16777216.0
def _smooth(t): return t*t*(3-2*t)
def vnoise(x,y,scale,seed=7):
    fx=x/scale; fy=y/scale; i=math.floor(fx); j=math.floor(fy); tx=_smooth(fx-i); ty=_smooth(fy-j)
    a=_hash(i,j,seed); b=_hash(i+1,j,seed); c=_hash(i,j+1,seed); d=_hash(i+1,j+1,seed)
    return (a*(1-tx)+b*tx)*(1-ty)+(c*(1-tx)+d*tx)*ty
def _seg_dist(x,y,a,b):
    ax,ay=a[0],a[1]; bx,by=b[0],b[1]; dx=bx-ax; dy=by-ay; L2=dx*dx+dy*dy
    t=0.0 if L2==0 else max(0.0,min(1.0,((x-ax)*dx+(y-ay)*dy)/L2))
    px=ax+t*dx; py=ay+t*dy; return math.hypot(x-px,y-py)
def _poly_inside(x,y,poly):
    inside=False; n=len(poly); j=n-1
    for i in range(n):
        xi,yi=poly[i][0],poly[i][1]; xj,yj=poly[j][0],poly[j][1]
        if ((yi>y)!=(yj>y)) and (x < (xj-xi)*(y-yi)/(yj-yi)+xi): inside=not inside
        j=i
    return inside
def _poly_dist(x,y,poly):
    n=len(poly); d=1e18
    for i in range(n): d=min(d,_seg_dist(x,y,poly[i],poly[(i+1)%n]))
    return d
def _polyline_dist(x,y,pts):
    d=1e18
    for i in range(len(pts)-1): d=min(d,_seg_dist(x,y,pts[i],pts[i+1]))
    return d
def _bump(d,r):
    """1 at d=0 falling smoothly to 0 at d=r."""
    if d>=r: return 0.0
    return 0.5+0.5*math.cos(math.pi*d/r)
def qz(h): return int(round(h/Q))*Q

class Relief:
    def __init__(self,extent,base=0,water=None,cell=CELL,seed=1):
        self.E=extent; self.cell=cell; self.base=base; self.water=water; self.seed=seed
        self.feats=[]; self.plats=[]; self.n=int(round(2*extent/cell)); self.z=None; self.lock=None; self.matkey=None
        self.rock_z=None; self.snow_z=None; self.sand_h=80; self.mat_fn=None; self.stairs_mats=None; self.cellmat={}
    # ---- continuous features (additive) ----
    def hill(self,cx,cy,r,h): self.feats.append(("hill",cx,cy,r,h))
    def ridge(self,pts,w,h): self.feats.append(("ridge",pts,w,h))
    def valley(self,pts,w,depth,flat=0): self.feats.append(("valley",pts,w,depth,flat))
    def slope(self,x0,y0,x1,y1,h0,h1): self.feats.append(("slope",x0,y0,x1,y1,h0,h1))
    def noise(self,amp,scale,seed=7): self.feats.append(("noise",amp,scale,seed))
    def basin(self,cx,cy,r,depth): self.feats.append(("hill",cx,cy,r,-depth))
    # ---- plateaus override the height inside their shape; rim blends to the surroundings outside ----
    def plateau(self,shape,z,rim=0,mode="set"):
        """mode 'set': the inside is exactly z; 'max': the inside is at least z (hills still rise above it)."""
        self.plats.append((shape,z,rim,mode))
    def _shape_dist(self,x,y,shape):
        """signed distance: <=0 inside."""
        if shape[0]=="circle": return math.hypot(x-shape[1],y-shape[2])-shape[3]
        if shape[0]=="rect":
            x0,y0,x1,y1=shape[1:5]; dx=max(x0-x,0,x-x1); dy=max(y0-y,0,y-y1)
            if dx==0 and dy==0: return -min(x-x0,x1-x,y-y0,y1-y)
            return math.hypot(dx,dy)
        poly=shape[1]; d=_poly_dist(x,y,poly); return -d if _poly_inside(x,y,poly) else d
    def H(self,x,y):
        h=self.base
        for f in self.feats:
            k=f[0]
            if k=="hill": h+=f[4]*_bump(math.hypot(x-f[1],y-f[2]),f[3])
            elif k=="ridge": h+=f[3]*_bump(_polyline_dist(x,y,f[1]),f[2])
            elif k=="valley":
                d=_polyline_dist(x,y,f[1]); fl=f[4]
                h-= f[3] if d<=fl else f[3]*_bump(d-fl,f[2]-fl)
            elif k=="slope":
                x0,y0,x1,y1,h0,h1=f[1:7]; dx=x1-x0; dy=y1-y0; L2=dx*dx+dy*dy
                t=max(0.0,min(1.0,((x-x0)*dx+(y-y0)*dy)/L2)) if L2 else 0.0; h+=h0+(h1-h0)*t
            elif k=="noise": h+=f[1]*(vnoise(x,y,f[2],f[3])*2-1)
        for (shape,z,rim,mode) in self.plats:
            d=self._shape_dist(x,y,shape)
            if mode=="max":
                if d<=0: h=max(h,z)
                elif rim>0 and d<rim: t=_smooth(d/rim); h=max(h,z*(1-t)+h*t)
            elif d<=0: h=z
            elif rim>0 and d<rim: t=_smooth(d/rim); h=z*(1-t)+h*t
        return h
    # ---- grid ----
    def cx(self,i): return -self.E+(i+0.5)*self.cell
    def idx(self,x): return max(0,min(self.n-1,int(math.floor((x+self.E)/self.cell))))
    def build(self):
        n=self.n; self.z=[[qz(self.H(self.cx(i),self.cx(j))) for j in range(n)] for i in range(n)]
        self.lock=[[False]*n for _ in range(n)]
        return self
    def zat(self,x,y): return self.z[self.idx(x)][self.idx(y)]
    def set_cell(self,i,j,z,lock=False,force=False):
        if 0<=i<self.n and 0<=j<self.n and (force or not self.lock[i][j]):
            self.z[i][j]=z
            if lock: self.lock[i][j]=True
    def cells_near(self,x,y,r):
        i0=self.idx(x-r); i1=self.idx(x+r); j0=self.idx(y-r); j1=self.idx(y+r)
        for i in range(i0,i1+1):
            for j in range(j0,j1+1):
                if abs(self.cx(i)-x)<=r and abs(self.cx(j)-y)<=r: yield i,j
    def flatten(self,shape,z=None,lock=True,margin=0,force=False):
        """Flattens every cell whose centre is inside the shape (expanded by margin) to z (default: height at the shape centre)."""
        if shape[0]=="circle":
            cx,cy,r=shape[1],shape[2],shape[3]; z=self.zat(cx,cy) if z is None else qz(z)
            for i,j in self.cells_near(cx,cy,r+margin):
                if math.hypot(self.cx(i)-cx,self.cx(j)-cy)<=r+margin: self.set_cell(i,j,z,lock,force)
        elif shape[0]=="rect":
            x0,y0,x1,y1=shape[1:5]; z=self.zat((x0+x1)/2,(y0+y1)/2) if z is None else qz(z)
            for i in range(self.idx(x0-margin),self.idx(x1+margin)+1):
                for j in range(self.idx(y0-margin),self.idx(y1+margin)+1): self.set_cell(i,j,z,lock,force)
        else:
            poly=shape[1]; xs=[p[0] for p in poly]; ys=[p[1] for p in poly]
            z=self.zat(sum(xs)/len(xs),sum(ys)/len(ys)) if z is None else qz(z)
            for i in range(self.idx(min(xs)-margin),self.idx(max(xs)+margin)+1):
                for j in range(self.idx(min(ys)-margin),self.idx(max(ys)+margin)+1):
                    x,y=self.cx(i),self.cx(j)
                    if _poly_inside(x,y,poly) or (margin>0 and _poly_dist(x,y,poly)<=margin): self.set_cell(i,j,z,lock,force)
        return z
    # ---- roads ----
    def _densify(self,pts,closed,step):
        seq=[(p[0],p[1]) for p in pts]+([(pts[0][0],pts[0][1])] if closed else [])
        out=[]
        for i in range(len(seq)-1):
            ax,ay=seq[i]; bx,by=seq[i+1]; L=math.hypot(bx-ax,by-ay); n=max(1,int(L/step))
            for k in range(n): out.append((ax+(bx-ax)*k/n, ay+(by-ay)*k/n))
        out.append(seq[-1]); return out
    def carve_roads(self,spec):
        self._stair_pts=[]
        """Gives every road a walkable height profile (<= one Q step per cell), cuts it into the terrain and places stairs where it
        meets a terrace edge (a jump >= 4Q between otherwise flat ground). Returns stairs props.
        'nocarve' roads keep their own z (bridges/piers); 'water' roads become channels; walls just follow the ground."""
        props=[]; cell=self.cell; W=self.water
        for ri,rd in enumerate(spec["roads"]):
            if gv(rd,"nocarve",False): continue
            kind=gv(rd,"kind","road"); w=gv(rd,"width",7); pitch=gv(rd,"row_pitch",100); half=w*pitch/2.0
            closed=gv(rd,"closed",False); samples=self._densify(rd["pts"],closed,cell)
            if kind=="water":
                dep=gv(rd,"depth",120)
                if W is not None:
                    zc=qz(W-dep); pts=[]
                    for (x,y) in samples:
                        for i,j in self.cells_near(x,y,half+cell*0.5):
                            if self.z[i][j]>zc: self.set_cell(i,j,zc,True,True)
                        pts.append((x,y,W))
                    rd["kind"]="nobuild"
                else:   # no water plane: a stream bed following the ground, drawn with water tiles
                    pts=[]; zs=[self.zat(x,y) for (x,y) in samples]
                    for a in range(1,len(zs)): zs[a]=min(zs[a],zs[a-1]+Q)      # streams run downhill-ish
                    for (x,y),zz in zip(samples,zs):
                        zc=qz(zz-dep)
                        for i,j in self.cells_near(x,y,half+cell*0.5): self.set_cell(i,j,zc,True,True)
                        pts.append((x,y,zc+30))
                rd["pts"]=pts; rd["linear"]=True; rd["_profile"]=list(pts)
                continue
            if kind=="wall" or kind=="nobuild":
                rd["pts"]=[(x,y,self.zat(x,y)) for (x,y) in samples]; rd["linear"]=True; rd["_profile"]=list(rd["pts"]); continue
            rmat=gv(rd,"mat",gv(spec,"road_mat"))
            zs=[self.zat(x,y) for (x,y) in samples]; n=len(samples); fixed=[False]*n
            out=[]; k=0; last_stair=-10
            while k<n:
                x,y=samples[k]; z=zs[k]
                if k+1<n and not gv(rd,"ramps",False) and k-last_stair>=3:
                    z2=zs[k+1]; R=abs(z2-z)
                    flat_before=(k==0 or abs(zs[k-1]-z)<=Q); flat_after=(k+2>=n or abs(zs[k+2]-z2)<=Q)
                    if R>=4*Q and R<=600 and flat_before and flat_after:
                        x2,y2=samples[k+1]; ex,ey=x2,y2
                        for st in range(1,21):
                            t=st/20.0; sx=x+(x2-x)*t; sy=y+(y2-y)*t
                            if abs(self.zat(sx,sy)-z)>=4*Q: ex,ey=sx,sy; break
                        up=z2>z; zhi=max(z,z2); S=min([s_ for s_ in STAIR_RISES if s_>=R]); zlo=zhi-S
                        dx=x2-x; dy=y2-y; L=math.hypot(dx,dy) or 1.0; dx/=L; dy/=L
                        if up: fx,fy=ex-dx*2*S,ey-dy*2*S; yaw=math.degrees(math.atan2(dy,dx))
                        else: fx,fy=ex+dx*2*S,ey+dy*2*S; yaw=math.degrees(math.atan2(-dy,-dx))
                        props.append(("mesh",kit_mesh(f"SM_Stairs{S}"),f"Stairs_{ri}_{k}",(fx,fy,zlo),yaw,(1.0,max(1.0,w*pitch/500.0),1.0),self.stairs_mats))
                        self._stair_pts.append((fx,fy))
                        sgn=1 if up else -1
                        for st in range(0,int(2*S/50)+2):
                            px=fx+dx*st*50*sgn; py=fy+dy*st*50*sgn
                            for i,j in self.cells_near(px,py,half+cell*0.5): self.set_cell(i,j,zlo,True,True); self.cellmat[(i,j)]=rmat
                        last_stair=k
                        if up:
                            while out and math.hypot(out[-1][0]-ex,out[-1][1]-ey)<2*S+1: out.pop()
                            out+= [[fx,fy,zlo,True],[ex,ey,zlo,True],[ex,ey,zhi,True]]; zs[k+1]=zhi; fixed[k+1]=True; k+=1; continue
                        else:
                            out+= [[x,y,zhi,True],[ex,ey,zhi,True],[ex,ey,zlo,True],[fx,fy,zlo,True]]; k+=1
                            while k<n and math.hypot(samples[k][0]-ex,samples[k][1]-ey)<2*S+1: k+=1
                            if k<n: zs[k]=zlo; fixed[k]=True
                            continue
                out.append([x,y,z,fixed[k]]); k+=1
            for p in range(2):
                rng=range(1,len(out)) if p==0 else range(len(out)-2,-1,-1)
                for a in rng:
                    b=a-1 if p==0 else a+1
                    if out[a][3]: continue
                    if out[a][0]==out[b][0] and out[a][1]==out[b][1]: continue
                    lo=out[b][2]-Q; hi=out[b][2]+Q; out[a][2]=max(lo,min(hi,out[a][2]))
            # cut the corridor: every cell takes the z of its nearest road sample
            best={}
            for (x,y,z,fx) in out:
                for i,j in self.cells_near(x,y,half+cell*0.5):
                    d=math.hypot(self.cx(i)-x,self.cx(j)-y)
                    if (i,j) not in best or d<best[(i,j)][0]: best[(i,j)]=(d,z)
            for (i,j),(d,z) in best.items():
                was_locked=self.lock[i][j]; self.set_cell(i,j,z,True)
                if d<=half+cell*0.25 and (not was_locked or (i,j) not in self.cellmat): self.cellmat[(i,j)]=rmat
            rd["_profile"]=[(x,y,z) for (x,y,z,_) in out]
            # stepped profile for the tile spline: z stays constant up to just past the midpoint between samples (inside the
            # higher cell, so the stacked tiles at the jump are hidden), then jumps
            stepped=[]
            for a in range(len(out)):
                x,y,z,_=out[a]
                if a>0:
                    px,py,pz,_=out[a-1]
                    if pz!=z and not (px==x and py==y):
                        L=math.hypot(x-px,y-py) or 1.0; sh=(12.0/L)*(1 if z>pz else -1)
                        t=0.5+sh; mx,my=px+(x-px)*t,py+(y-py)*t; stepped.append((mx,my,pz)); stepped.append((mx,my,z))
                stepped.append((x,y,z))
            rd["pts"]=stepped; rd["linear"]=True
            if closed: rd["closed"]=False
        return props
    def house_pads(self,spec):
        """Flat pads where the PCG house sampler will put houses (same spacing/setback as the graph)."""
        hs=gv(spec,"houses") or {}; sets=gv(spec,"house_sets") or {}
        for rd in spec["roads"]:
            if not gv(rd,"houses",False): continue
            cfg=sets[rd["set"]] if gv(rd,"set") in sets else hs
            sp=gv(rd,"spacing",gv(hs,"spacing",1400)); sb=gv(rd,"setback",gv(hs,"setback",900)); sides=gv(rd,"sides","both")
            fp=gv(cfg,"footprint",(-560,-560,740,420)); r=max(abs(fp[0]),abs(fp[1]),abs(fp[2]),abs(fp[3]))+60
            pts=rd["pts"]; dist=0.0; nxt=0.0
            for i in range(len(pts)-1):
                ax,ay,az=pts[i][0],pts[i][1],pts[i][2]; bx,by,bz=pts[i+1][0],pts[i+1][1],pts[i+1][2]
                L=math.hypot(bx-ax,by-ay)
                if L==0: continue
                tx,ty=(bx-ax)/L,(by-ay)/L
                while nxt<=dist+L:
                    t=(nxt-dist)/L; x=ax+(bx-ax)*t; y=ay+(by-ay)*t; z=qz(az+(bz-az)*t+Q*0.49)
                    for sg,side in ((1,"left"),(-1,"right")):
                        if sides=="both" or sides==side:
                            px=x+sg*sb*(-ty); py=y+sg*sb*tx
                            for ci,cj in self.cells_near(px,py,r): self.set_cell(ci,cj,z,False,False)
                    nxt+=sp
                dist+=L
        for (x,y,yaw) in gv(spec,"house_pts") or []:
            z=self.zat(x,y)
            for ci,cj in self.cells_near(x,y,700): self.set_cell(ci,cj,z,False,False)
    def _walk(self,prof,step,start=0.0):
        """Yields (x,y,z,tangent_yaw) every `step` cm along a 3D polyline profile."""
        dist=0.0; nxt=start
        for i in range(len(prof)-1):
            ax,ay,az=prof[i]; bx,by,bz=prof[i+1]; L=math.hypot(bx-ax,by-ay)
            if L==0: continue
            yaw=math.degrees(math.atan2(by-ay,bx-ax))
            while nxt<=dist+L:
                t=(nxt-dist)/L; yield (ax+(bx-ax)*t, ay+(by-ay)*t, az+(bz-az)*t, yaw); nxt+=step
            dist+=L
    def road_props(self,spec):
        """House slots (per set), street-line props and wall towers from the carved road profiles. Rotation is yaw only."""
        hs=gv(spec,"houses") or {}; sets=gv(spec,"house_sets") or {}
        slots={"":[]}; lines=[]; towers=[]
        for k in sets: slots[k]=[]
        for rd in spec["roads"]:
            prof=gv(rd,"_profile")
            if not prof: continue
            if gv(rd,"houses",False):
                key=rd["set"] if gv(rd,"set") in sets else ""
                sp=gv(rd,"spacing",gv(hs,"spacing",1400)); sb=gv(rd,"setback",gv(hs,"setback",900)); sides=gv(rd,"sides","both")
                for (x,y,z,yaw) in self._walk(prof,sp):
                    tx,ty=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
                    for sg,side in ((1,"left"),(-1,"right")):
                        if sides=="both" or sides==side:
                            px=x+sg*sb*(-ty); py=y+sg*sb*tx; slots[key].append((px,py,qz(z+Q*0.49),yaw+90*sg))
            for ln in gv(rd,"lines") or []:
                off=ln["offset"]; sd=gv(ln,"sides","both"); sgs=[1] if sd=="left" else ([-1] if sd=="right" else [1,-1])
                pts=[]
                for (x,y,z,yaw) in self._walk(prof,ln["step"]):
                    tx,ty=math.cos(math.radians(yaw)),math.sin(math.radians(yaw))
                    for sg in sgs: pts.append((x+sg*off*(-ty),y+sg*off*tx,z+gv(ln,"z",0),yaw+gv(ln,"yaw",0)+90*sg))
                lines.append({"mesh":ln["mesh"],"mats":[gv(ln,"mat")] if gv(ln,"mat") else None,"scale":gv(ln,"scale",(1,1,1)),"pts":pts,"name":gv(rd,"name","")})
            tw=gv(rd,"towers")
            if tw:
                pts=[(x,y,z+tw["z"],0) for (x,y,z,yaw) in self._walk(prof,tw["step"])]
                towers.append({"mesh":gv(tw,"mesh",CUBE),"mats":[tw["mat"]],"scale":tw["scale"],"pts":pts,"name":gv(rd,"name","")})
        for (x,y,yaw) in gv(spec,"house_pts") or []: slots[""].append((x,y,self.zat(x,y),yaw))
        # placement rules: no house footprint over a crossing road/plaza, no two slots on top of each other
        foot=gv(spec,"slot_clear",260); minslot=gv(spec,"slot_spacing",780)
        allkept=[]
        for k in list(slots.keys()):
            cand=[sl for sl in slots[k] if not self.near_paved(sl[0],sl[1],foot) and not self._near_any(getattr(self,"_stair_pts",()),sl[0],sl[1],gv(spec,"slot_stair_clear",560))]
            kept=self._thin(allkept+cand,minslot)[len(allkept):]
            slots[k]=kept; allkept+=kept
        self._slot_pts=[(sl[0],sl[1]) for sl in allkept]
        # street furniture: never on pavement, never in a doorway, never doubled up where roads meet
        self._line_pts=[]
        for ln in lines:
            pts=[p for p in ln["pts"] if not self.near_paved(p[0],p[1],gv(spec,"line_clear",110)) and not self._near_any(self._slot_pts,p[0],p[1],gv(spec,"line_house_clear",330)) and not self._near_any(getattr(self,"_stair_pts",()),p[0],p[1],280)]
            step=min((l["step"] for rd in spec["roads"] for l in (gv(rd,"lines") or []) if l["mesh"]==ln["mesh"]),default=1000)
            pts=self._thin(self._line_pts_all(lines,ln)+pts,0.55*step)[len(self._line_pts_all(lines,ln)):]
            ln["pts"]=pts; self._line_pts+=[(p[0],p[1]) for p in pts]
        spec["house_slots"]=slots; spec["line_sets"]=lines; spec["tower_sets"]=towers
    def _line_pts_all(self,lines,upto):
        out=[]
        for ln in lines:
            if ln is upto: break
            out+=[(p[0],p[1]) for p in ln["pts"]]
        return out
    def carve_plazas(self,spec):
        for pz in gv(spec,"plazas") or []:
            pts=pz["pts"]; z=gv(pz,"z")
            if z is not None and z<0 and W_is_rel(pz): pass
            zz=self.flatten(("poly",pts),z,True,margin=self.cell*0.5)
            pz["z"]=zz; pz["pts"]=[(p[0],p[1]) for p in pts]
            xs=[p[0] for p in pts]; ys=[p[1] for p in pts]
            for i in range(self.idx(min(xs)),self.idx(max(xs))+1):
                for j in range(self.idx(min(ys)),self.idx(max(ys))+1):
                    if _poly_inside(self.cx(i),self.cx(j),pts): self.cellmat[(i,j)]=pz["mat"]
    # ---- output ----
    def _mat(self,i,j):
        if (i,j) in self.cellmat: return self.cellmat[(i,j)]
        z=self.z[i][j]; x=self.cx(i); y=self.cx(j)
        if self.mat_fn:
            m=self.mat_fn(x,y,z)
            if m: return m
        if self.snow_z is not None and z>=self.snow_z: return "snow"
        if self.rock_z is not None and z>=self.rock_z: return "rock"
        if self.water is not None and z<=self.water+self.sand_h: return "sand"
        return "grass"
    def is_cliff(self,i,j):
        z=self.z[i][j]
        for (a,b) in ((i-1,j),(i+1,j),(i,j-1),(i,j+1)):
            if 0<=a<self.n and 0<=b<self.n and z-self.z[a][b]>=3*Q: return True
        return False
    # ---- placement rules (roads, plazas and water are painted into cellmat) ----
    def paved(self,x,y): return (self.idx(x),self.idx(y)) in self.cellmat
    def near_paved(self,x,y,r):
        """True when any painted (road/plaza/water) cell centre lies within r of (x,y)."""
        i0,j0=self.idx(x-r),self.idx(y-r); i1,j1=self.idx(x+r),self.idx(y+r)
        for i in range(i0,i1+1):
            for j in range(j0,j1+1):
                if (i,j) in self.cellmat and math.hypot(self.cx(i)-x,self.cx(j)-y)<=r: return True
        return False
    @staticmethod
    def _thin(pts,mind,key=lambda p:(p[0],p[1])):
        """Greedy spacing filter: keeps a point only when no kept point is closer than mind (grid hash)."""
        kept=[]; grid={}; g=max(1.0,mind)
        for p in pts:
            x,y=key(p); gi,gj=int(x//g),int(y//g); ok=True
            for a in (gi-1,gi,gi+1):
                for b in (gj-1,gj,gj+1):
                    for (qx,qy) in grid.get((a,b),()):
                        if (qx-x)**2+(qy-y)**2<mind*mind: ok=False; break
                    if not ok: break
                if not ok: break
            if ok: kept.append(p); grid.setdefault((gi,gj),[]).append((x,y))
        return kept
    def _near_any(self,pts,x,y,r):
        for (px,py) in pts:
            if abs(px-x)<=r and abs(py-y)<=r and (px-x)**2+(py-y)**2<=r*r: return True
        return False
    def boxes(self):
        """Run-merged terrain boxes: list of (matkey, x, y, zcenter, sx, sy, sz) in cm."""
        n=self.n; zmin=min(min(r) for r in self.z); floor=zmin-400
        if self.water is not None: floor=min(floor,self.water-600)
        out=[]; cell=self.cell
        keys=[[(self.z[i][j],self._mat(i,j),self.is_cliff(i,j)) for j in range(n)] for i in range(n)]
        for i in range(n):
            j=0
            while j<n:
                k=keys[i][j]; j0=j
                while j<n and keys[i][j]==k: j+=1
                z,m,cliff=k; y=(self.cx(j0)+self.cx(j-1))/2; sy=(j-j0)*cell; x=self.cx(i)
                if cliff:
                    out.append(("rock",x,y,(floor+z-Q)/2,cell,sy,z-Q-floor)); out.append((m,x,y,z-Q/2,cell,sy,Q))
                else: out.append((m,x,y,(floor+z)/2,cell,sy,z-floor))
        return out
    def lattice(self):
        pts=[]; n=self.n
        for i in range(n):
            for j in range(n):
                z=self.z[i][j]
                if self.water is not None and z<=self.water: continue
                m=self._mat(i,j)
                if m in ("rock","snow") or self.is_cliff(i,j): continue
                x,y=self.cx(i),self.cx(j)
                if (i,j) in self.cellmat or self.near_paved(x,y,self.cell*0.75): continue
                if self._near_any(getattr(self,"_slot_pts",()),x,y,330) or self._near_any(getattr(self,"_line_pts",()),x,y,170) or self._near_any(getattr(self,"_stair_pts",()),x,y,220): continue
                pts.append((x,y,z))
        return pts
def W_is_rel(pz): return False
def peaks(R,pts,w,h,n=5,seed=3,jag=0.45):
    """A mountain range along a polyline: a broad ridge plus n individual peaks of varying height, and rough noise."""
    R.ridge(pts,w,h*0.55)
    L=[0.0]
    for i in range(1,len(pts)): L.append(L[-1]+math.hypot(pts[i][0]-pts[i-1][0],pts[i][1]-pts[i-1][1]))
    for k in range(n):
        t=(k+0.5)/n*L[-1]; i=1
        while i<len(L)-1 and L[i]<t: i+=1
        f=(t-L[i-1])/max(1e-6,L[i]-L[i-1]); x=pts[i-1][0]+(pts[i][0]-pts[i-1][0])*f; y=pts[i-1][1]+(pts[i][1]-pts[i-1][1])*f
        hh=h*(0.7+0.6*_hash(k,1,seed)); rr=w*(0.35+0.3*_hash(k,2,seed))
        ox=(_hash(k,3,seed)-0.5)*w*0.4; oy=(_hash(k,4,seed)-0.5)*w*0.4
        R.hill(x+ox,y+oy,rr,hh)
    R.noise(h*jag*0.12,w*0.22,seed=seed+11); R.noise(h*jag*0.05,w*0.08,seed=seed+17)

# ---------- spec-level helpers ----------
def relief_apply(spec,R,palette,water_mat=None):
    """Builds the grid, carves roads/plazas/pads/prop pads, and attaches terrain boxes, lattice and water to the spec."""
    R.build()
    for (shape,z) in gv(spec,"pads") or []: R.flatten(shape,z,True)
    stairs=R.carve_roads(spec)
    for rd in spec["roads"]:
        if gv(rd,"nocarve",False) and gv(rd,"rel",False):
            p0=rd["pts"][0]; z0=R.zat(p0[0],p0[1]); rd["pts"]=[(p[0],p[1],z0+(p[2] if len(p)>2 else 0)) for p in rd["pts"]]
    R.carve_plazas(spec)
    R.house_pads(spec)
    R.road_props(spec)
    spec["props"]=[p for p in spec["props"] if p[2]!="Ground"]+stairs
    spec["terrain"]={"boxes":R.boxes(),"palette":palette}
    spec["lattice"]=R.lattice(); spec["lattice_density"]=(R.cell/100.0)**2
    spec["_relief"]=R
    if R.water is not None:
        E=R.E
        spec["props"].append(("mesh",CUBE,"WaterPlane",(0,0,R.water-10),0,(E*2/100.0,E*2/100.0,0.2),[water_mat or WATER],"abs"))
    # lift portal/player-start moves onto the terrain
    mv=[]
    for (label,loc,yaw) in gv(spec,"moves") or []:
        mv.append((label,(loc[0],loc[1],R.zat(loc[0],loc[1])+loc[2]),yaw))
    spec["moves"]=mv
    spec["lift_props"]=True
    return R
def stair_prop(label,x,y,yaw,rise,z=0,mats=None):
    return ("mesh",kit_mesh(f"SM_Stairs{rise}"),label,(x,y,z),yaw,1.0,mats)
def ladder(label,x,y,yaw,h,z=0):
    return ("actor",LADDER_BP,label,(x,y,z),yaw,1.0,{"LadderSize":{"x":0,"y":0,"z":h}})
def npc(label,name,row,x,y,yaw=0,radius=900,cls=None,tags=()):
    """A villager (or, with cls, a main-character NPC Blueprint). tags become actor tags, e.g. 'QuestStart=Q_Winternight'."""
    props={"Name":name,"CurrentRowName":row,"WanderRadius":float(radius)}
    if tags: props["Tags"]=list(tags)
    return ("npc",label,(x,y),yaw,props,cls)
MAIN_NPC="/Game/Wot/NPCs/Main/%s_NPC"
def hero(label,name,row,x,y,yaw=0,radius=350,tags=()):
    """Main character NPC (Blueprint under /Game/Wot/NPCs/Main, created by setup_cast.py)."""
    return npc(label,name,row,x,y,yaw,radius,cls=MAIN_NPC%label,tags=tags)
def lines(prefix,texts):
    d={}
    for i,t in enumerate(texts): d[f"{prefix}_{i+1}"]=(t, f"{prefix}_{i+2}" if i+1<len(texts) else "")
    return d
def potion(label,x,y,z=0): return ("mesh",POTION,label,(x,y,z),0,1.0,None)
def chest(label,x,y,z=0,yaw=0): return ("mesh",CHEST,label,(x,y,z),yaw,1.0,None)
def waterfall(label,x,y,yaw,z_top,z_bottom):
    return [("mesh",WF_TOP,label+"_top",(x,y,z_top),yaw,1.0,None,"abs"),("mesh",WF_FOAM,label+"_foam",(x,y,z_bottom+20),yaw,1.0,None,"abs")]
def ring_excl(cx,cy,r,n=24,w=420):
    return [(cx+r*math.cos(2*math.pi*k/n),cy+r*math.sin(2*math.pi*k/n),w,w) for k in range(n)]
def rect_excl(x0,y0,x1,y1,w=380):
    return [((x0+x1)/2,y0,(x1-x0)/2+w,w),((x0+x1)/2,y1,(x1-x0)/2+w,w),(x0,(y0+y1)/2,w,(y1-y0)/2+w),(x1,(y0+y1)/2,w,(y1-y0)/2+w)]
def wiggle(pts,amp=300,seed=3,n=1):
    """Adds gentle lateral meander to a polyline (inserts midpoints offset sideways)."""
    out=[]
    for i in range(len(pts)-1):
        ax,ay=pts[i][0],pts[i][1]; bx,by=pts[i+1][0],pts[i+1][1]; out.append((ax,ay))
        L=math.hypot(bx-ax,by-ay)
        if L==0: continue
        nx,ny=-(by-ay)/L,(bx-ax)/L
        for k in range(1,n+1):
            t=k/(n+1); s=(_hash(i,k,seed)*2-1)*amp
            out.append((ax+(bx-ax)*t+nx*s, ay+(by-ay)*t+ny*s))
    out.append((pts[-1][0],pts[-1][1])); return out
