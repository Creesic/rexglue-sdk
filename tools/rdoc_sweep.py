"""Sweep every draw in a RenderDoc capture for the PGR4 bug classes seen so far.

Run with the RenderDoc source-build python:
  python rdoc_sweep.py <capture.rdc> [report.txt]
"""
import os, sys, struct, math, collections, time

D = r"C:\Users\Tera\Documents\GitHub\renderdoc\x64\Development"
os.add_dll_directory(D)
sys.path.insert(0, os.path.join(D, "pymodules"))
import renderdoc as rd

path = sys.argv[1]
out_path = sys.argv[2] if len(sys.argv) > 2 else None
out = open(out_path, "w", encoding="utf-8") if out_path else sys.stdout

def log(*a):
    print(*a, file=out)
    out.flush()

rd.InitialiseReplay(rd.GlobalEnvironment(), [])
cap = rd.OpenCaptureFile()
res = cap.OpenFile(path, "", None)
if res != rd.ResultCode.Succeeded:
    raise SystemExit(f"open failed: {res}")
res, ctrl = cap.OpenCapture(rd.ReplayOptions(), None)
if res != rd.ResultCode.Succeeded:
    raise SystemExit(f"replay failed: {res}")

names = {r.resourceId: r.name for r in ctrl.GetResources()}
textures = {t.resourceId: t for t in ctrl.GetTextures()}

def tex_desc(rid):
    t = textures.get(rid)
    if t is None:
        return names.get(rid, str(rid))
    fmt = t.format
    return (f"{names.get(rid, rid)} {t.width}x{t.height}"
            + (f"x{t.arraysize}" if t.arraysize > 1 else "")
            + (f" d{t.depth}" if t.depth > 1 else "")
            + f" {fmt.Name()}")

def is_null_texture(rid):
    # The three host null textures are 1x1 single-channel 8-bit images.
    t = textures.get(rid)
    return (t is not None and t.width == 1 and t.height == 1 and t.depth == 1
            and t.format.compByteWidth == 1 and t.format.compCount == 1)

draws = []
def walk(actions):
    for a in actions:
        if a.flags & rd.ActionFlags.Drawcall:
            draws.append(a)
        walk(a.children)
walk(ctrl.GetRootActions())
log(f"capture {os.path.basename(path)}: {len(draws)} draws")

zero_tex_cache = {}
def texture_all_zero(rid):
    if rid in zero_tex_cache:
        return zero_tex_cache[rid]
    t = textures.get(rid)
    result = None
    if t is not None and t.byteSize <= (8 << 20):
        try:
            data = ctrl.GetTextureData(rid, rd.Subresource(0, 0, 0))
            result = (len(data) > 0 and data.count(0) == len(data))
        except Exception:
            result = None
    zero_tex_cache[rid] = result
    return result

