# Cerebrum

> OpenWolf's learning memory. Updated automatically as the AI learns from interactions.
> Do not edit manually unless correcting an error.
> Last updated: 2026-07-03

## User Preferences

<!-- How the user likes things done. Code style, tools, patterns, communication. -->

## Key Learnings

- **Project:** ReXGlue080plume
- **Description:** > [!CAUTION]

## Do-Not-Repeat

<!-- Mistakes made and corrected. Each entry prevents the same mistake recurring. -->
<!-- Format: [YYYY-MM-DD] Description of what went wrong and what to do instead. -->

## Decision Log

<!-- Significant technical decisions with rationale. Why X was chosen over Y. -->

## Key Learnings (2026-07-03 session 6)
- IDA MCP server mapping: ida37 = FM2 (FM2.xex.i64), ida38 = Sonic Unleashed, ida39 = MM3. Check server_health first.
- Handoff-doc mechanism models can be wrong: pt10's "retire gated on a D3D fence" was disproven by decompiling the counter writers. Before building a fix on a counter's meaning, find its writers in IDA (py_eval scan for stores at the displacement, remembering embedded structs shift displacements: pool at renderThread+2224 => +120 becomes 0x928, at wrapper+24 => 0x90).
- FM2 deferred pool pacing: game loop is vblank-locked via PulseEvent(829C24C0); plume replaces it with StartGatePulseThreadOnce at fm2_plume_gate_pulse_hz (default 1000 = uncapped producer). Any backlog/drop symptom in the 0x4001CA20 pool should FIRST be checked against the pulse rate.
- fm2 --log_file TRUNCATES (recreates) the target file, it does not append. Offset-based tailing from a pre-launch length is wrong across launches; use separate log files per run instead.

## Do-Not-Repeat (2026-07-03)
- Do NOT re-run the fm2_plume_gate_pulse_hz pacing A/B (60/30 Hz) expecting textured UI: user-verified NO visual difference; default stays 1000. The pool/pacing mechanism findings (see docs/FM2-handoff-2026-07-02-session5.md session 6) remain valid telemetry, but pacing alone does not fix the black textured UI.

## Key Learnings (2026-07-04 — shader cache internals, black-texture root cause SETTLED)
- **FM2/generated/shader_cache.cpp stores DXIL *libraries*, not standalone shaders.** Each DXBC container has parts `SFI0,VERS,RDAT,HASH,DXIL` and NO `PSV0/ISG1/OSG1`. Runtime `LoadShader`→`LinkShaderLibrary(lib, dxil_off, type, specializedValue)` (pipeline.cpp) links per `spec_constants_mask` into a NEW standalone DXBC with a NEW DxilShaderHash. ⇒ a RenderDoc-presented DXIL hash is a LINK-TIME output and will NEVER match any cache entry's hash/bytes. Don't conclude "not in cache ⇒ fallback/miss" from a hash mismatch.
- **To tell if a cached shader samples textures, parse its RDAT ResourceTable** (NOT the DXIL bitcode — dx.op intrinsic names are not ASCII-visible there). RDAT: hdr {u32 ver,u32 partcount} + u32 part offsets; part type 3 = ResourceTable {u32 count,u32 stride=32} then records {class,kind,id,space,lb,ub,nameoff}; class 0=SRV,1=UAV,2=CBV,3=Sampler; names in part type 1 StringBuffer. Texture shader = has Sampler + SRV(Texture2D) `g_*DescriptorHeap`; color-only = CBV `SharedConstants` only. Scripts: scratchpad check_bg.py / rdat.py (zstd CLI decompresses g_compressedDxilCache → 1215408 B).
- **Black-texture root cause SETTLED:** bg guest PS `9E93B37448CA0172` is genuinely color-only (RDAT = CBV only) and links to presented `1b2db2c8`. XenosRecomp translation is CORRECT. The bug is plume handing the WRONG guest PS to textured draws (selection/context-routing), NOT translation, NOT texture-binding, NOT drop-latch/pacing. Investigate what writes the context PS slot ctx+0x307C per-draw.

## Key Learnings (2026-07-05 — CAR flashing = XenosRecomp translation, constants exonerated)
- **CAR flashing polys root cause = XenosRecomp LOG/RECIP clamp mistranslation, NOT constants.** VS `4fff9681` does `1/dot(c32)` + `log2(sat(..))`. c32 (VertexShaderConstants byte 512) is authentically **~0 in BOTH backends** (RenderDoc: bad EID1858 vs good EID1157, c32≈0 each) — so the entire multi-session constant-delivery theory is EXONERATED. Xenos hw + runtime dxbc_translator resolve log2(0)/rcp(0) to finite and render the car correctly; XenosRecomp emits `max(x,-FLT_MAX)/min(x,+FLT_MAX)` which preserves −FLT_MAX poison → COLOR0/position explode. Fix = XenosRecomp LOGc/RECIP lowering. Full: [[project_fm2_car_flashing_xenosrecomp_2026-07-05]].
- **PM4 VS-constant apply DOES fire (steady state pm4Valid=1, applyCalls→100k+).** The "apply never fires / pm4Valid=0" was a diagnostic artifact: FM2_VSC32 capped on the first 80/160 !noPos draws, which are all BOOT draws with g_liveVsFloatConstants==null, before the menu loads. Gate such per-draw diagnostics on `g_liveVsFloatConstants != nullptr` and sample forever (first N + 1-in-K), never a low one-shot cap, or you measure the wrong draws.
- **RenderDoc debug_vertex is the fastest way to localize shader NaN/Inf** — `nan_or_inf_steps` + `full_trace` gives the exact DXIL op (log2/rcp) and the register data-flow. Needs working disassembly (see MCP fix). Prefer the **DXBC/DXIL** disassembly target for DXIL; AMD ISA targets can't decode it.

## Do-Not-Repeat (2026-07-05)
- Do NOT re-litigate FM2 car/lit-mesh constants (live-file vs PM4 shadow vs fresh-vs-accum vs cross-context). Settled: constants (incl. degenerate c32≈0) are authentic; the bug is XenosRecomp shader math. Constant register-alignment across xenos(dxbc_translator) vs plume(XenosRecomp) is apples-to-oranges — use RenderDoc matched EIDs + debug_vertex instead.
- Editing `renderdoc-mcp/renderdoc_mcp/*.py` alone does NOT fix the live MCP: it runs from bundle `x64/Development/mcp/renderdoc_mcp` as a qrenderdoc child on :8765. Sync files there AND toggle MCP off/on in qrenderdoc (also clears the stale replay device that returns disassembly garbage). See [[reference_renderdoc_mcp_disasm_fix]].
