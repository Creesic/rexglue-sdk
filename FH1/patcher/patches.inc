// FH1 patches — anchor-based form of the legacy rexglue/run-fh1/patches/*.patch.
//
// Each entry replaces one .patch file (or several merged where they refine
// the same insertion site). The patch applies if:
//   1. The DEFINE_REX_FUNC(<function>) body exists in the target file.
//   2. The `marker` string is NOT already present inside that body (idempotent).
//   3. The `match` substring appears EXACTLY ONCE inside that body.
//
// Adding a new patch:
//   1. Confirm the failure case + capture (memory file).
//   2. Pick a unique `marker` — convention is the static atomic counter name
//      (`fh1_<short>_<reason>_`); never reuse one across patches.
//   3. Copy the exact instruction-sequence anchor from the codegen body into
//      `match` (preserve leading tabs).
//   4. Build the replacement into `with` — typically the original anchor
//      wrapped in an `if (cond) { ... } else { static atomic counter; ... }`.
//
// Why these can't fold into REX_HOOK_RAW like leaf_guards.cpp:
//   These guards intercept a specific PPC instruction sequence inside a
//   function body. REX_HOOK_RAW wraps function boundaries; there's no
//   in-body callable seam, so the guard has to be inserted at codegen-text
//   level.
//
// Historical consolidation (2026-05-12):
//   - 0003 + 0014/0017 → GetSupportingFeature inner-loop guard (final 0x10000 threshold)
//   - 0010 + 0014/0017 → GetSupportingFeature 2nd-loop guard (final 0x10000 threshold)
//   - 0005 + 0011 + 0013 + 0016 + 0017 → GetSupportingPolyFeature entry guard (final state)
//   - 0015 + 0017                       → CGame::Update r28 guard (final 0x10000 threshold)
//   - 0009 split into two entries (entry guard + post-_Buy guard) — same source patch
//
// Patches 0014 and 0017 themselves are not separate entries: they're threshold
// refinements of the patches above and have been baked into the `with` text.

#include "patcher.h"