# class -> key -> list of eids
flags = collections.defaultdict(lambda: collections.defaultdict(list))
details = {}
t0 = time.time()
for n, a in enumerate(draws):
    eid = a.eventId
    ctrl.SetFrameEvent(eid, False)
    st = ctrl.GetPipelineState()
    vs = st.GetShader(rd.ShaderStage.Vertex)
    ps = st.GetShader(rd.ShaderStage.Pixel)
    key = f"vs={names.get(vs, vs)} ps={names.get(ps, ps)}"

    # --- vertex streams ---------------------------------------------------
    vbs = st.GetVBuffers()
    attrs = st.GetVertexInputs()
    used_slots = set()
    int_attrs = []
    for at in attrs:
        if not at.used:
            continue
        used_slots.add(at.vertexBuffer)
        ct = at.format.compType
        if ct in (rd.CompType.UInt, rd.CompType.SInt) and at.format.compByteWidth <= 2:
            int_attrs.append(f"{at.name}:{at.format.Name()}")
    for slot in sorted(used_slots):
        if slot >= len(vbs) or vbs[slot].resourceId == rd.ResourceId.Null() or vbs[slot].byteSize == 0:
            flags["A. used vertex slot unbound"][key + f" slot{slot}"].append(eid)
        elif vbs[slot].byteStride == 0:
            flags["B. stride-0 vertex slot"][key + f" slot{slot}"].append(eid)
    if int_attrs:
        flags["C. 16-bit integer vertex attributes"][key + " " + ",".join(int_attrs)].append(eid)

    # --- textures actually accessed --------------------------------------
    for stage in (rd.ShaderStage.Vertex, rd.ShaderStage.Pixel):
        try:
            used = st.GetReadOnlyResources(stage, True)
        except TypeError:
            used = st.GetReadOnlyResources(stage)
        for u in used:
            rid = u.descriptor.resource
            if rid == rd.ResourceId.Null():
                flags["D. null descriptor sampled"][key + f" {stage}"].append(eid)
                continue
            if is_null_texture(rid):
                flags["D. null descriptor sampled"][key + f" {stage} {tex_desc(rid)}"].append(eid)
                continue
            if rid in textures and texture_all_zero(rid):
                flags["E. all-zero texture sampled"][key + f" {stage} {tex_desc(rid)}"].append(eid)

    # --- post-VS positions -------------------------------------------------
    try:
        mesh = ctrl.GetPostVSData(0, 0, rd.MeshDataStage.VSOut)
    except Exception as e:
        mesh = None
    if mesh is not None and mesh.vertexResourceId != rd.ResourceId.Null() and mesh.numIndices > 0:
        count = min(mesh.numIndices, 4096)
        stride = mesh.vertexByteStride
        data = ctrl.GetBufferData(mesh.vertexResourceId, mesh.vertexByteOffset, stride * count)
        xs, ys, behind, nan = [], [], 0, 0
        for i in range(min(count, len(data) // stride)):
            x, y, z, w = struct.unpack_from("<4f", data, i * stride)
            if any(math.isnan(v) or math.isinf(v) for v in (x, y, z, w)):
                nan += 1
                continue
            if w <= 1e-6:
                behind += 1
                continue
            xs.append(x / w)
            ys.append(y / w)
        total = nan + behind + len(xs)
        if total:
            if nan == total:
                flags["F. post-VS all NaN/inf"][key].append(eid)
            elif behind == total:
                flags["G. post-VS all behind camera"][key].append(eid)
            elif xs:
                ex, ey = max(xs) - min(xs), max(ys) - min(ys)
                inside = sum(1 for x, y in zip(xs, ys) if -1 <= x <= 1 and -1 <= y <= 1)
                if len(xs) >= 3 and ex < 0.004 and ey < 0.004:
                    flags["H. post-VS collapsed to a point"][key].append(eid)
                    details[eid] = f"ndc=({min(xs):.3f},{min(ys):.3f}) w>0:{len(xs)}"
                elif inside == 0 and len(xs) >= 3:
                    flags["I. post-VS entirely off screen"][key].append(eid)
                    details[eid] = f"ndc x[{min(xs):.2f},{max(xs):.2f}] y[{min(ys):.2f},{max(ys):.2f}]"
    if n % 100 == 0:
        print(f"  {n}/{len(draws)} draws, {time.time()-t0:.0f}s", file=sys.stderr, flush=True)

log("")
for cls in sorted(flags):
    groups = flags[cls]
    total = sum(len(v) for v in groups.values())
    log(f"== {cls}: {total} draws in {len(groups)} shader groups")
    for k, eids in sorted(groups.items(), key=lambda kv: -len(kv[1]))[:12]:
        sample = ", ".join(str(e) for e in eids[:6])
        extra = details.get(eids[0], "")
        log(f"   {len(eids):5d}  {k}  e.g. {sample}  {extra}")
log("")
log(f"done in {time.time()-t0:.0f}s")
ctrl.Shutdown()
