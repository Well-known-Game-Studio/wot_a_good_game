import re
# ---------- relief: deterministic stepped (voxel) heightfield terrain for the town levels ----------
FURN_DEBUG=False
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
    def _flight_box(self,footx,footy,ax,ay,Lrun,halfw):
        """World AABB of a flight's run plus its upper landing (what finalize_stairs will paint)."""
        cell=self.cell; a0,a1=-cell*0.6,Lrun+cell*1.6; c=halfw+cell*0.5
        xs=[footx+ax*a0-ay*c,footx+ax*a1-ay*c,footx+ax*a0+ay*c,footx+ax*a1+ay*c]; ys=[footy+ay*a0+ax*c,footy+ay*a1+ax*c,footy+ay*a0-ax*c,footy+ay*a1-ax*c]
        return (min(xs),min(ys),max(xs),max(ys))
    def _flight_clear(self,footx,footy,ax,ay,Lrun,halfw):
        b=self._flight_box(footx,footy,ax,ay,Lrun,halfw)
        for st in getattr(self,"_stairs",[]):
            (ox,oy)=st["foot"]; (oax,oay)=st["axis"]; o=self._flight_box(ox,oy,oax,oay,st["run"],st["halfw"])
            if b[0]<o[2] and o[0]<b[2] and b[1]<o[3] and o[1]<b[3]: return False
        return True
    def finalize_stairs(self):
        """Last word on the ground under every flight: the run sits on the lower level, the cells just past the top edge are
        the upper level (the corridor cut and later roads/pads may have repainted them), all paved."""
        cell=self.cell
        for st in getattr(self,"_stairs",[]):
            (footx,footy)=st["foot"]; (ax,ay)=st["axis"]; Lrun=st["run"]; half=st["half"]; zlo=st["zlo"]; zhi=st["zhi"]
            for a_ in range(-int(cell*0.5),int(Lrun+cell*1.6),int(cell*0.5)):
                qx=footx+ax*a_; qy=footy+ay*a_
                for i,j in self.cells_near(qx,qy,half+cell*0.5):
                    al=(self.cx(i)-footx)*ax+(self.cx(j)-footy)*ay; ac=abs(-(self.cx(i)-footx)*ay+(self.cx(j)-footy)*ax)
                    if ac>half+cell*0.5 or al<-cell*0.6 or al>Lrun+cell*1.6: continue
                    self.set_cell(i,j,zlo if al<Lrun-1 else zhi,True,True); self.cellmat[(i,j)]=st["mat"]
    def carve_roads(self,spec):
        self._stair_pts=[]; self._stairs=[]
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
                    wmat=gv(rd,"mat",WATER)
                    for (x,y),zz in zip(samples,zs):
                        zc=qz(zz-dep)
                        for i,j in self.cells_near(x,y,half+cell*0.5):
                            self.set_cell(i,j,zc,True,True)
                            if math.hypot(self.cx(i)-x,self.cx(j)-y)<=half+cell*0.25: self.cellmat[(i,j)]=wmat   # the bed itself is water-coloured
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
                        # the terraces are axis-aligned cell edges: climb along the axis where the terrain actually steps
                        # (toward the higher road sample), put the top edge on that cell boundary, tuck into a corner when a
                        # second wall is beside the flight, and paint an axis-aligned connector so the road enters the front
                        dx=x2-x; dy=y2-y
                        if not up: dx,dy=-dx,-dy            # ascent direction along the road (low -> high)
                        cands=[]
                        for (axc,ayc) in ((1,0),(0,1)):
                            d=self.zat(ex+axc*cell*1.5,ey+ayc*cell*1.5)-self.zat(ex-axc*cell*1.5,ey-ayc*cell*1.5)
                            if abs(d)>=3*Q:
                                sg=1 if d>0 else -1
                                if axc*sg*dx+ayc*sg*dy>0: cands.append((abs(abs(d)-S),axc*sg,ayc*sg))
                        if cands: cands.sort(); ax,ay=cands[0][1],cands[0][2]
                        elif abs(dx)>=abs(dy): ax,ay=(1 if dx>=0 else -1),0
                        else: ax,ay=0,(1 if dy>=0 else -1)
                        yaw=math.degrees(math.atan2(ay,ax))
                        Lrun=2*S; piv=20                    # SM_Stairs: run 40 per 20 rise, pivot at the centre of the first step
                        halfw=max(500.0,w*pitch)/2.0+15     # flight width incl. side walls
                        if ax: edge=-self.E+round((ex+self.E)/cell)*cell; along=edge; across=ey
                        else: edge=-self.E+round((ey+self.E)/cell)*cell; along=edge; across=ex
                        # snap to the boundary where the ground actually steps up (within 3 cells of the guess), nearest first
                        best_e=None
                        for t_ in sorted(range(-3,4),key=abs):
                            e_=edge+t_*cell
                            lo_=(e_-ax*cell*0.5,across) if ax else (across,e_-ay*cell*0.5); hi_=(e_+ax*cell*0.5,across) if ax else (across,e_+ay*cell*0.5)
                            if self.zat(*lo_)<=zlo+2*Q and self.zat(*hi_)>=zhi-2*Q: best_e=e_; break
                        if best_e is not None: edge=best_e; along=edge
                        # corner: a wall beside the top of the flight -> slide the flight against it
                        px_,py_=(along+ax*cell*0.5,across) if ax else (across,along+ay*cell*0.5)
                        for sg in (1,-1):
                            qx,qy=(px_,py_+sg*(halfw+cell)) if ax else (px_+sg*(halfw+cell),py_)
                            if self.zat(qx,qy)-zhi>=3*Q:
                                wall=-self.E+round(((py_ if ax else px_)+sg*halfw+self.E)/cell)*cell
                                across=wall-sg*halfw; break
                        if ax: ex,ey=along,across; fx,fy=along-ax*(Lrun-piv),across
                        else: ex,ey=across,along; fx,fy=across,along-ay*(Lrun-piv)
                        # two roads crossing the same edge within a flight's width share one flight: reuse the earlier one
                        shared=None
                        for st_ in self._stairs:
                            if st_["axis"]!=(ax,ay) or abs(st_["zlo"]-zlo)>Q or abs(st_["zhi"]-zhi)>Q: continue
                            (sfx,sfy)=st_["foot"]; dal=(fx-ax*piv-sfx)*ax+(fy-ay*piv-sfy)*ay; dac=abs(-(fx-ax*piv-sfx)*ay+(fy-ay*piv-sfy)*ax)
                            if dac<=halfw+st_["halfw"] and -Lrun<dal<st_["run"]: shared=st_; break
                        skip=False
                        if shared:
                            (sfx,sfy)=shared["foot"]; Lrun=shared["run"]; S=int(Lrun/2); fx,fy=sfx+ax*piv,sfy+ay*piv; ex,ey=sfx+ax*Lrun,sfy+ay*Lrun
                            if ax: across=ey
                            else: across=ex
                        elif not self._flight_clear(fx-ax*piv,fy-ay*piv,ax,ay,Lrun,halfw):
                            # a flight from another road already owns this ground (a corner): slide along the edge until clear
                            for sh_ in (1,-1,2,-2,3,-3,4,-4):
                                acr=across+sh_*cell
                                tfx,tfy=(fx,acr) if ax else (acr,fy)
                                if self._flight_clear(tfx-ax*piv,tfy-ay*piv,ax,ay,Lrun,halfw): across=acr; fx,fy=tfx,tfy; ex,ey=(ex,acr) if ax else (acr,ey); break
                            else: skip=True
                        if skip: pass
                        elif not shared:
                            props.append(("mesh",kit_mesh(f"SM_Stairs{S}"),f"Stairs_{ri}_{k}",(fx,fy,zlo),yaw,(1.0,max(1.0,w*pitch/500.0),1.0),self.stairs_mats,"abs"))  # zlo is already terrain height
                            self._stair_pts.append((fx,fy))
                            self._stairs.append({"foot":(fx-ax*piv,fy-ay*piv),"axis":(ax,ay),"run":Lrun,"halfw":halfw,"half":half,"zlo":zlo,"zhi":zhi,"mat":rmat})
                            for a_ in range(-60,int(Lrun)+int(cell)+1,int(cell*0.5)):
                                self.keepout(fx-ax*piv+ax*a_,fy-ay*piv+ay*a_,halfw+80)
                        def paint_seg(ax_,ay_,bx_,by_,zz):
                            n_=max(1,int(math.hypot(bx_-ax_,by_-ay_)/(cell*0.5)))
                            for t_ in range(n_+1):
                                qx=ax_+(bx_-ax_)*t_/n_; qy=ay_+(by_-ay_)*t_/n_
                                for i,j in self.cells_near(qx,qy,half+cell*0.5): self.set_cell(i,j,zz,True,True); self.cellmat[(i,j)]=rmat
                        # landing under the flight and the upper terrace just past its top edge
                        footx,footy=fx-ax*piv,fy-ay*piv
                        for i,j in self.cells_near((footx+ex)/2,(footy+ey)/2,0):
                            pass
                        for st in range(-1,int(Lrun/50)+1):
                            qx=footx+ax*st*50; qy=footy+ay*st*50
                            for i,j in self.cells_near(qx,qy,half+cell*0.5):
                                a_=(self.cx(i)-footx)*ax+(self.cx(j)-footy)*ay
                                self.set_cell(i,j,zlo if a_<Lrun-1 else zhi,True,True); self.cellmat[(i,j)]=rmat
                        for st in range(1,4):
                            qx=ex+ax*st*cell*0.5; qy=ey+ay*st*cell*0.5
                            for i,j in self.cells_near(qx,qy,half+cell*0.5):
                                if (self.cx(i)-ex)*ax+(self.cx(j)-ey)*ay>0: self.set_cell(i,j,zhi,True,True); self.cellmat[(i,j)]=rmat
                        # axis-aligned connectors: road sample -> foot (low) as an L, top edge -> next road sample (high)
                        lx,ly=(x,y) if up else (x2,y2); hx_,hy_=(x2,y2) if up else (x,y)
                        if ax: paint_seg(lx,ly,lx,footy,zlo); paint_seg(lx,footy,footx,footy,zlo); paint_seg(ex,ey,hx_,ey,zhi); paint_seg(hx_,ey,hx_,hy_,zhi)
                        else: paint_seg(lx,ly,footx,ly,zlo); paint_seg(footx,ly,footx,footy,zlo); paint_seg(ex,ey,ex,hy_,zhi); paint_seg(ex,hy_,hx_,hy_,zhi)
                        if skip:
                            out.append([x,y,z,fixed[k]]); k+=1; continue
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
                for (x,y,z,yaw) in self._walk(prof,ln["step"],start=ln["step"]*0.5):   # half a step in: nothing right at a junction corner
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
            cand=[sl for sl in slots[k] if not self.near_paved(sl[0],sl[1],foot) and not self._kept_out(sl[0],sl[1],400) and not self._occ_hit(sl[0],sl[1],420) and not self._near_any(getattr(self,"_stair_pts",()),sl[0],sl[1],gv(spec,"slot_stair_clear",560))]
            kept=self._thin(allkept+cand,minslot)[len(allkept):]
            slots[k]=kept; allkept+=kept
        self._slot_pts=[(sl[0],sl[1]) for sl in allkept]
        # street furniture: never on pavement, never in a doorway, never doubled up where roads meet
        self._line_pts=[]
        for ln in lines:
            pts=[p for p in ln["pts"] if not self.near_paved(p[0],p[1],gv(spec,"line_clear",110)) and not self._kept_out(p[0],p[1]) and not self._occ_hit(p[0],p[1],120) and not self._near_any(self._slot_pts,p[0],p[1],gv(spec,"line_house_clear",330)) and not self._near_any(getattr(self,"_stair_pts",()),p[0],p[1],280)]
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
    def _occ_hit(self,x,y,r):
        for (ox,oy,orad,k) in getattr(self,"_occ",()):
            if abs(ox-x)<=orad+r and abs(oy-y)<=orad+r and (ox-x)**2+(oy-y)**2<(orad+r)**2: return True
        return False
    def keepout(self,x,y,r):
        """Registers a circle no prop, house slot, street-line item or lattice point may enter (e.g. in front of a Waygate)."""
        if not hasattr(self,"_keep"): self._keep=[]
        self._keep.append((x,y,r))
    def _kept_out(self,x,y,extra=0):
        for (kx,ky,kr) in getattr(self,"_keep",()):
            if (kx-x)**2+(ky-y)**2<=(kr+extra)**2: return True
        return False
    def _near_any(self,pts,x,y,r):
        for (px,py) in pts:
            if abs(px-x)<=r and abs(py-y)<=r and (px-x)**2+(py-y)**2<=r*r: return True
        return False

    # ---- occupancy: every placed thing registers a footprint circle; later placements and the foliage lattice avoid it ----
    FOOT_RADIUS=[(re.compile(r"Waygate",re.I),None),(re.compile(r"Fireflies|Waterfall|WF_|Ground$|WaterPlane",re.I),0),
                 (re.compile(r"Chest|Potion|Crate|Ladder|Campfire|Stone|Lamp|Obelisk|Sign",re.I),140),(re.compile(r"Campfire",re.I),240),
                 (re.compile(r"Horse|Sheep|Deer|Fox|Goat|NPC|_BP$",re.I),200),(re.compile(r"AnimalPen|Pen$",re.I),900),
                 (re.compile(r"Well",re.I),300),(re.compile(r"Stall",re.I),320),(re.compile(r"GateHouse|Gate",re.I),520),(re.compile(r"House|Inn|Keep|Longhouse|Tower|Mill|Stable|Barracks|Temple|Church",re.I),760),
                 (re.compile(r"tree|Tree|Palm|Pine|Birch",re.I),320),(re.compile(r"Ruin|Arch|Bridge",re.I),520),
                 (re.compile(r"mountain|dragonmount|stone_of_tear|white_tower|Waterfall|Plane",re.I),0)]   # landmarks have their own pads/exclusions
    @classmethod
    def foot_radius(cls,path,label,scale=1.0):
        """Footprint radius (cm) of a hand-placed prop, from its label/asset name; None = handled elsewhere, 0 = no footprint."""
        key=f"{label} {str(path).split('/')[-1]}"; r=180
        for (rx,rr) in cls.FOOT_RADIUS:
            if rx.search(key):
                if rr is None: return None
                r=rr
        sc=scale if isinstance(scale,(int,float)) else max(scale[0],scale[1])
        return min(1400.0,r*sc)
    HOUSE_RX=re.compile(r"House|Inn|Keep|Longhouse|Mill|Stable|Barracks|Temple|Church|rand_house",re.I)
    def occupy(self,x,y,r,kind=""):
        if not hasattr(self,"_occ"): self._occ=[]
        if r>0: self._occ.append((x,y,r,kind))
    def occupy_rect(self,x,y,yaw,fp,kind="prop"):
        """An oriented rectangle footprint: fp=(x0,y0,x1,y1) in the prop's local frame (x forward)."""
        if not hasattr(self,"_occ_rect"): self._occ_rect=[]
        self._occ_rect.append((x,y,math.radians(yaw),fp,kind))
        self.occupy(x,y,max(abs(fp[0]),abs(fp[1]),abs(fp[2]),abs(fp[3]))*0.55,kind)   # a coarse circle for the lattice / quick tests
    def _rect_hit(self,x,y,r):
        for (ox,oy,th,fp,k) in getattr(self,"_occ_rect",()):
            dx,dy=x-ox,y-oy; c,s_=math.cos(th),math.sin(th); lx=dx*c+dy*s_; ly=-dx*s_+dy*c
            if fp[0]-r<lx<fp[2]+r and fp[1]-r<ly<fp[3]+r: return True
        return False
    def _occupied(self,x,y,r=0,ignore=None):
        """True when a circle of radius r at (x,y) overlaps a registered footprint, a keepout, a stair, a house slot or a street-line item."""
        for (ox,oy,orad,k) in getattr(self,"_occ",()):
            if k!=ignore and k!="building" and (ox-x)**2+(oy-y)**2<(orad+r)**2: return True
        if self._rect_hit(x,y,r): return True
        if self._kept_out(x,y,r): return True
        if self._near_any(getattr(self,"_stair_pts",()),x,y,r+200): return True
        if self._near_any(getattr(self,"_slot_pts",()),x,y,r+200): return True
        if self._near_any(getattr(self,"_line_pts",()),x,y,r+100): return True
        return False
    def _why(self,x,y,r):
        """Which occupancy rule blocks (x,y,r); '' when free."""
        for (ox,oy,orad,k) in getattr(self,"_occ",()):
            if k!="building" and (ox-x)**2+(oy-y)**2<(orad+r)**2: return f"occ:{k}@({ox:.0f},{oy:.0f},{orad:.0f})"
        if self._rect_hit(x,y,r): return "building"
        if self._kept_out(x,y,r): return "keepout"
        if self._near_any(getattr(self,"_stair_pts",()),x,y,r+200): return "stairs"
        if self._near_any(getattr(self,"_slot_pts",()),x,y,r+200): return "slot"
        if self._near_any(getattr(self,"_line_pts",()),x,y,r+100): return "line"
        return ""
    def register_props(self,spec):
        """Hand-placed props (chests, fires, animals, NPCs, wells, inns...) claim their footprints before anything else is placed."""
        for p in gv(spec,"props") or []:
            if p[0] not in ("mesh","actor","class","npc"): continue
            if p[0]=="npc": loc=p[2]; r=120.0   # just the spawn capsule; they wander anyway
            else: path=p[1]; label=p[2]; loc=p[3]; scale=p[5] if len(p)>5 else 1.0; r=self.foot_radius(path,label,scale)
            if p[0]=="mesh" and self.HOUSE_RX.search(f"{label} {str(path).split('/')[-1]}") and not re.search(r"Waygate|Stall|Lamp",label):
                sc=scale if isinstance(scale,(int,float)) else max(scale[0],scale[1]); fp=(-560*sc-60,-560*sc-60,740*sc+60,420*sc+60)
                yaw=p[4] if not isinstance(p[4],(list,tuple)) else p[4][1]
                self.occupy_rect(loc[0],loc[1],yaw,fp,"building"); continue
            if r: self.occupy(loc[0],loc[1],r,"prop")
    @staticmethod
    def _edges(poly):
        n=len(poly)
        for i in range(n):
            ax,ay=poly[i][0],poly[i][1]; bx,by=poly[(i+1)%n][0],poly[(i+1)%n][1]
            L=math.hypot(bx-ax,by-ay)
            if L>1: yield ax,ay,bx,by,L
    def _plaza_roads(self,spec,poly):
        """Road segments (2D) and the points where roads cross the plaza outline (street mouths), with their half widths."""
        segs=[]; mouths=[]
        for rd in spec["roads"]:
            if gv(rd,"kind","road")!="road": continue
            prof=gv(rd,"_profile") or rd["pts"]; halfw=gv(rd,"width",7)*gv(rd,"row_pitch",100)/2.0
            for i in range(len(prof)-1):
                a_=(prof[i][0],prof[i][1]); b_=(prof[i+1][0],prof[i+1][1])
                if a_==b_: continue
                segs.append((a_,b_,halfw))
                ia,ib=_poly_inside(a_[0],a_[1],poly),_poly_inside(b_[0],b_[1],poly)
                if ia!=ib:   # the crossing point, by bisection
                    lo,hi=a_,b_
                    for _ in range(12):
                        m=((lo[0]+hi[0])/2,(lo[1]+hi[1])/2)
                        if _poly_inside(m[0],m[1],poly)==ia: lo=m
                        else: hi=m
                    mouths.append((lo[0],lo[1],halfw))
        return segs,mouths
    @staticmethod
    def _near_road(segs,x,y,extra):
        for (a_,b_,hw) in segs:
            if _seg_dist(x,y,a_,b_)<hw+extra: return True
        return False
    @staticmethod
    def _near_mouth(mouths,x,y,extra):
        for (mx,my,hw) in mouths:
            if (mx-x)**2+(my-y)**2<(hw+extra)**2: return True
        return False
    def plaza_furniture(self,spec):
        """Orderly plaza furniture from each plaza's `scatter` list (replaces random scatter). Rules, in order:
        lamp posts at the inset corners and evenly along the edges; market stalls in rows (along the edges facing the
        middle, plus back-to-back rows down the middle of wide plazas, alternating colour variants); an obelisk in the
        middle; anything else on a jittered grid. Nothing sits on a road running through the plaza, within reach of a
        street mouth, or on another footprint: every item checks the occupancy registry and registers its own."""
        sets=[]
        for pi,pz in enumerate(gv(spec,"plazas") or []):
            sc=gv(pz,"scatter")
            if not sc: continue
            poly=[(p[0],p[1]) for p in pz["pts"]]; z=gv(pz,"z",0) or 0
            xs=[p[0] for p in poly]; ys=[p[1] for p in poly]; cx=sum(xs)/len(xs); cy=sum(ys)/len(ys)
            segs,mouths=self._plaza_roads(spec,poly)
            area=sum(poly[i][0]*poly[(i+1)%len(poly)][1]-poly[(i+1)%len(poly)][0]*poly[i][1] for i in range(len(poly)))
            inward=1 if area>0 else -1
            def normal(ax,ay,bx,by,L): return (-(by-ay)/L*inward,(bx-ax)/L*inward)   # unit normal pointing into the plaza
            lamps=[e for e in sc if "LampPost" in e[0]]; stalls=[e for e in sc if "MarketStall" in e[0]]
            obels=[e for e in sc if "Obelisk" in e[0]]; others=[e for e in sc if e not in lamps+stalls+obels]
            rectish=len(poly)<=8
            def free(x,y,r,mouth=350,road_r=None):
                return _poly_inside(x,y,poly) and _poly_dist(x,y,poly)>=r*0.8 and not self._near_road(segs,x,y,r if road_r is None else road_r) and not self._near_mouth(mouths,x,y,mouth) and not self._occupied(x,y,r)
            def emit(entry,pts,name):
                if not pts: return
                mesh,ratio,smin,smax,mats=entry; s_=(smin+smax)/2.0
                sets.append({"mesh":mesh,"mats":mats,"scale":(s_,s_,s_),"pts":pts,"name":f"plaza {pi} {name}"})
            # oriented frame of a rectangle-ish plaza: u along its longest edge, v across (inward from that edge)
            if rectish:
                ax,ay,bx,by,L=max(self._edges(poly),key=lambda e:e[4]); u=((bx-ax)/L,(by-ay)/L); v=normal(ax,ay,bx,by,L)
                us=[(p[0]-ax)*u[0]+(p[1]-ay)*u[1] for p in poly]; vs=[(p[0]-ax)*v[0]+(p[1]-ay)*v[1] for p in poly]
                u0,u1,v0,v1=min(us),max(us),min(vs),max(vs)
                def P(uu,vv): return (ax+u[0]*uu+v[0]*vv, ay+u[1]*uu+v[1]*vv)
            # a feature (well, monument, old tree...) near the middle of the plaza: stalls and lamps ring it
            size=max(max(xs)-min(xs),max(ys)-min(ys)); feature=None
            for (ox,oy,orad,k) in getattr(self,"_occ",()):
                if k=="prop" and math.hypot(ox-cx,oy-cy)<size*0.25 and _poly_inside(ox,oy,poly): feature=(ox,oy,orad); break
            # --- lamps: the inset corners, evenly along the edges (rect) or around the rim (circle), plus a ring around the feature
            for e in lamps[:1]:
                pts=[]; inset=240; cand=[]
                if rectish:
                    for vv in (v0+inset,v1-inset):
                        n_=max(1,int((u1-u0-2*inset)/1800))
                        cand+=[P(u0+inset+(u1-u0-2*inset)*k/n_,vv) for k in range(n_+1)]
                    for uu in (u0+inset,u1-inset):
                        n_=max(1,int((v1-v0-2*inset)/1800))
                        cand+=[P(uu,v0+inset+(v1-v0-2*inset)*k/n_) for k in range(1,n_)]
                else:
                    rr=sum(math.hypot(p[0]-cx,p[1]-cy) for p in poly)/len(poly)-inset; n_=max(6,int(2*math.pi*rr/1500))
                    cand+=[(cx+rr*math.cos(2*math.pi*k/n_),cy+rr*math.sin(2*math.pi*k/n_)) for k in range(n_)]
                if feature:
                    fr=feature[2]+260
                    cand+=[(feature[0]+fr*math.cos(math.pi/4+k*math.pi/2),feature[1]+fr*math.sin(math.pi/4+k*math.pi/2)) for k in range(4)]
                for (x,y) in cand:
                    if free(x,y,120,300): pts.append((x,y,z,0.0)); self.occupy(x,y,140,"lamp")
                    elif FURN_DEBUG: print("  lamp rejected",(round(x),round(y)),"inside",_poly_inside(x,y,poly),"edge",round(_poly_dist(x,y,poly)),"road",self._near_road(segs,x,y,120),"mouth",self._near_mouth(mouths,x,y,300),"occ",self._occupied(x,y,120))
                emit(e,self._thin(pts,700),"lamps")
            # --- stalls: SM_MarketStall is 250 deep (x) by 400 wide (y), open front on its -X side. Candidates, in order:
            #     a ring around the central feature (fronts to it), rows along the edges (fronts to the middle), then
            #     back-to-back rows down the middle of wide plazas; a candidate is taken only when its footprint is free.
            if stalls:
                fill=max(0.4,min(1.0,sum(e[1] for e in stalls)*20.0)); pitch=620/fill; inset=330; var=[[] for _ in stalls]; k=[0]
                def put(x,y,fx,fy):   # a stall at (x,y) whose front faces unit vector (fx,fy)
                    if not free(x,y,250,420,200):
                        if FURN_DEBUG: print("  stall rejected",(round(x),round(y)),"inside",_poly_inside(x,y,poly),"edge",round(_poly_dist(x,y,poly)),"road",self._near_road(segs,x,y,200),"mouth",self._near_mouth(mouths,x,y,420),"occ",self._why(x,y,250))
                        return
                    var[k[0]%len(stalls)].append((x,y,z,math.degrees(math.atan2(-fy,-fx)))); self.occupy(x,y,300,"stall"); k[0]+=1
                if feature:
                    fr=feature[2]+400; best=None
                    for n_ in range(4,max(5,int(2*math.pi*fr/pitch))+1):   # the count and phase that fit the most stalls around it
                        for ph in (0.0,0.5):
                            cand=[(2*math.pi*(i+ph)/n_) for i in range(n_)]
                            got=[th for th in cand if free(feature[0]+fr*math.cos(th),feature[1]+fr*math.sin(th),250,420,200)]
                            if best is None or len(got)>len(best): best=got
                    for th in best or []:
                        x,y=feature[0]+fr*math.cos(th),feature[1]+fr*math.sin(th); put(x,y,-math.cos(th),-math.sin(th))
                if rectish:
                    def layout(U0,U1,V0,V1,PP,VV):
                        """Stall seats for rows along the U axis at the V edges (and back to back down the middle of wide plazas)."""
                        usable=U1-U0-2*inset; n_=int((usable+(pitch-460))/pitch)   # 440-wide stalls with gaps between
                        rows=[(V0+inset,1),(V1-inset,-1)]                           # edge rows face inward
                        m=max(0,int((V1-V0-2*inset)/1300))                         # back-to-back rows down the middle (aisles >= 550)
                        for j in range(m):
                            vc=V0+inset+(V1-V0-2*inset)*(j+1)/(m+1); rows+=[(vc-165,-1),(vc+165,1)]
                        seats=[]
                        for (vv,sg) in rows:
                            best=None   # column phase per row: a street through the plaza should fall in a gap, not under a stall
                            for ph in (0.0,0.5):
                                cols=[U0+inset+(usable-n_*pitch)/2+(i+ph)*pitch for i in range(n_+1)]
                                cols=[c for c in cols if U0+inset-10<=c<=U1-inset+10]
                                got=[c for c in cols if free(*PP(c,vv),250,420,200)]
                                if best is None or len(got)>len(best): best=got
                            for uu in best or []:
                                x,y=PP(uu,vv); seats.append((x,y,VV[0]*sg,VV[1]*sg))
                        return seats
                    A=layout(u0,u1,v0,v1,P,v)
                    Bv=(u[0],u[1]); B=layout(v0,v1,u0,u1,lambda vv,uu:P(uu,vv),Bv)   # rows along the other axis
                    for (x,y,fx,fy) in (A if len(A)>=len(B) else B): put(x,y,fx,fy)
                else:
                    rr=sum(math.hypot(p[0]-cx,p[1]-cy) for p in poly)/len(poly)-inset; n_=max(4,int(2*math.pi*rr/pitch))
                    for i in range(n_):
                        th=2*math.pi*(i+0.5)/n_; x,y=cx+rr*math.cos(th),cy+rr*math.sin(th); put(x,y,-math.cos(th),-math.sin(th))
                for e,pts in zip(stalls,var): emit(e,pts,"stalls")
            # --- a monument in the middle when the middle is free
            for e in obels[:1]:
                if free(cx,cy,220,300): emit(e,[(cx,cy,z,0.0)],"monument"); self.occupy(cx,cy,260,"monument")
            # --- everything else: a jittered grid, density from the ratio
            for oi,e in enumerate(others):
                mesh,ratio,smin,smax,mats=e; pitch=max(300.0,100.0/math.sqrt(max(ratio,1e-4))); pts=[]; r_=160; gi=0
                for x in self._frange(min(xs)+pitch/2,max(xs),pitch):
                    for y in self._frange(min(ys)+pitch/2,max(ys),pitch):
                        gi+=1; px=x+(_hash(gi,oi,11)-0.5)*pitch*0.5; py=y+(_hash(gi,oi+7,13)-0.5)*pitch*0.5
                        if not free(px,py,r_,250): continue
                        pts.append((px,py,z,_hash(gi,oi+3,17)*360.0)); self.occupy(px,py,r_+40,"other")
                emit(e,pts,mesh.split("/")[-1].split(".")[0])
        spec["furniture_sets"]=sets
        return sets
    @staticmethod
    def _frange(a,b,step):
        x=a
        while x<b: yield x; x+=step
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
                if (i,j) in self.cellmat or self.near_paved(x,y,self.cell*0.75) or self._kept_out(x,y): continue
                if self._near_any(getattr(self,"_slot_pts",()),x,y,330) or self._near_any(getattr(self,"_line_pts",()),x,y,170) or self._near_any(getattr(self,"_stair_pts",()),x,y,220): continue
                if self._occ_hit(x,y,120): continue
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
    # Waygates: a level pad under the arch and its approach, nothing placed within reach of the doorway
    for p in gv(spec,"props") or []:
        if p[0]=="mesh" and str(p[2]).startswith("Waygate_"):
            (x,y)=p[3][0],p[3][1]; yaw=p[4]; fx,fy=-math.cos(math.radians(yaw)),-math.sin(math.radians(yaw))   # the opening faces -X at yaw 0
            cx,cy=x+fx*250,y+fy*250
            R.flatten(("circle",cx,cy,850),None,True); R.keepout(cx,cy,950)
            spec["exclusions"]=list(gv(spec,"exclusions") or [])+[(cx,cy,950,950)]
    R.register_props(spec)   # chests, fires, animals, NPCs, wells and inns claim their ground first
    stairs=R.carve_roads(spec)
    for rd in spec["roads"]:
        if gv(rd,"nocarve",False) and gv(rd,"rel",False):
            p0=rd["pts"][0]; z0=R.zat(p0[0],p0[1]); rd["pts"]=[(p[0],p[1],z0+(p[2] if len(p)>2 else 0)) for p in rd["pts"]]
    R.carve_plazas(spec)
    R.house_pads(spec)
    R.finalize_stairs()
    # every flight is also a PCG exclusion (axis-aligned box): keeps foliage, houses and wall tiles out of it
    for st in getattr(R,"_stairs",[]):
        (fx_,fy_)=st["foot"]; (ax_,ay_)=st["axis"]; L_=st["run"]+R.cell; cx_=fx_+ax_*(L_/2-R.cell*0.3); cy_=fy_+ay_*(L_/2-R.cell*0.3)
        hx_=L_/2+60 if ax_ else st["halfw"]+40; hy_=st["halfw"]+40 if ax_ else L_/2+60
        spec["exclusions"]=list(gv(spec,"exclusions") or [])+[(cx_,cy_,hx_,hy_)]
    R.road_props(spec)
    R.plaza_furniture(spec)
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
    # dialogue rows are numbered (lines() makes <prefix>_1, _2, ...): an NPC starts on the first line
    first=row if re.match(r".*_\d+$",row) else f"{row}_1"
    props={"Name":name,"CurrentRowName":first,"WanderRadius":float(radius)}
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