namespace fh1::patcher {

namespace {

constexpr Patch g_patches[] = {

// =========================================================================
// 0001: CConvexHullConvexHull::Intersect — NULL shape/table/verts guard
// =========================================================================
{
    .id       = "0001",
    .file     = "fh1_recomp.78.cpp",
    .function = "sub_82D44B90",
    .marker   = "fh1_intersect_null_hits_",
    .match =
        "\t// lwz r28,36(r8)\n"
        "\tctx.r28.u64 = REX_LOAD_U32(ctx.r8.u32 + 36);\n"
        "\t// addi r7,r1,320\n"
        "\tctx.r7.s64 = ctx.r1.s64 + 320;",
    .with =
        "\t// lwz r28,36(r8)\n"
        "\tctx.r28.u64 = REX_LOAD_U32(ctx.r8.u32 + 36);\n"
        "\t// FH1 LAYER B 0001: skip Intersect when shape pointers or their data tables are NULL.\n"
        "\t{\n"
        "\t\tconst uint32_t shape_a = ctx.r29.u32;\n"
        "\t\tconst uint32_t shape_b = ctx.r28.u32;\n"
        "\t\tbool bail = (shape_a == 0) || (shape_b == 0);\n"
        "\t\tuint32_t a_table = 0, a_verts = 0, b_table = 0, b_verts = 0;\n"
        "\t\tif (!bail) {\n"
        "\t\t\ta_table = REX_LOAD_U32(shape_a + 84);\n"
        "\t\t\ta_verts = REX_LOAD_U32(shape_a + 92);\n"
        "\t\t\tb_table = REX_LOAD_U32(shape_b + 84);\n"
        "\t\t\tb_verts = REX_LOAD_U32(shape_b + 92);\n"
        "\t\t\tbail = (a_table == 0) || (a_verts == 0) || (b_table == 0) || (b_verts == 0);\n"
        "\t\t}\n"
        "\t\tif (bail) {\n"
        "\t\t\tstatic std::atomic<uint32_t> fh1_intersect_null_hits_{0};\n"
        "\t\t\tconst auto _n = fh1_intersect_null_hits_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] ConvexHullConvexHull::Intersect NULL (pair={:#x} shape_a={:#x} shape_b={:#x} a_table={:#x} a_verts={:#x} b_table={:#x} b_verts={:#x}); skipping. count={}\", ctx.r8.u32, shape_a, shape_b, a_table, a_verts, b_table, b_verts, _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\tctx.r3.s64 = 0;\n"
        "\t\t\tgoto loc_82D451B0;\n"
        "\t\t}\n"
        "\t}\n"
        "\t// addi r7,r1,320\n"
        "\tctx.r7.s64 = ctx.r1.s64 + 320;",
},

// =========================================================================
// 0003 + 0014/0017: GetSupportingFeature inner-loop NULL guard
// Threshold = 0x10000 per patch 0017 (bumped from 0x1000 to catch garbage
// values like 0x1BD8, 0x4063 observed in the wild).
// =========================================================================
{
    .id       = "0003+0014+0017",
    .file     = "fh1_recomp.78.cpp",
    .function = "sub_82D3DB00",
    .marker   = "fh1_gsf_inner_null_hits_",
    .match =
        "\t// lwz r9,92(r31)\n"
        "\tctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);\n"
        "\t// lbzx r8,r11,r30\n"
        "\tctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r30.u32);",
    .with =
        "\t// lwz r9,92(r31)\n"
        "\tctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 92);\n"
        "\t// FH1 LAYER B 0003+0014+0017+0026: inner-loop invalid-pointer guard (<0x10000 OR >=0xFFFF0000 sentinel).\n"
        "\t{\n"
        "\t\tconst auto _bad = [](uint32_t v) { return v < 0x10000u || v >= 0xFFFF0000u; };\n"
        "\t\tif (_bad(ctx.r11.u32) || _bad(ctx.r9.u32)) {\n"
        "\t\t\tstatic std::atomic<uint32_t> fh1_gsf_inner_null_hits_{0};\n"
        "\t\t\tconst auto _n = fh1_gsf_inner_null_hits_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] GetSupportingFeature inner invalid (this={:#x} table_entry={:#x} verts={:#x} idx={}); skipping. count={}\", ctx.r31.u32, ctx.r11.u32, ctx.r9.u32, ctx.r30.u32, _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\tgoto loc_82D3DD4C;\n"
        "\t\t}\n"
        "\t}\n"
        "\t// lbzx r8,r11,r30\n"
        "\tctx.r8.u64 = REX_LOAD_U8(ctx.r11.u32 + ctx.r30.u32);",
},

// =========================================================================
// 0010 + 0014/0017 + 0026: GetSupportingFeature second-loop NULL guard
// Patch 0026 extended both inner-loop and 2nd-loop sentinel filters to also
// reject >=0xFFFF0000 (e.g., 0xFFFFFFFF observed at PC 0x82D3DCFB+ in a
// 2026-05-13 SIGBUS on XThread5DFFD6C0 / UpdateIslandsAsync worker).
// =========================================================================
{
    .id       = "0010+0014+0017",
    .file     = "fh1_recomp.78.cpp",
    .function = "sub_82D3DB00",
    .marker   = "fh1_gsf_2ndloop_null_hits_",
    .match =
        "\t// lwz r9,80(r31)\n"
        "\tctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);\n"
        "loc_82D3DC1C:\n"
        "\t// lbzx r10,r8,r11\n"
        "\tctx.r10.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r11.u32);",
    .with =
        "\t// lwz r9,80(r31)\n"
        "\tctx.r9.u64 = REX_LOAD_U32(ctx.r31.u32 + 80);\n"
        "\t// FH1 LAYER B 0010+0014+0017+0026: 2nd-loop invalid-pointer guard (<0x10000 OR >=0xFFFF0000 sentinel).\n"
        "\t{\n"
        "\t\tconst auto _bad = [](uint32_t v) { return v < 0x10000u || v >= 0xFFFF0000u; };\n"
        "\t\tif (_bad(ctx.r8.u32) || _bad(ctx.r7.u32) || _bad(ctx.r9.u32)) {\n"
        "\t\t\tstatic std::atomic<uint32_t> fh1_gsf_2ndloop_null_hits_{0};\n"
        "\t\t\tconst auto _n = fh1_gsf_2ndloop_null_hits_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] GetSupportingFeature 2nd-loop invalid (this={:#x} entry={:#x} entry0={:#x} r7={:#x} r9={:#x}); skipping. count={}\", ctx.r31.u32, ctx.r29.u32, ctx.r8.u32, ctx.r7.u32, ctx.r9.u32, _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\tgoto loc_82D3DD4C;\n"
        "\t\t}\n"
        "\t}\n"
        "loc_82D3DC1C:\n"
        "\t// lbzx r10,r8,r11\n"
        "\tctx.r10.u64 = REX_LOAD_U8(ctx.r8.u32 + ctx.r11.u32);",
},

// =========================================================================
// 0005 + 0011 + 0013 + 0016 + 0017: GetSupportingPolyFeature entry guard
// Final consolidated state: rejects this/r4, verts/r5, first_vert (*r5),
// and verts_at_80 (*shape+80) when any is below the 0x10000 invalid-pointer
// threshold or equals the 0xFFFFFFFF sentinel.
// =========================================================================
{
    .id       = "0005+0011+0013+0016+0017",
    .file     = "fh1_recomp.78.cpp",
    .function = "sub_82D3CD48",
    .marker   = "fh1_gspf_null_hits_",
    .match =
        "\tREX_FUNC_PROLOGUE();\n"
        "\tuint32_t ea{};\n"
        "\t// std r31,-8(r1)",
    .with =
        "\tREX_FUNC_PROLOGUE();\n"
        "\t// FH1 LAYER B 0005+0011+0013+0016+0017: invalid-pointer/sentinel guard.\n"
        "\t{\n"
        "\t\tconstexpr uint32_t kInvalidPtrThreshold = 0x10000u;\n"
        "\t\tconst uint32_t verts = ctx.r5.u32;\n"
        "\t\tconst uint32_t shape = ctx.r4.u32;\n"
        "\t\tuint32_t first_vert_ptr = 0;\n"
        "\t\tuint32_t verts_at_80 = 0;\n"
        "\t\tconst bool verts_ok = (verts >= kInvalidPtrThreshold) && (verts != 0xFFFFFFFFu);\n"
        "\t\tconst bool shape_ok = (shape >= kInvalidPtrThreshold) && (shape != 0xFFFFFFFFu);\n"
        "\t\tif (verts_ok) { first_vert_ptr = REX_LOAD_U32(verts + 0); }\n"
        "\t\tif (shape_ok) { verts_at_80    = REX_LOAD_U32(shape + 80); }\n"
        "\t\tconst bool invalid = !verts_ok || !shape_ok ||\n"
        "\t\t    first_vert_ptr < kInvalidPtrThreshold || first_vert_ptr == 0xFFFFFFFFu ||\n"
        "\t\t    verts_at_80    < kInvalidPtrThreshold || verts_at_80    == 0xFFFFFFFFu;\n"
        "\t\tif (invalid) {\n"
        "\t\t\tstatic std::atomic<uint32_t> fh1_gspf_null_hits_{0};\n"
        "\t\t\tconst auto _n = fh1_gspf_null_hits_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] GetSupportingPolyFeature invalid (verts={:#x} first_vert={:#x} shape={:#x} verts_at_80={:#x}); skipping. count={}\", verts, first_vert_ptr, shape, verts_at_80, _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\treturn;\n"
        "\t\t}\n"
        "\t}\n"
        "\tuint32_t ea{};\n"
        "\t// std r31,-8(r1)",
},

// =========================================================================
// 0004: CPhysicsWorld::UpdateIslandsAsync — NULL event-slot guard + fallback
//
// The async worker's work-item (r31) has the layout:
//   [0]  = this (CPhysicsWorld*)
//   [48] = &this->[1164] (event-handle slot pointer)
// The caller SimulateIslands builds this on its stack at offsets 96..147
// before calling AsyncFunction1_..._82D4BBC8 and then blocks on
// WaitForSingleObject(this->[1164], INFINITE). The worker MUST SetEvent
// that handle when its queue drain completes; otherwise the wait never
// returns and CForza::Run hangs at race-load (CPhysicsWorld::Update →
// UpdateHF → SimulateIslands).
//
// Original 0004 captured a NULL-`work_item->[48]` crash and skipped the
// SetEvent. That avoided the crash but caused a hard race-load hang (seen
// 2026-05-12 PM: render thread spinning in Frame, CForza::Run blocked
// inside SimulateIslands' WaitForSingleObject, log shows exactly one
// "UpdateIslandsAsync NULL event_slot" fire immediately before the hang).
//
// Refined version: when work_item->[48] is NULL, fall back to deriving the
// event from work_item->[0] (= this) + 1164. work_item->[0] must already
// be valid because the queue-drain loop above used it (this->[1108] is the
// queue base). If even that is NULL, log and skip — back to the original
// shield behavior so the process survives.
// =========================================================================
{
    .id       = "0004",
    .file     = "fh1_recomp.78.cpp",
    .function = "sub_82D4D9D0",
    .marker   = "fh1_updateislands_null_evt_",
    .match =
        "\t// lwz r11,48(r31)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 48);\n"
        "\t// lwz r3,0(r11)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);\n"
        "\t// bl 0x8247af78\n"
        "\tctx.lr = 0x82D4DA44;\n"
        "\tsub_8247AF78(ctx, base);",
    .with =
        "\t// lwz r11,48(r31)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r31.u32 + 48);\n"
        "\t// FH1 LAYER B 0004: NULL event-slot guard + fallback to work_item->[0]+1164.\n"
        "\tif (ctx.r11.u32 != 0) {\n"
        "\t\t// lwz r3,0(r11)\n"
        "\t\tctx.r3.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);\n"
        "\t\t// bl 0x8247af78\n"
        "\t\tctx.lr = 0x82D4DA44;\n"
        "\t\tsub_8247AF78(ctx, base);\n"
        "\t} else {\n"
        "\t\tconst uint32_t this_ptr = REX_LOAD_U32(ctx.r31.u32 + 0);\n"
        "\t\tstatic std::atomic<uint32_t> fh1_updateislands_null_evt_{0};\n"
        "\t\tconst auto _n = fh1_updateislands_null_evt_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (this_ptr != 0) {\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] UpdateIslandsAsync NULL event_slot; deriving from this={:#x} +1164. count={}\", this_ptr, _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\tctx.r3.u64 = REX_LOAD_U32(this_ptr + 1164);\n"
        "\t\t\tctx.lr = 0x82D4DA44;\n"
        "\t\t\tsub_8247AF78(ctx, base);\n"
        "\t\t} else if (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] UpdateIslandsAsync NULL event_slot AND NULL this; SimulateIslands will hang. count={}\", _n + 1);\n"
        "\t\t}\n"
        "\t}",
},

// =========================================================================
// 0008: AsyncQueueFunction wrapper vtable=0 post-dispatch guard
// =========================================================================
{
    .id       = "0008",
    .file     = "fh1_recomp.67.cpp",
    .function = "sub_82C0C8C8",
    .marker   = "fh1_aqfn_dtor_skip_",
    .match =
        "\t// lwz r11,0(r11)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);\n"
        "\t// mtctr r11\n"
        "\tctx.ctr.u64 = ctx.r11.u64;\n"
        "\t// bctrl \n"
        "\tctx.lr = 0x82C0C980;\n"
        "\tREX_CALL_INDIRECT_FUNC(ctx.ctr.u32);\n"
        "\t// b 0x82c0c910\n"
        "\tgoto loc_82C0C910;",
    .with =
        "\t// FH1 LAYER B 0008: wrapper post-dispatch vtable=0 guard.\n"
        "\tif (ctx.r11.u32 != 0) {\n"
        "\t\t// lwz r11,0(r11)\n"
        "\t\tctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 0);\n"
        "\t\t// mtctr r11\n"
        "\t\tctx.ctr.u64 = ctx.r11.u64;\n"
        "\t\t// bctrl\n"
        "\t\tctx.lr = 0x82C0C980;\n"
        "\t\tREX_CALL_INDIRECT_FUNC(ctx.ctr.u32);\n"
        "\t} else {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_aqfn_dtor_skip_{0};\n"
        "\t\tconst auto _n = fh1_aqfn_dtor_skip_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] AsyncQueueFunction wrapper={:#x} vtable=0; skipping deleting-dtor. count={}\", ctx.r31.u32, _n + 1);\n"
        "\t\t}\n"
        "\t}\n"
        "\t// b 0x82c0c910\n"
        "\tgoto loc_82C0C910;",
},

// =========================================================================
// 0009a: vector<CSoundFileId> copy ctor entry guard
// =========================================================================
{
    .id       = "0009-entry",
    .file     = "fh1_recomp.70.cpp",
    .function = "sub_82C5BC08",
    .marker   = "fh1_vec_csid_null_",
    .match =
        "\tREX_FUNC_PROLOGUE();\n"
        "\tuint32_t ea{};\n"
        "\t// mflr r12\n"
        "\tctx.r12.u64 = ctx.lr;",
    .with =
        "\tREX_FUNC_PROLOGUE();\n"
        "\t// FH1 LAYER B 0009a: vector<CSoundFileId> NULL this/src guard.\n"
        "\tif (ctx.r3.u32 == 0 || ctx.r4.u32 == 0) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_vec_csid_null_{0};\n"
        "\t\tconst auto _n = fh1_vec_csid_null_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] vector<CSoundFileId>::vector(this={:#x}, src={:#x}); skipping copy. count={}\", ctx.r3.u32, ctx.r4.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\treturn;\n"
        "\t}\n"
        "\tuint32_t ea{};\n"
        "\t// mflr r12\n"
        "\tctx.r12.u64 = ctx.lr;",
},

// =========================================================================
// 0009b: vector<CSoundFileId> post-_Buy dest.begin==0 short-circuit
// =========================================================================
{
    .id       = "0009-postbuy",
    .file     = "fh1_recomp.70.cpp",
    .function = "sub_82C5BC08",
    .marker   = "fh1_vec_csid_buy_null_",
    .match =
        "\t// lwz r10,0(r30)\n"
        "\tctx.r10.u64 = REX_LOAD_U32(ctx.r30.u32 + 0);\n"
        "\t// b 0x82c5bc78\n"
        "\tgoto loc_82C5BC78;",
    .with =
        "\t// lwz r10,0(r30)\n"
        "\tctx.r10.u64 = REX_LOAD_U32(ctx.r30.u32 + 0);\n"
        "\t// FH1 LAYER B 0009b: dest.begin==0 after _Buy → short-circuit (avoids r10+=4 trap).\n"
        "\tif (ctx.r10.u32 == 0) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_vec_csid_buy_null_{0};\n"
        "\t\tconst auto _n = fh1_vec_csid_buy_null_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] vector<CSoundFileId>: dest.begin==0 after _Buy (this={:#x}); skipping copy. count={}\", ctx.r30.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\tgoto loc_82C5BC84;\n"
        "\t}\n"
        "\t// b 0x82c5bc78\n"
        "\tgoto loc_82C5BC78;",
},

// =========================================================================
// 0015 + 0017: CGame::Update r28 corruption guard
// Threshold = 0x10000 per patch 0017.
// =========================================================================
{
    .id       = "0015+0017",
    .file     = "fh1_recomp.0.cpp",
    .function = "sub_823E3218",
    .marker   = "fh1_cgame_update_r28_null_",
    .match =
        "\tsub_82DE7330(ctx, base);\n"
        "\t// lwz r11,4(r28)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r28.u32 + 4);",
    .with =
        "\tsub_82DE7330(ctx, base);\n"
        "\t// FH1 LAYER B 0015+0017: r28 corruption guard before audio-cue loop.\n"
        "\tif (ctx.r28.u32 < 0x10000u) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_cgame_update_r28_null_{0};\n"
        "\t\tconst auto _n = fh1_cgame_update_r28_null_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] CGame::Update r28={:#x} after inner calls; skipping audio-cue loop. count={}\", ctx.r28.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\tgoto loc_823E3418;\n"
        "\t}\n"
        "\t// lwz r11,4(r28)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r28.u32 + 4);",
},

// =========================================================================
// 0018: TransferManagerThread r30 refresh at loop top
// =========================================================================
{
    .id       = "0018",
    .file     = "fh1_recomp.68.cpp",
    .function = "sub_82C204B8",
    .marker   = "fh1_xfermgr_r30_corrupt_",
    .match =
        "loc_82C204FC:\n"
        "\t// mr r3,r31\n"
        "\tctx.r3.u64 = ctx.r31.u64;\n"
        "\t// bl 0x82c1fce8\n"
        "\tctx.lr = 0x82C20504;\n"
        "\tsub_82C1FCE8(ctx, base);",
    .with =
        "loc_82C204FC:\n"
        "\t// FH1 LAYER B 0018: r30 refresh at loop top (callee-saved-register corruption).\n"
        "\tif (ctx.r30.u32 != 0x834B0000u) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_xfermgr_r30_corrupt_{0};\n"
        "\t\tconst auto _n = fh1_xfermgr_r30_corrupt_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] TransferManagerThread r30={:#x} (expected 0x834B0000); refreshing. count={}\", ctx.r30.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\tctx.r30.s64 = -2092236800;\n"
        "\t}\n"
        "\t// mr r3,r31\n"
        "\tctx.r3.u64 = ctx.r31.u64;\n"
        "\t// bl 0x82c1fce8\n"
        "\tctx.lr = 0x82C20504;\n"
        "\tsub_82C1FCE8(ctx, base);",
},

// =========================================================================
// 0019: sub_82467B18 (DPTP worker inner) r30 corruption guard
// =========================================================================
{
    .id       = "0019",
    .file     = "fh1_recomp.5.cpp",
    .function = "sub_82467B18",
    .marker   = "fh1_dptp_r30_corrupt_",
    .match =
        "loc_82467CD0:\n"
        "\t// lwz r11,40(r30)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 40);",
    .with =
        "loc_82467CD0:\n"
        "\t// FH1 LAYER B 0019: r30 corruption guard before queue-node access.\n"
        "\tif (ctx.r30.u32 < 0x10000u || ctx.r30.u32 >= 0xFFFF0000u) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_dptp_r30_corrupt_{0};\n"
        "\t\tconst auto _n = fh1_dptp_r30_corrupt_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] sub_82467B18 r30={:#x}; skipping queue-node access. count={}\", ctx.r30.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\tgoto loc_82467DD8;\n"
        "\t}\n"
        "\t// lwz r11,40(r30)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r30.u32 + 40);",
},

// =========================================================================
// 0023: CPhysicsWorld::SimulateIslands — bound WaitForSingleObject timeout
//
// SimulateIslands queues a work-item to the async worker pool, then drains
// remaining islands synchronously, then WaitForSingleObject(event, INFINITE)
// for the worker's SetEvent. When the underlying fn-obj wrapper gets
// destroyed before the worker dequeues it (separate FH1 lifetime bug —
// the wrapper at +0xBE7016DD shows vtable=0, and the work-item it points
// to reads back as all-zeros so patch 0004's normal AND derived
// event-slot paths both fail), the SetEvent never fires and CForza::Run
// hangs forever at race load.
//
// Patch: replace the INFINITE timeout (`li r4,-1`) with 500ms. Normal-
// case latency for this wait is sub-ms — the worker SetEvent's almost
// immediately — so this changes nothing for healthy frames. When the
// worker drops the event, SimulateIslands returns 500ms late instead of
// hanging; any unprocessed islands sit in the queue and get picked up by
// the next frame's SimulateIslands call.
// =========================================================================
{
    .id       = "0023",
    .file     = "fh1_recomp.79.cpp",
    .function = "sub_82D4DCC8",
    .marker   = "fh1_simislands_wait_timeout_",
    .match =
        "\t// li r4,-1\n"
        "\tctx.r4.s64 = -1;\n"
        "\t// lwz r3,0(r29)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);\n"
        "\t// bl 0x82465eb8\n"
        "\tctx.lr = 0x82D4DDEC;\n"
        "\tsub_82465EB8(ctx, base);",
    .with =
        "\t// FH1 LAYER B 0023: cap SimulateIslands worker-wait at 500ms instead of INFINITE.\n"
        "\t// Healthy frames: worker SetEvent's in <1ms; timeout is irrelevant. Broken\n"
        "\t// frames (fn-obj wrapper destroyed before dispatch, patch 0004 sees\n"
        "\t// double-NULL work-item): wait returns WAIT_TIMEOUT, game proceeds with\n"
        "\t// partially-drained island queue (carried into next frame).\n"
        "\tctx.r4.s64 = 500;\n"
        "\t{\n"
        "\t\tstatic std::atomic<uint32_t> fh1_simislands_wait_timeout_{0};\n"
        "\t\tconst auto _n = fh1_simislands_wait_timeout_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t(void)_n;\n"
        "\t}\n"
        "\t// lwz r3,0(r29)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r29.u32 + 0);\n"
        "\t// bl 0x82465eb8\n"
        "\tctx.lr = 0x82D4DDEC;\n"
        "\tsub_82465EB8(ctx, base);",
},

// =========================================================================
// 0022: CWindowedStream ctor — NULL stream-ref guard (defense-in-depth)
//
// Capture: 2026-05-12 PM, SIGABRT on XThread1E3C86C0 via
// CForza::CreateGameFiberStartup → CGameManager::Init → CGameManager::Reset
// → CRecordReplayComponent::PreRace → CReplayManager::CreateStream →
// CReplayManager::CreateReplayStream → CWindowedStream::CWindowedStream.
// Original cause: rexglue's SetupVfs leaves `cache:` unmounted, so
// NtCreateFile('cache:\replay_stream') returns 0xC000000F and the resulting
// NULL stream ref propagates into the constructor. Parent CRedirectStream
// already null-checks [this+32] before its own equivalent vcall (82C634AC),
// but CWindowedStream's first vcall (82C7E5AC, vtable[88]) does NOT — bctrl
// on *NULL faults.
//
// Primary fix is the cache: mount in Fh1App::OnPostSetup (fh1_app.cpp), which
// gives the open a writable host-backed directory. This in-body guard stays
// as a defensive net for the case where the cache directory can't be
// created (permissions, full disk, ...). On the bypass branch synthesise
// r3 = 1 so the downstream clrlwi/addic/subfe math stores [this+8] = 1,
// which then makes the SECOND vcall site (82C7E62C, vtable[40]) take its
// existing skip-branch (`bne cr6, loc_82C7E650`). One patch covers both
// sites.
// =========================================================================
{
    .id       = "0022",
    .file     = "fh1_recomp.71.cpp",
    .function = "sub_82C7E4F0",
    .marker   = "fh1_cwindowed_null_stream_",
    .match =
        "\t// lwz r3,32(r31)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);\n"
        "\t// lwz r11,0(r3)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);\n"
        "\t// lwz r11,88(r11)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 88);\n"
        "\t// mtctr r11\n"
        "\tctx.ctr.u64 = ctx.r11.u64;\n"
        "\t// bctrl \n"
        "\tctx.lr = 0x82C7E5AC;\n"
        "\tREX_CALL_INDIRECT_FUNC(ctx.ctr.u32);",
    .with =
        "\t// lwz r3,32(r31)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r31.u32 + 32);\n"
        "\t// FH1 LAYER B 0022: NULL stream-ref guard ([this+32]==0 when cache: open failed).\n"
        "\tif (ctx.r3.u32 != 0) {\n"
        "\t\t// lwz r11,0(r3)\n"
        "\t\tctx.r11.u64 = REX_LOAD_U32(ctx.r3.u32 + 0);\n"
        "\t\t// lwz r11,88(r11)\n"
        "\t\tctx.r11.u64 = REX_LOAD_U32(ctx.r11.u32 + 88);\n"
        "\t\t// mtctr r11\n"
        "\t\tctx.ctr.u64 = ctx.r11.u64;\n"
        "\t\t// bctrl\n"
        "\t\tctx.lr = 0x82C7E5AC;\n"
        "\t\tREX_CALL_INDIRECT_FUNC(ctx.ctr.u32);\n"
        "\t} else {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_cwindowed_null_stream_{0};\n"
        "\t\tconst auto _n = fh1_cwindowed_null_stream_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] CWindowedStream ctor: [this+32]=0 (cache: device unmounted; replay-stream open failed); forcing flag=1 to skip vtable[88]+[40]. count={}\", _n + 1);\n"
        "\t\t}\n"
        "\t\tctx.r3.s64 = 1;\n"
        "\t}",
},

// =========================================================================
// 0024: DNAppendListNBQueue — r28 sentinel guard in list-walk
//
// Capture: 2026-05-13 01:45 UTC, SIGBUS on XThreadDBFFF6C0 (DPTP worker) at
// DNAppendListNBQueue_8312EC58+198 (PPC PC 0x8312ECB0).
//
// Disasm: `mov (%rbx,%rcx,1),%ecx` with rcx=0xFFFFFFFF, faulting at
// guest_base+4GiB-4. rcx held ctx.r28.u32 (the loop's input-list cursor).
// Source line: `loc_8312ECB0:  ctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);`.
//
// Call site: DPTPWorkerThreadProc → sub_82467B18 (already guards r30 via
// patch 0019, but unrelated register) → DNAppendListNBQueue. The input list
// (r4 at entry) comes from ProcessIo_82467F58 writing into the caller's
// stack slot at sp+84. Under contention, ProcessIo evidently produces a
// chain whose terminal node's `next` pointer (offset 0) reads back as
// 0xFFFFFFFF — uninitialized-memory sentinel — instead of the expected 0.
// The loop walks `r28 = *r28` until r28==0; a sentinel terminator faults
// instead of stopping.
//
// Patch: at loc_8312ECB0, before the load, sentinel-check r28 and force
// loop-exit (r28=0) when it's outside the heap range. The subsequent
// `cmplwi cr6,r28,0; bne cr6,...` already implements the loop-exit on
// r28==0; we just take that path explicitly when r28 looks corrupt.
//
// Effect on healthy traversals: zero — r28 is a real heap pointer
// (typically 0x40000000-0x80000000) so the guard's predicate is false.
// Effect on corrupted traversals: the partial chain we built so far
// (r29..r31 via *(r27+0)=r31 chaining) is still committed to the queue
// inside the critsec below; the unprocessed tail of the input list is
// dropped on the floor.
// =========================================================================
{
    .id       = "0024",
    .file     = "fh1_recomp.111.cpp",
    .function = "sub_8312EC58",
    .marker   = "fh1_dnappend_r28_sentinel_",
    .match =
        "loc_8312ECB0:\n"
        "\t// lwz r28,0(r28)\n"
        "\tctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);",
    .with =
        "loc_8312ECB0:\n"
        "\t// FH1 LAYER B 0024: r28 sentinel guard before list-next dereference.\n"
        "\tif (ctx.r28.u32 < 0x10000u || ctx.r28.u32 >= 0xFFFF0000u) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_dnappend_r28_sentinel_{0};\n"
        "\t\tconst auto _n = fh1_dnappend_r28_sentinel_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] DNAppendListNBQueue r28={:#x}; truncating input list. count={}\", ctx.r28.u32, _n + 1);\n"
        "\t\t}\n"
        "\t\tctx.r28.s64 = 0;\n"
        "\t} else {\n"
        "\t\t// lwz r28,0(r28)\n"
        "\t\tctx.r28.u64 = REX_LOAD_U32(ctx.r28.u32 + 0);\n"
        "\t}",
},

// =========================================================================
// 0025: CPhysicsWorld::UpdateHF — bound CSemaphore::WaitFor at loc_82D4E05C
//
// Capture: 2026-05-13 01:53 UTC, live-hang snapshot on XThreadABFFF6C0
// (gameplay) stuck in pthread_cond_wait inside
// __imp__sub_823E91C0, called from
// __imp__CPhysicsWorld__UpdateHF_82D4DE30 (frame 10) — NOT from
// SimulateIslands. Stack: UpdateHF → CSemaphore::WaitFor → WaitForSingleObjectEx
// → NtWaitForSingleObjectEx → pthread_cond_wait. Patch 0023 (SimulateIslands
// 500ms) is in effect — this is a sibling wait one level up.
//
// Source: UpdateHF queues car-dynamics work via
// AsyncFunction1_bool_CUpdateCarDynamicsAsyncParams_82D4BAF8 in a setup loop,
// then drains FetchNextCarDynamicsToProcess synchronously, then at
// loc_82D4E05C calls Threads::CSemaphore::WaitFor(r3=r30=sem, r4=r28=-1) to
// block on the remaining async workers' completion event. When a worker's
// fn-obj wrapper has been destroyed before dispatch (same FH1 lifetime bug
// patch 0023 mitigates for SimulateIslands), the SetEvent never fires and
// UpdateHF hangs at this WaitFor with r4=-1=INFINITE.
//
// Patch: when r28==-1 (INFINITE), replace timeout with 500ms AND force the
// post-call r3 to 1 (success) so the caller follows the `bne 0x82d4e0b4`
// success-branch out of the retry loop. Naive timeout alone is insufficient:
// the wait sits inside an `if (r3.u8 == 0) goto loc_82D4E020` retry loop
// (unlike SimulateIslands which unconditionally `b 0x82d4de14` after its
// WaitForSingleObject) — so a 500ms timeout would just re-arm the wait.
// Spoofing r3=1 routes execution to loc_82D4E0B4, which then continues the
// physics frame (SweepAndPrune::UpdatePairs/GetPairs, UpdateJoints,
// ApplyCarCollisionBiasHacks, …). The unfinished async car-dynamics work
// gets picked up on the next frame.
//
// Non-INFINITE callers pass through unchanged (the other entry into
// loc_82D4E05C from the fall-through at +24422 uses r28 already set to a
// real timeout / bounded retry).
// =========================================================================
{
    .id       = "0025",
    .file     = "fh1_recomp.79.cpp",
    .function = "sub_82D4DE30",
    .marker   = "fh1_updatehf_wait_timeout_",
    .match =
        "\t// mr r4,r28\n"
        "\tctx.r4.u64 = ctx.r28.u64;\n"
        "\t// mr r3,r30\n"
        "\tctx.r3.u64 = ctx.r30.u64;\n"
        "\t// bl 0x823e91c0\n"
        "\tctx.lr = 0x82D4E068;\n"
        "\tsub_823E91C0(ctx, base);",
    .with =
        "\t// FH1 LAYER B 0025: cap UpdateHF car-dynamics wait at 50ms when INFINITE,\n"
        "\t// then spoof r3=1 on timeout so the caller exits the retry loop.\n"
        "\tif (ctx.r28.s32 == -1) {\n"
        "\t\tstatic std::atomic<uint32_t> fh1_updatehf_wait_timeout_{0};\n"
        "\t\tctx.r4.s64 = 50;\n"
        "\t\tctx.r3.u64 = ctx.r30.u64;\n"
        "\t\tctx.lr = 0x82D4E068;\n"
        "\t\tsub_823E91C0(ctx, base);\n"
        "\t\tif ((ctx.r3.u32 & 0xFFu) == 0u) {\n"
        "\t\t\tconst auto _n = fh1_updatehf_wait_timeout_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\t\tif (_n < 16 || (_n & 0xFFFF) == 0) {\n"
        "\t\t\t\tREXLOG_WARN(\"[fh1-patch] UpdateHF car-dynamics wait timed out at 50ms; spoofing success to exit retry loop. count={}\", _n + 1);\n"
        "\t\t\t}\n"
        "\t\t\tctx.r3.s64 = 1;\n"
        "\t\t}\n"
        "\t} else {\n"
        "\t\t// mr r4,r28\n"
        "\t\tctx.r4.u64 = ctx.r28.u64;\n"
        "\t\t// mr r3,r30\n"
        "\t\tctx.r3.u64 = ctx.r30.u64;\n"
        "\t\t// bl 0x823e91c0\n"
        "\t\tctx.lr = 0x82D4E068;\n"
        "\t\tsub_823E91C0(ctx, base);\n"
        "\t}",
},

// =========================================================================
// XMem fallback: stw r27,6(r0) uses absolute EA 6 (null-page probe), not
// host base+6. XMedia physical alloc failure path (sub_830ED008 returned 0).
// =========================================================================
{
    .id       = "xmem-abs6",
    .file     = "fh1_recomp.73.cpp",
    .function = "sub_82CC1B10",
    .marker   = "fh1_xmem_stw_abs6_guard_",
    .match =
        "\t// stw r27,6(0)\n"
        "\tREX_STORE_U32(6, ctx.r27.u32);",
    .with =
        "\t// stw r27,6(0) — absolute guest EA 6; skip (REX_STORE would use base+6).\n"
        "\t{\n"
        "\t\tstatic std::atomic<uint32_t> fh1_xmem_stw_abs6_guard_{0};\n"
        "\t\tconst auto _n = fh1_xmem_stw_abs6_guard_.fetch_add(1, std::memory_order_relaxed);\n"
        "\t\tif (_n < 12) {\n"
        "\t\t\tREXLOG_WARN(\"[fh1-patch] sub_82CC1B10: XPhysicalAlloc fallback null; skip stw to guest addr 6 (size=0x{:08X}). count={}\", ctx.r27.u32, _n + 1);\n"
        "\t\t}\n"
        "\t}",
},

// =========================================================================
// GameManager_Init: lwz r3,dword_834B36A8@l(r28) after sub_82C84E78
// r28 (lis dword_834B36A8@ha) clobbered by callees on work-queue fiber path.
// =========================================================================
{
    .id       = "gamemgr-r28-profile-end",
    .file     = "fh1_recomp.11.cpp",
    .function = "sub_8251E560",
    .marker   = "fh1_gamemgr_r28_profile_end_",
    .match =
        "\t// bl 0x82c84e78\n"
        "\tctx.lr = 0x8251E730;\n"
        "\tsub_82C84E78(ctx, base);\n"
        "\t// mr r4,r29\n"
        "\tctx.r4.u64 = ctx.r29.u64;\n"
        "\t// lwz r3,13992(r28)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r28.u32 + 13992);",
    .with =
        "\t// bl 0x82c84e78\n"
        "\tctx.lr = 0x8251E730;\n"
        "\tsub_82C84E78(ctx, base);\n"
        "\t// mr r4,r29\n"
        "\tctx.r4.u64 = ctx.r29.u64;\n"
        "\t// lwz r3,dword_834B36A8@l(r28) — absolute global; r28 NV clobbered.\n"
        "\tctx.r3.u64 = REX_LOAD_U32(0x834B36A8u);",
},

{
    .id       = "gamemgr-r28-profile-fpu1",
    .file     = "fh1_recomp.11.cpp",
    .function = "sub_8251E560",
    .marker   = "fh1_gamemgr_r28_profile_fpu1_",
    .match =
        "\tctx.lr = 0x8251E830;\n"
        "\tsub_82C0C668(ctx, base);\n"
        "\t// lis r11,-32256\n"
        "\tctx.r11.s64 = -2113929216;\n"
        "\t// lwz r3,13992(r28)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r28.u32 + 13992);",
    .with =
        "\tctx.lr = 0x8251E830;\n"
        "\tsub_82C0C668(ctx, base);\n"
        "\t// lis r11,-32256\n"
        "\tctx.r11.s64 = -2113929216;\n"
        "\t// lwz r3,dword_834B36A8@l(r28) — absolute global; r28 NV clobbered.\n"
        "\tctx.r3.u64 = REX_LOAD_U32(0x834B36A8u);",
},

{
    .id       = "gamemgr-r28-profile-fpu2",
    .file     = "fh1_recomp.11.cpp",
    .function = "sub_8251E560",
    .marker   = "fh1_gamemgr_r28_profile_fpu2_",
    .match =
        "\tctx.lr = 0x8251E86C;\n"
        "\tsub_82C0C668(ctx, base);\n"
        "\t// lwz r3,13992(r28)\n"
        "\tctx.r3.u64 = REX_LOAD_U32(ctx.r28.u32 + 13992);",
    .with =
        "\tctx.lr = 0x8251E86C;\n"
        "\tsub_82C0C668(ctx, base);\n"
        "\t// lwz r3,dword_834B36A8@l(r28) — absolute global; r28 NV clobbered.\n"
        "\tctx.r3.u64 = REX_LOAD_U32(0x834B36A8u);",
},

// =========================================================================
// sub_82482718: lwz r4,24872(r11) then lwz r11,0(r4) — global @83446128 null.
// Streaming obj init path; skip vtable dispatch when manager pointer unset.
// =========================================================================
{
    .id       = "streaming-null-mgr-83446128",
    .file     = "fh1_recomp.6.cpp",
    .function = "sub_82482718",
    .marker   = "fh1_streaming_null_mgr_83446128_",
    .match =
        "\t// addi r5,r1,112\n"
        "\tctx.r5.s64 = ctx.r1.s64 + 112;\n"
        "\t// lwz r4,24872(r11)\n"
        "\tctx.r4.u64 = REX_LOAD_U32(ctx.r11.u32 + 24872);\n"
        "\t// addi r3,r1,80\n"
        "\tctx.r3.s64 = ctx.r1.s64 + 80;\n"
        "\t// lwz r11,0(r4)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);",
    .with =
        "\t// addi r5,r1,112\n"
        "\tctx.r5.s64 = ctx.r1.s64 + 112;\n"
        "\t// fh1_streaming_null_mgr_83446128_\n"
        "\t// lwz r4,dword_83446128@l(r11)\n"
        "\tctx.r4.u64 = REX_LOAD_U32(0x83446128u);\n"
        "\t// addi r3,r1,80\n"
        "\tctx.r3.s64 = ctx.r1.s64 + 80;\n"
        "\t// cmplwi cr6,r4,0\n"
        "\tctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);\n"
        "\t// beq cr6,0x82482860\n"
        "\tif (ctx.cr6.eq) goto loc_82482860;\n"
        "\t// lwz r11,0(r4)\n"
        "\tctx.r11.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);",
},

// =========================================================================
// sub_825109E8: null r3 (→ r20) from FH1_streaming_obj_init lwz r3,0(r3).
// r31 byte 0 path (loc_82510B40) does lwz r11,0(r20) → guest addr 0 AV.
// =========================================================================
{
    .id       = "streaming-null-r20-825109e8",
    .file     = "fh1_recomp.11.cpp",
    .function = "sub_825109E8",
    .marker   = "fh1_sub_825109e8_null_r20_",
    .match =
        "\t// stwu r1,-624(r1)\n"
        "\tea = -624 + ctx.r1.u32;\n"
        "\tREX_STORE_U32(ea, ctx.r1.u32);\n"
        "\tctx.r1.u32 = ea;\n"
        "\t// mr r20,r3\n"
        "\tctx.r20.u64 = ctx.r3.u64;\n"
        "\t// mr r31,r4\n"
        "\tctx.r31.u64 = ctx.r4.u64;\n"
        "\t// addi r4,r1,80\n"
        "\tctx.r4.s64 = ctx.r1.s64 + 80;",
    .with =
        "\t// stwu r1,-624(r1)\n"
        "\tea = -624 + ctx.r1.u32;\n"
        "\tREX_STORE_U32(ea, ctx.r1.u32);\n"
        "\tctx.r1.u32 = ea;\n"
        "\t// fh1_sub_825109e8_null_r20_\n"
        "\t// mr r20,r3\n"
        "\tctx.r20.u64 = ctx.r3.u64;\n"
        "\t// mr r31,r4\n"
        "\tctx.r31.u64 = ctx.r4.u64;\n"
        "\t// cmplwi cr6,r20,0\n"
        "\tctx.cr6.compare<uint32_t>(ctx.r20.u32, 0, ctx.xer);\n"
        "\t// beq cr6,0x82510ddc\n"
        "\tif (ctx.cr6.eq) goto loc_82510DDC;\n"
        "\t// addi r4,r1,80\n"
        "\tctx.r4.s64 = ctx.r1.s64 + 80;",
},

};  // g_patches

}  // namespace

const std::span<const Patch> kPatches{g_patches};

}  // namespace fh1::patcher
