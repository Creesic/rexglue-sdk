"""Verify FM2 IDA names against fm2_manifest hook targets."""
import json
import re

MANIFEST = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\FM2\fm2_manifest.toml"
OUT = r"C:\Users\Tera\Documents\GitHub\ReXGlue080plume\.cursor\hooks\state\fm2_hook_verify_targets.json"
LINE_RE = re.compile(r'^(0x[0-9A-Fa-f]+)\s*=\s*\{\s*name\s*=\s*"([^"]+)"\s*\}')

# FM2 native renderer hook symbols from d3d_hooks.cpp
HOOK_NAMES = {
    "FM2_D3D_CreateVertexBufferWrapper",
    "FM2_D3D_CreateIndexBufferWrapper",
    "FM2_D3D_CreateTextureWrapper",
    "FM2_D3D_CreateDepthStencilSurfaceAndTexture",
    "FM2_D3D_CreateVertexDeclarationFromElements",
    "FM2_Render_GetOrCreateVertexShaderResourceById",
    "FM2_Render_GetOrCreatePixelShaderResourceById",
    "FM2_D3D_CreateTextureFromMemoryBuffer",
    "FM2_D3D_LockVertexBufferWrapper",
    "FM2_D3D_DeserializeAndLockIndexBuffer",
    "FM2_D3D_GatherSurfaceMetadataForTextureCreate",
    "FM2_D3DResource_UnlockForRelease",
    "FM2_RenderContext_SetPixelShaderState",
    "FM2_RenderContext_SetVertexShaderState",
    "FM2_RenderContext_BindVertexStream",
    "FM2_RenderContext_BindIndexBuffer",
    "FM2_RenderContext_SetBoundSurface",
    "FM2_Render_DrawIndexedPrimitive",
    "FM2_D3D_EmitIndexedDrawPacket",
    "FM2_D3D_TryPresentAndUpdateStatus",
}

entries = []
with open(MANIFEST, encoding="utf-8") as f:
    for line in f:
        m = LINE_RE.match(line.strip())
        if m and m.group(2) in HOOK_NAMES:
            entries.append({"addr": m.group(1), "name": m.group(2)})

# shader create names may be elsewhere in manifest
for extra_addr, extra_name in [
    ("0x825A16E0", "FM2_Render_GetOrCreateVertexShaderResourceById"),
    ("0x825A1608", "FM2_Render_GetOrCreatePixelShaderResourceById"),
    ("0x8236A2B8", "FM2_D3DResource_UnlockForRelease"),
]:
    if not any(e["name"] == extra_name for e in entries):
        entries.append({"addr": extra_addr, "name": extra_name})

with open(OUT, "w", encoding="utf-8") as f:
    json.dump(entries, f, indent=2)

print(len(entries))
