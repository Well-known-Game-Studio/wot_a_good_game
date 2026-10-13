import time
def run():
    towns=TOWNS
    out={}
    for town in towns:
        t0=time.time()
        try:
            spec=CITY_SPECS[town]()
            r=make_city(spec,town,["Tam_NPC","Lan_NPC","Ground"])
            out[town]={"ok":True,"nodes":r["build"]["info"]["nodes"],"msgs":r["build"]["msgs"],"placed":r["build"]["placed"],"s":round(time.time()-t0)}
        except Exception as e:
            out[town]={"ok":False,"err":str(e)[:400],"s":round(time.time()-t0)}
    return out
