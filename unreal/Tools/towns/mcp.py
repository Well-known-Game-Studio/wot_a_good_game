#!/usr/bin/env python3
"""Minimal MCP (streamable HTTP) client for the UE 5.8 editor server.
Usage:
  mcp.py list                      -> list tools (name + first line of description)
  mcp.py schema <tool>             -> full schema for one tool
  mcp.py call <tool> '<json args>' -> call a tool
  mcp.py raw <method> '<json>'     -> raw JSON-RPC
"""
import sys, json, os, urllib.request, urllib.error

URL = os.environ.get("UE_MCP_URL", "http://127.0.0.1:8000/mcp")
SESS = os.path.expanduser("~/.ue_mcp_session")
_id = [0]

def _post(payload, session=None):
    hdr = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
    if session: hdr["Mcp-Session-Id"] = session
    req = urllib.request.Request(URL, data=json.dumps(payload).encode(), headers=hdr, method="POST")
    with urllib.request.urlopen(req, timeout=1800) as r:
        sid = r.headers.get("Mcp-Session-Id")
        body = r.read().decode(errors="replace")
        ctype = r.headers.get("Content-Type", "")
    if "text/event-stream" in ctype:
        msgs = []
        for line in body.splitlines():
            if line.startswith("data:"):
                try: msgs.append(json.loads(line[5:].strip()))
                except Exception: pass
        # return the last response-type message
        resp = [m for m in msgs if "result" in m or "error" in m]
        return (resp[-1] if resp else (msgs[-1] if msgs else None)), sid
    return (json.loads(body) if body.strip() else None), sid

def rpc(method, params=None, session=None, notify=False):
    _id[0] += 1
    p = {"jsonrpc": "2.0", "method": method}
    if params is not None: p["params"] = params
    if not notify: p["id"] = _id[0]
    return _post(p, session)

def session():
    if os.path.exists(SESS):
        sid = open(SESS).read().strip()
        try:
            r, _ = rpc("ping", {}, sid)
            if r is not None and "error" not in r: return sid
        except Exception: pass
    r, sid = rpc("initialize", {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "claude-code", "version": "1.0"}})
    if "error" in (r or {}): raise SystemExit(json.dumps(r))
    try: rpc("notifications/initialized", {}, sid, notify=True)
    except urllib.error.HTTPError: pass
    if sid: open(SESS, "w").write(sid)
    return sid

def list_tools(sid):
    tools, cursor = [], None
    while True:
        r, _ = rpc("tools/list", {"cursor": cursor} if cursor else {}, sid)
        res = r.get("result", {})
        tools += res.get("tools", [])
        cursor = res.get("nextCursor")
        if not cursor: break
    return tools

def main():
    a = sys.argv[1:]
    sid = session()
    if a[0] == "list":
        for t in list_tools(sid):
            print(f"{t['name']}: {(t.get('description') or '').strip().splitlines()[0][:150] if t.get('description') else ''}")
    elif a[0] == "schema":
        for t in list_tools(sid):
            if t["name"] == a[1]: print(json.dumps(t, indent=1)); break
        else: print("not found")
    elif a[0] == "call":
        args = json.loads(a[2]) if len(a) > 2 else {}
        r, _ = rpc("tools/call", {"name": a[1], "arguments": args}, sid)
        if isinstance(r, dict) and "error" in r and "result" not in r:
            print(json.dumps(r["error"], indent=1)); sys.exit(2)
        out = r.get("result", r)
        # unwrap text content
        if isinstance(out, dict) and "content" in out:
            for c in out["content"]:
                if c.get("type") == "text":
                    txt = c["text"]; saved = False
                    if '"mimeType":"image/' in txt[:400] or '"mimeType": "image/' in txt[:400]:
                        import re as _re, base64, time as _t
                        mm = _re.search(r'"data":\s*"([A-Za-z0-9+/=]+)"', txt)
                        if mm:
                            fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"img_{int(_t.time()*1000)}.png")
                            open(fn, "wb").write(base64.b64decode(mm.group(1))); print("[image saved]", fn); saved = True
                    if not saved: print(txt)
                elif c.get("type") == "image":
                    import base64, time as _t
                    fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"img_{int(_t.time()*1000)}.{ 'png' if 'png' in c.get('mimeType','') else 'jpg'}")
                    open(fn, "wb").write(base64.b64decode(c["data"])); print("[image saved]", fn)
                else: print(json.dumps(c)[:500])
            if out.get("isError"): print("[isError]"); sys.exit(1)
            if "structuredContent" in out:
                sc = out["structuredContent"]; rv = sc.get("returnValue") if isinstance(sc, dict) else None
                if isinstance(rv, dict) and "data" in rv and "mimeType" in rv and str(rv["mimeType"]).startswith("image/"):
                    import base64, time as _t
                    fn = os.path.join(os.path.dirname(os.path.abspath(__file__)), f"img_{int(_t.time()*1000)}.png")
                    open(fn, "wb").write(base64.b64decode(rv["data"])); print("[image saved]", fn)
                else: print(json.dumps(sc, indent=1))
        else: print(json.dumps(out, indent=1))
    elif a[0] == "raw":
        r, _ = rpc(a[1], json.loads(a[2]) if len(a) > 2 else {}, sid)
        print(json.dumps(r, indent=1))
main()
