#!/usr/bin/env python3
"""Local (no editor) audit of the auto-placed stairs of every town: builds the spec + relief like --dry and checks each
flight against the terrain grid, the lattice (trees/bushes), house slots, street-line props and wall roads.
usage: audit_stairs.py [Town ...]"""
import json, math, os, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from run_town import base, TOWNS
Q = 40
def audit(town, g, verbose=False):
    s = g["CITY_SPECS"][town](); R = s["_relief"]; cell = R.cell
    stairs = [p for p in s["props"] if "Stairs_" in str(p[2])]
    lat = s.get("lattice") or []
    slots = [sl for k, v in (s.get("house_slots") or {}).items() for sl in v]
    lines = [pt for ln in (s.get("line_sets") or []) for pt in ln["pts"]]
    walls = [rd for rd in s["roads"] if rd.get("kind") == "wall"]
    bad = {"top_not_hi": 0, "run_not_lo": 0, "lattice": 0, "slots": 0, "lines": 0, "wall": 0}
    for p in stairs:
        (fx, fy, zlo) = p[3]; yaw = p[4]; sc = p[5]; S = int(str(p[1]).split("SM_Stairs")[-1].split(".")[0]); zhi = zlo + S
        ax, ay = round(math.cos(math.radians(yaw))), round(math.sin(math.radians(yaw)))
        Lrun = 2 * S; piv = 20; halfw = 250.0 * (sc[1] if isinstance(sc, (tuple, list)) else 1.0) + 15
        footx, footy = fx - ax * piv, fy - ay * piv; ex, ey = fx + ax * (Lrun - piv), fy + ay * (Lrun - piv)
        def inside(x, y, pad=0):
            a = (x - footx) * ax + (y - footy) * ay; c = abs(-(x - footx) * ay + (y - footy) * ax)
            return -pad <= a <= Lrun + pad and c <= halfw + pad
        # terrain: just past the top must be the upper level, under the run the lower level
        issues = []
        for c in (-halfw * 0.6, 0, halfw * 0.6):
            tx, ty = ex + ax * cell * 0.6 - ay * c, ey + ay * cell * 0.6 + ax * c
            if abs(R.zat(tx, ty) - zhi) > Q: issues.append("top_not_hi"); break
        for a in range(60, int(Lrun) - 20, 100):
            for c in (-halfw * 0.6, 0, halfw * 0.6):
                tx, ty = footx + ax * a - ay * c, footy + ay * a + ax * c
                if R.zat(tx, ty) - zlo > Q: issues.append("run_not_lo"); break
            else: continue
            break
        if any(inside(x, y) for (x, y, z) in lat): issues.append("lattice")
        if any(inside(x, y, 200) for sl in slots for (x, y) in [sl[:2]]): issues.append("slots")
        if any(inside(x, y, 60) for pt in lines for (x, y) in [pt[:2]]): issues.append("lines")
        for rd in walls:
            for (x, y, *rest) in rd.get("_profile") or rd["pts"]:
                if inside(x, y, 40): issues.append("wall"); break
            else: continue
            break
        for k in set(issues): bad[k] += 1
        if verbose and issues: print(f"   {p[2]} S={S} yaw={yaw:.0f} at ({fx:.0f},{fy:.0f}): {', '.join(sorted(set(issues)))}")
    print(f"{town:13s} stairs {len(stairs):3d}  " + "  ".join(f"{k} {v}" for k, v in bad.items()))
    return bad
if __name__ == "__main__":
    g = {"execute_tool": lambda n, a: (_ for _ in ()).throw(RuntimeError("no editor")), "math": math, "json": json, "time": time}
    exec(base(), g)
    towns = [a for a in sys.argv[1:] if not a.startswith("-")] or TOWNS
    for t in towns: audit(t, g, verbose="-v" in sys.argv)
