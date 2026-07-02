#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <rex/cvar.h>
#include <rex/hash.h>
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xevent.h>
#include <rex/system/util/object_table.h>

#include "native_renderer/fm2_direct_draw_decode.h"
#include "native_renderer/fm2_native_renderer.h"
#include "render/guest_device.h"
#include "render/guest_heap.h"
#include "render/guest_resources.h"
#include "render/render_internal.h"
#include "render/render_state.h"
#include "render/fm2_device.h"
#include "render/video.h"

#include <windows.h>

namespace {
// Crash logger: runs LAST in the VEH chain (AddVectoredExceptionHandler First=0),
// so the runtime's commit-on-demand page-fault handler intercepts benign AVs first
// and only genuinely-UNHANDLED faults reach here. Logs the faulting rip + a stack
// backtrace so we can map the host addresses to the crashing function/shader.
LONG WINAPI Fm2CrashLogger(EXCEPTION_POINTERS *ep) {
  const auto *r = ep->ExceptionRecord;
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) >= 4)
    return EXCEPTION_CONTINUE_SEARCH;
  void *frames[24];
  const USHORT n = RtlCaptureStackBackTrace(0, 24, frames, nullptr);
  if (FILE *f = std::fopen("C:\\temp\\fm2-clean.log", "a")) {
    std::fprintf(f, "FM2_CRASH code=0x%08lX rip=%p fault=0x%llX frames:",
                 r->ExceptionCode, r->ExceptionAddress,
                 r->NumberParameters >= 2
                     ? (unsigned long long)r->ExceptionInformation[1]
                     : 0ull);
    for (USHORT i = 0; i < n; ++i)
      std::fprintf(f, " %p", frames[i]);
    std::fprintf(f, "\n");
    std::fclose(f);
  }
  return EXCEPTION_CONTINUE_SEARCH; // let it crash normally (now logged)
}
struct Fm2CrashInit {
  Fm2CrashInit() { AddVectoredExceptionHandler(0, &Fm2CrashLogger); }
} g_fm2CrashInit;
} // namespace

namespace rr = fm2::render;
namespace ghp = fm2::ghp;
namespace nr = fm2::native_renderer;

// Win32 thread-id without pulling in <windows.h> (avoids min/max macro clashes).
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);

namespace fm2::render {
GuestBuffer *CreateVertexBuffer(uint32_t length);
GuestBuffer *CreateIndexBuffer(uint32_t length, uint32_t format);
void RegisterBufferAlias(uint32_t guestAddr, GuestBuffer *buf);
GuestBuffer *LookupBufferAlias(uint32_t guestAddr);
GuestTexture *CreateTexture(uint32_t width, uint32_t height, uint32_t depth,
                            uint32_t levels, uint32_t usage, uint32_t format,
                            uint32_t pool, uint32_t type);
GuestSurface *CreateSurface(uint32_t width, uint32_t height, uint32_t format,
                            uint32_t multiSample);
GuestVertexDeclaration *
CreateVertexDeclaration(GuestVertexElement *guestElements);
void RegisterVertexDeclarationAlias(uint32_t guestAddress,
                                    GuestVertexDeclaration *declaration);
GuestShader *CreateVertexShader(const uint32_t *function);
GuestShader *CreatePixelShader(const uint32_t *function);
void RegisterShaderAlias(uint32_t guestAddress, GuestShader *shader);
GuestShader *LookupShaderAlias(uint32_t guestAddress);
GuestTexture *LoadTextureFromMemory(const uint8_t *data, uint32_t size);
GuestTexture *TranslateGuestTextureFetch(const void *guestFetch,
                                         bool uploadGuestData);
GuestTexture *LookupGuestTextureByBase(uint32_t baseAddress);
GuestTexture *TranslateGuestTexture(void *guestHeader, bool uploadGuestData);
GuestBaseTexture *TranslateGuestSurface(void *guestHeader);
uint32_t LockVertexBuffer(GuestBuffer *buffer, uint32_t flags);
void UnlockVertexBuffer(GuestBuffer *buffer);
uint32_t LockIndexBuffer(GuestBuffer *buffer, uint32_t flags);
void UnlockIndexBuffer(GuestBuffer *buffer);
void LockTextureRect(GuestTexture *texture, uint32_t *outPitch,
                     uint32_t *outBits);
void UnlockTextureRect(GuestTexture *texture);
} // namespace fm2::render

REX_IMPORT(__imp__FM2_RenderContext_SetPixelShaderState,
           g_origFm2SetPixelShaderState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetVertexShaderState,
           g_origFm2SetVertexShaderState, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_BindVertexStream,
           g_origFm2BindVertexStream,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint64_t));
REX_IMPORT(__imp__FM2_RenderContext_BindIndexBuffer,
           g_origFm2BindIndexBuffer, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetBoundSurface,
           g_origFm2SetBoundSurface, void(uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_TryPresentAndUpdateStatus,
           g_origFm2TryPresentAndUpdateStatus, void(uint32_t));
// D3DDevice_Swap (was FM2_GpuCommandBuffer_BuildAndSubmit; renamed 2026-07-01,
// see docs/FM2-ida-renames-2026-07-01.md -- confirmed == LO rex_D3DDevice_Swap).
REX_IMPORT(__imp__D3DDevice_Swap,
           g_origFm2GpuCommandBufferBuildAndSubmit,
           void(uint32_t, uint32_t, uint32_t));
// Guest render-worker per-frame dispatch (sub_82288948), called in an infinite
// loop by sub_82289640 on a guest XThread. In plume_native the present triggers
// (FMOD pump bit / sub_825ADE20) never fire, so we drive Video::Present() from
// here — same thread that records draws into g_commandList (no cross-thread race).
REX_IMPORT(__imp__sub_82288948, g_origRenderWorkerFrame, void(uint32_t));
// Gate-probe helpers: the two functions RunFrame uses to compute v33 (the drain
// a2 selector). sub_8245CD38(queue) returns *(queue+60)?1:*(queue+128); >1 means
// backlog pressure. sub_8245D3E0(queue) returns the head deferred node's flag byte
// (under the queue+64 lock). v33 = (backlog>1 && cd38>1) || d3e0.
REX_IMPORT(__imp__sub_8245CD38, g_cbQueueCd38, uint32_t(uint32_t));
REX_IMPORT(__imp__sub_8245D3E0, g_cbQueueD3e0, uint32_t(uint32_t));
// FM2_RenderContext_UploadMatrixConstants (sub_8236D958): the game uploads per-draw
// VS transform constants here, but to a render-context object FlushRenderState's
// GuestDevice never sees (so device VS constants are zero for scene draws -> geometry
// collapses in plume_native). Mirror the upload into the unified VS const buffer.
REX_IMPORT(__imp__sub_8236D958, g_origUploadMatrixConstants,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
// FM2_RenderContext_SetActivePassId (sub_8236E228): stores the active vertex
// declaration handle at renderContext+11540, read by FM2_D3D_EmitDrawListStatePackets
// at draw time. Forza binds the per-draw vertex declaration here (a D3DVERTEXELEMENT9
// declaration created via D3DDevice_CreateVertexDeclaration), NOT via the standard
// D3DDevice_SetVertexDeclaration that writes the device field -- so device+0x2E24 is
// always 0 in plume_native and every draw fails with missing_decl. Hook it to feed
// device->vertexDeclaration so the existing Sync/alias path resolves the layout.
REX_IMPORT(__imp__sub_8236E228, g_origSetActivePassId, void(uint32_t, uint32_t));
// FM2_RenderContext_BindSurfaceInternal (sub_823716F8): binds a COLOR render-target
// surface to slot a2 (0-3) at renderContext+12160+4*slot -- this is Forza's color RT
// bind (proven: FM2_D3D_CreatePresentBackbufferResources calls it with slot 0 for the
// backbuffer color surface, while depth goes through SetBoundSurface at +12176). The
// native renderer never hooked it, so g_renderTarget stayed null and forward draws had
// no color attachment -> rendered depth-only -> blue screen. Hook it to bind the color
// surface natively via SetRenderTargetNative, mirroring the depth path.
REX_IMPORT(__imp__sub_823716F8, g_origBindSurfaceInternal,
           void(uint32_t, uint32_t, uint32_t));
// Frame-sync gate: the guest game-loop thread (sub_822172E8) blocks each frame on
// FM2_Win32_WaitForSingleObject(dword_829C24C0,-1) when in vsync frame-sync mode.
// FM2_SignalGate pulses that event to advance the loop, but its normal trigger is
// dead in plume_native (no Xenia GPU/vblank), so the game never advances/renders.
// We pulse it ourselves (rate-limited) from the per-frame present hook.
REX_IMPORT(__imp__FM2_SignalGate_8220A4E8, g_origSignalGate, void(uint32_t));
REX_IMPORT(__imp__FM2_FmodIrqSubmit_8236C688,
           g_origFm2FmodIrqSubmit8236C688,
           void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_ProducerProgressGuard_82369340,
           g_origFm2ProducerProgressGuard, uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_Render_LoadPixelShaderResourceById,
           g_origFm2LoadPixelShaderResourceById, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_Render_LoadVertexShaderResourceById,
           g_origFm2LoadVertexShaderResourceById, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_Render_AllocGpuPassMemoryBlock,
           g_origFm2AllocGpuPassMemoryBlock, uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_D3D_CreateGpuMemoryBlock,
           g_origFm2CreateGpuMemoryBlock, uint32_t(uint32_t));

// Rewired 2026-07-01 (manifest/IDA drift fix #3, docs/FM2-ida-renames-2026-07-01.md):
// the June-18 "packed state" cluster at 0x8236Exxx-0x8236Fxxx is the ordinary
// XDK D3DDevice_SetRenderState_* family writing RB_DEPTHCONTROL fields (ctx+
// 10420) etc., and nearly every old FM2_RenderContext_* hook mirrored the
// WRONG semantic. The killers: the real ZFunc (0x8236F1F0) was mirrored into
// D3DRS_ALPHABLENDENABLE, while D3DRS_ZFUNC was driven by 0x8236F2D0
// (StencilFail, always 0=KEEP) -> depth compare NEVER -> every fragment
// rejected -> black screen. StencilPass (0x8236F340) drove
// D3DRS_COLORWRITEENABLE (the historic colorWrite=0 depth-only bug), and
// AlphaBlendEnable (0x8236EAF8) drove D3DRS_ZENABLE. The stencil-op setters
// (TwoSidedStencilMode/StencilFail/StencilPass) are no longer hooked at all:
// their originals run untouched, which is the correct passthrough.
REX_IMPORT(__imp__D3DDevice_SetRenderState_AlphaBlendEnable,
           g_origRsAlphaBlendEnable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_AlphaTestEnable,
           g_origRsAlphaTestEnable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_ZEnable,
           g_origRsZEnable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_ZWriteEnable,
           g_origRsZWriteEnable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_ZFunc,
           g_origRsZFunc, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_ColorWriteEnable,
           g_origRsColorWriteEnable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_BlendOp,
           g_origRsBlendOp, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_SrcBlend,
           g_origRsSrcBlend, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_DestBlend,
           g_origRsDestBlend, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_SrcBlendAlpha,
           g_origRsSrcBlendAlpha, void(uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_SetRenderState_DestBlendAlpha,
           g_origRsDestBlendAlpha, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane0Enable,
           g_origFm2SetClipPlane0Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane1Enable,
           g_origFm2SetClipPlane1Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane2Enable,
           g_origFm2SetClipPlane2Enable, void(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_RenderContext_SetClipPlane3Enable,
           g_origFm2SetClipPlane3Enable, void(uint32_t, uint32_t));

REX_IMPORT(__imp__FM2_D3DDevice_CreateVertexBuffer, g_origCreateVertexBuffer,
           uint32_t(uint32_t));
REX_IMPORT(__imp__FM2_D3DDevice_CreateIndexBuffer, g_origCreateIndexBuffer,
           uint32_t(uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DVertexBuffer_Lock, g_origVertexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3D_LockGpuBufferRaw, g_origIndexBufferLock,
           uint32_t(void *, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__FM2_D3DSurface_GetDesc, g_origSurfaceGetDesc,
           void(void *, void *));
REX_IMPORT(__imp__FM2_D3DSurface_LockRect, g_origSurfaceLockRect,
           void(rr::GuestTexture *, void *, void *, uint32_t));
REX_IMPORT(__imp__FM2_D3DResource_UnlockResource, g_origUnlockResource,
           void(rr::GuestResource *, uint32_t, uint32_t));
// Renamed 2026-07-01 (see docs/FM2-ida-renames-2026-07-01.md): these are the
// XDK D3DDevice_Draw* primitives, compiler-inlined into these engine call
// sites and previously named for their PM4/engine role rather than their XDK
// identity. D3DDevice_DrawVertices was misnamed "Indexed" despite being the
// non-indexed primitive; the two indexed variants are separate inlined copies
// of D3DDevice_DrawIndexedVertices (one with fused vertex-format setup).
REX_IMPORT(__imp__D3DDevice_DrawVertices,
           g_origFm2EmitIndexedDrawPm4Packets,
           void(uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_DrawIndexedVertices,
           g_origFm2EmitIndexedDrawPm4PacketsWithGpuOffset,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
REX_IMPORT(__imp__D3DDevice_DrawIndexedVertices_WithVertexFormatSetup,
           g_origFm2EmitIndexedDrawPm4WithVertexFormatSetup,
           void(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t));
// 2026-07-01: originals for the newly-named XDK entry points below. These are
// used ONLY via .fn(ctx, base) raw passthrough (no typed marshaling), so the
// signature here is a placeholder -- same pattern as g_origRenderWorkerFrame.
REX_IMPORT(__imp__D3DDevice_ClearF, g_origD3DClearF, void(uint32_t));
REX_IMPORT(__imp__D3DDevice_DrawVerticesUP, g_origD3DDrawVerticesUP,
           void(uint32_t));
REX_IMPORT(__imp__D3DDevice_Resolve, g_origD3DResolve, void(uint32_t));
// Forward/opaque object pass draw emitter (a1=CDevice/context, a2=drawListNode,
// a3=flags). The depth prepass uses EmitIndexedDrawPm4* (hooked above); the
// forward COLOR pass goes through here and is NOT translated to native draws ->
// the color RT never gets written -> black. Diagnostic hook to decode the draw
// list before translating.
REX_IMPORT(__imp__FM2_D3D_EmitDirtyStateAndDrawList,
           g_origFm2EmitDirtyStateAndDrawList,
           void(uint32_t, uint32_t, uint32_t));
// Forza's PM4 draw-list interpreter (walks command nodes, dispatches opcodes).
// Hooked only to count whether the 3D-scene draw path runs in plume_native.
REX_IMPORT(__imp__FM2_Render_WalkAndDispatchPm4DrawList,
           g_origWalkAndDispatchPm4DrawList, void(uint32_t));
// PATH-LOGGING (2026-06-26): count whether the forward/object-pass call chain runs
// in plume_native. EmitDirtyStateAndDrawList (the forward COLOR/UI emitter) never
// fires; these are its callers, so counting them pinpoints where the render thread
// stops. RAW hooks (signature-agnostic: we only use g_orig.fn(ctx,base)).
REX_IMPORT(__imp__FM2_Render_ExecuteSortedDrawLists,
           g_origExecuteSortedDrawLists, void(uint32_t));
REX_IMPORT(__imp__FM2_Render_SubmitSortedObjectDrawLists,
           g_origSubmitSortedObjectDrawLists, void(uint32_t));
REX_IMPORT(__imp__FM2_Render_ObjectPassDrawTraversal,
           g_origObjectPassDrawTraversal, void(uint32_t));
REX_IMPORT(__imp__FM2_Render_PrepareAndWalkObjectPassDrawPackets,
           g_origPrepareAndWalkObjectPassDrawPackets, void(uint32_t));
REX_IMPORT(__imp__FM2_Render_UiOrScreenDrawListSubmit,
           g_origUiOrScreenDrawListSubmit, void(uint32_t));
// PATH-LOGGING upward: the SCENE-render entry chain (FramePipeline ->
// SubmitPassWrapper / ViewTraversal -> SceneSliceEntry -> ExecuteSortedDrawLists).
// Counting these locates exactly how high the scene-render dormancy reaches.
REX_IMPORT(__imp__FM2_Render_FramePipeline, g_origFramePipeline, void(uint32_t));
REX_IMPORT(__imp__FM2_Render_SubmitPassWrapper, g_origSubmitPassWrapper,
           void(uint32_t));
REX_IMPORT(__imp__FM2_Render_SceneSliceEntry, g_origSceneSliceEntry,
           void(uint32_t));
REX_IMPORT(__imp__FM2_Render_ViewTraversal, g_origViewTraversal, void(uint32_t));
// sub_825E8BF8 = the per-view scene-render DISPATCHER (vtable method). It calls
// FM2_Render_FramePipeline via (*(this+20))->vtable[+44]; its whole body is gated
// by *(this+2379). Runs ~4x/frame in Xenos, 0x in plume_native. Probe: learn
// whether plume_native (a) never reaches it, or (b) reaches it with the +2379
// enable flag clear (or a null pass object at this+20).
REX_IMPORT(__imp__sub_825E8BF8, g_origSceneViewDispatch, void(uint32_t));
// sub_8245D048 = deferred-callback QUEUE dispatcher (walks a node list, invokes
// each node's fn ptr *(node)). The scene render sub_825E8BF8 is one registered
// node. Called by FM2_RenderThread_RunFrame only when v11>*(a1+2100) (frame gate)
// && sub_8245D448(). Probe: in plume_native, is this called at all, and is the
// scene callback enqueued in its node list?
REX_IMPORT(__imp__sub_8245D048, g_origCbQueueDispatch, void(uint32_t));
// sub_8245CED8 = the ENQUEUE API (queue=r3, fn=r4, this=r5, arg2=r6, flag=r7):
// allocates an 0x18 node, node[0]=fn, node[4]=this, links it. The scene render is
// enqueued via fn=sub_825E8BF8. Hook + filter fn==0x825E8BF8 to capture the
// PRODUCER (caller) that schedules the scene render -- runs in Xenos, silent in
// plume_native.
REX_IMPORT(__imp__sub_8245CED8, g_origScheduleCallback, void(uint32_t));
// session 6P: pin WHY the scene/car render-pass action never fires in plume_native.
// FM2_TriggerMatchingListEntryActions (0x8234d5a8) walks an owner's (a1) intrusive entry
// list and fires the render-action vfunc for entries matching a2 with weight/enable set.
// sub_82279630 = the car-render CParams ctor (one such action). Log, per distinct (a1,a2),
// whether that owner's entry list is empty (sentinel=*(a1+20); empty iff *(sentinel)==
// sentinel), re-logging on status change; and tag the car render's actual (a1,a2). Diff
// Xenos vs plume: an owner non-empty in Xenos but empty in plume = the scene render-pass
// entries are never registered there.
REX_IMPORT(__imp__FM2_TriggerMatchingListEntryActions, g_origTriggerListActions,
           void(uint32_t));
REX_IMPORT(__imp__sub_82279630, g_origCarReflectionCtor, void(uint32_t));
// sub_82279610 = the CParams EXECUTE that reads the scene-view vtable[11]=0x825E8BF8 (verified via
// x64dbg: scene-vtable read fires at sub_82279610+0x181). It runs on the render thread under RunFrame
// in Xenos but NOT in plume. Capture ctx.lr at entry = the GUEST address of the DRAIN that invokes it
// (the unlabeled recompiled fn x64dbg couldn't name). Decompile that guest fn to find the plume gate.
REX_IMPORT(__imp__sub_82279610, g_origCarReflectionExec, void(uint32_t));
// sub_82363800 = FM2_AllocPoolBumpAllocate(pool=r3, size=r4) — every render CParams ctor allocs its
// node here. Filter ctx.lr to the render-CParams ctor range (0x82278xxx-0x8227Axxx) and log which POOL
// each ctor uses. The scene ctor sub_82279630 (size 0x1C) vs siblings SetClearColor(8)/BeginRender(0xC):
// if the scene CParams goes to a different/dead pool in plume (vs the drained queue), that's why its
// execute (sub_82279610) is never dispatched. Compare Xenos vs plume.
REX_IMPORT(__imp__sub_82363800, g_origAllocPoolBump, void(uint32_t));
// Renamed 2026-07-01 (docs/FM2-ida-renames-2026-07-01.md): this is D3D's
// per-draw predication-state flush (D3D::SetPending_Predicated), called from
// BeginVertices/DrawVertices/DrawIndexedVertices/Resolve alike -- NOT an EDRAM
// "resolve packet emitter". It was misidentified/misnamed by an earlier pass
// (the IDA database was later correctly typed as SetPending_Predicated, but
// the manifest name and this hook's logic were never updated to match), which
// is why the resolve-surface-aliasing logic that used to live in this hook
// fired on every draw instead of only at actual resolve time. See the real
// D3DDevice_Resolve (0x8237D158) for the function that should own that logic.
REX_IMPORT(__imp__D3D_SetPending_Predicated, g_origSetPendingPredicated,
           uint32_t(uint32_t, uint32_t, uint32_t));

namespace {

// Writes directly to the FM2 clean log so traces appear alongside LogLine output.
template <typename... Args>
static void LogReplayDbg(const char* fmt, Args&&... args) {
  FILE* f = std::fopen("C:\\temp\\fm2-clean.log", "a");
  if (!f) return;
  std::fprintf(f, fmt, std::forward<Args>(args)...);
  std::fputc('\n', f);
  std::fflush(f);
  std::fclose(f);
}

// Self-contained RenderDoc in-app capture trigger (no header/include-path dep).
// TriggerCapture is the 16th fn ptr in RENDERDOC_API_1_0_0 (renderdoc_app.h).
// When a frame draws real geometry (>threshold draws) we auto-trigger a capture
// so a *render* frame is captured without the user having to time the key.
// Layout matches RENDERDOC_API_1_0_1 fn-ptr order (renderdoc_app.h).
struct MiniRdocApi {
  void *fn[15];                        // GetAPIVersion .. GetCapture (0..14)
  void(__cdecl *TriggerCapture)(void); // 15
  void *fn2[3];                        // IsRemoteAccessConnected,LaunchReplayUI,SetActiveWindow (16..18)
  void(__cdecl *StartFrameCapture)(void *dev, void *wnd);       // 19
  void *IsFrameCapturing;                                       // 20
  uint32_t(__cdecl *EndFrameCapture)(void *dev, void *wnd);     // 21
};
std::atomic<uint32_t> g_drawsSinceLastPresent{0};
// A (CL-flow diagnostic): PM4 scene (3D) draws submitted since the last present.
// Logged + reset at present to tell whether scene draws reach the submitted CL.
std::atomic<uint32_t> g_sceneDrawsThisCL{0};
MiniRdocApi *RdocApi() {
  static MiniRdocApi *s_rdoc = nullptr;
  static bool s_tried = false;
  if (!s_tried) {
    s_tried = true;
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      using GetApiFn = int(__cdecl *)(int, void **);
      if (auto get = reinterpret_cast<GetApiFn>(
              GetProcAddress(mod, "RENDERDOC_GetAPI"))) {
        get(10000 /*eRENDERDOC_API_Version_1_0_0*/,
            reinterpret_cast<void **>(&s_rdoc));
      }
    }
    LogReplayDbg("FM2_RDOC_INIT renderdoc.dll=%s api=%s",
                 GetModuleHandleA("renderdoc.dll") ? "loaded" : "absent",
                 s_rdoc ? "ok" : "null");
  }
  return s_rdoc;
}
// Scene draws land on isolated/alternating frames, so TriggerCapture (next frame)
// keeps missing them. Instead wrap the WHOLE render-worker call (Start at top,
// before the draws execute, End after present) for a short window of frames once
// drawing is detected -- a real geometry frame is captured in full.
std::atomic<int> g_rdocCaptureRemaining{0};
std::atomic<bool> g_rdocArmed{false};
bool RenderDocFrameBegin() {
  MiniRdocApi *api = RdocApi();
  if (api == nullptr || g_rdocCaptureRemaining.load(std::memory_order_relaxed) <= 0)
    return false;
  api->StartFrameCapture(nullptr, nullptr);
  return true;
}
void RenderDocFrameEnd(bool capturing, uint32_t drawsThisFrame) {
  MiniRdocApi *api = RdocApi();
  if (capturing && api != nullptr) {
    api->EndFrameCapture(nullptr, nullptr);
    g_rdocCaptureRemaining.fetch_sub(1, std::memory_order_relaxed);
  }
  if (api == nullptr) {
    return;
  }
  // Capture a 4-frame window whenever the PREVIOUS frame was draw-dense (>8 draws =
  // the 3D car scene, not a sparse loading/blit frame), spaced >=200 present-frames
  // apart. With the submission fix the scene flows steadily, so consecutive frames
  // are both dense -- arming on a dense frame reliably captures a dense next frame
  // (unlike the old isolated-draw pattern that kept capturing blit frames).
  static std::atomic<uint32_t> s_frameCtr{0};
  static std::atomic<uint32_t> s_lastArm{0};
  const uint32_t fc = s_frameCtr.fetch_add(1, std::memory_order_relaxed);
  if (drawsThisFrame > 8u &&
      g_rdocCaptureRemaining.load(std::memory_order_relaxed) <= 0 &&
      fc - s_lastArm.load(std::memory_order_relaxed) >= 200u) {
    s_lastArm.store(fc, std::memory_order_relaxed);
    g_rdocCaptureRemaining.store(4, std::memory_order_relaxed);
    LogReplayDbg("FM2_RDOC_ARM frame=%u draws=%u capturing next 4 frames", fc,
                 drawsThisFrame);
  }
}

using rr::GuestBaseTexture;
using rr::GuestBuffer;
using rr::GuestDevice;
using rr::GuestShader;
using rr::GuestSurface;
using rr::GuestTexture;
using rr::GuestVertexDeclaration;

REXCVAR_DEFINE_UINT32(
    fm2_plume_gate_pulse_hz, 1000, "FM2",
    "plume_native: rate (Hz) to pulse the guest frame-sync gate so the game loop "
    "advances in every state. Default 1000 (far above any real framerate, so "
    "effectively uncapped) without pinning a core; 0 = truly unlimited spin.");
REXCVAR_DEFINE_BOOL(
    fm2_shader_resource_dump, false, "FM2",
    "Dump FM2 shader resource payloads loaded through title shader IDs.");
REXCVAR_DEFINE_UINT32(
    fm2_shader_resource_dump_limit, 128, "FM2",
    "Maximum unique FM2 shader resource payloads to dump in one run.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_render_hook_trace, false, "FM2",
    "Emit bounded diagnostics when FM2 native render hooks fire.");
REXCVAR_DEFINE_UINT32(
    fm2_plume_render_hook_trace_limit, 128, "FM2",
    "Maximum FM2 native render hook trace lines to emit per hook in one run; 0 is unlimited.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_vdswap_present, true, "FM2",
    "Mirror FM2's VdSwap frontbuffer into the active Plume swapchain.");
REXCVAR_DEFINE_BOOL(
    fm2_plume_bypass_prod_guard_wait, true, "FM2",
    "Make FM2 producer-progress guard report ready in Plume mode (required in "
    "plume_native/plume_clear: no Xenia GPU backend to advance completion fences).");

struct GuestRasterizerState {
  uint8_t pad0[8];
  rex::be<uint32_t> cullMode;
};
static_assert(offsetof(GuestRasterizerState, cullMode) == 8);

// Guest D3DLOCKED_RECT { DWORD Pitch; void* pBits; } (big-endian).
struct GuestLockedRect {
  rex::be<uint32_t> pitch;
  rex::be<uint32_t> bits;
};

template <typename T> T *AsFm2(T *p) {
  return rr::IsFm2Resource(p) ? p : nullptr;
}

uint32_t NextRenderHookTraceIndex(uint32_t &counter) {
  if (!REXCVAR_GET(fm2_plume_render_hook_trace)) {
    return 0;
  }
  const uint32_t index = ++counter;
  const uint32_t limit = REXCVAR_GET(fm2_plume_render_hook_trace_limit);
  if (limit != 0 && index > limit) {
    return 0;
  }
  return index;
}

bool ShouldMirrorPlumeRenderState() {
  return !nr::WantsReXGraphics();
}

// Per-second present diagnostics: prove which hook drives present, on which
// thread, and whether it has a valid present source. One line per second.
struct PresentDiagSlot {
  std::atomic<uint64_t> calls{0};
  std::atomic<uint64_t> withSource{0};
  std::atomic<uint32_t> lastTid{0};
  std::atomic<uint64_t> lastSec{0};
};

void CountPresentDiag(const char *label, PresentDiagSlot &slot,
                      const rr::GuestBaseTexture *presentSource) {
  slot.calls.fetch_add(1, std::memory_order_relaxed);
  if (presentSource != nullptr && presentSource->texture != nullptr) {
    slot.withSource.fetch_add(1, std::memory_order_relaxed);
  }
  slot.lastTid.store(static_cast<uint32_t>(::GetCurrentThreadId()),
                     std::memory_order_relaxed);
  using clock = std::chrono::steady_clock;
  const uint64_t nowSec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(clock::now().time_since_epoch())
          .count());
  uint64_t last = slot.lastSec.load(std::memory_order_relaxed);
  if (last == 0) {
    slot.lastSec.store(nowSec, std::memory_order_relaxed);
    return;
  }
  if (nowSec == last ||
      !slot.lastSec.compare_exchange_strong(last, nowSec, std::memory_order_relaxed)) {
    return;
  }
  const uint64_t calls = slot.calls.exchange(0, std::memory_order_relaxed);
  const uint64_t withSrc = slot.withSource.exchange(0, std::memory_order_relaxed);
  LogReplayDbg("FM2_PRESENT_DIAG sec=%llu hook=%s calls=%llu with_source=%llu tid=%u src=%p",
               (unsigned long long)nowSec, label, (unsigned long long)calls,
               (unsigned long long)withSrc,
               slot.lastTid.load(std::memory_order_relaxed),
               (const void *)presentSource);
}

// Per-second tally of native draw device availability. The PM4 draw emitters
// rely on the global active guest device (set by the Set* render-context hooks);
// if it is null the draw silently bails before opening a frame. This proves
// whether that is happening in steady state.
struct DrawDeviceSlot {
  std::atomic<uint64_t> ok{0};
  std::atomic<uint64_t> nullDev{0};
  std::atomic<uint64_t> lastSec{0};
};
void CountDrawDevice(const char *label, DrawDeviceSlot &slot, bool deviceOk) {
  (deviceOk ? slot.ok : slot.nullDev).fetch_add(1, std::memory_order_relaxed);
  using clock = std::chrono::steady_clock;
  const uint64_t nowSec = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(
          clock::now().time_since_epoch())
          .count());
  uint64_t last = slot.lastSec.load(std::memory_order_relaxed);
  if (last == 0) {
    slot.lastSec.store(nowSec, std::memory_order_relaxed);
    return;
  }
  if (nowSec == last ||
      !slot.lastSec.compare_exchange_strong(last, nowSec,
                                            std::memory_order_relaxed)) {
    return;
  }
  const uint64_t ok = slot.ok.exchange(0, std::memory_order_relaxed);
  const uint64_t nul = slot.nullDev.exchange(0, std::memory_order_relaxed);
  LogReplayDbg("FM2_DRAW_DEVICE sec=%llu hook=%s ok=%llu null=%llu",
               (unsigned long long)nowSec, label, (unsigned long long)ok,
               (unsigned long long)nul);
}

// Host-side pulse of the guest frame-sync event (handle stored at guest
// dword_829C24C0). Looks the handle up in the kernel object table and pulses the
// event directly. SAFE from any (native) thread: touches only host kernel
// objects, never recompiled guest code (calling guest code from a native thread
// crashes — no guest thread context). The event ref is cached to avoid object-
// table lock contention when pulsing at a high/unlimited rate.
void HostPulseFrameSyncGate() {
  auto *handlePtr = ghp::ToHost<rex::be<uint32_t>>(0x829C24C0u);
  if (handlePtr == nullptr)
    return;
  const uint32_t handle = handlePtr->get();
  if (handle == 0)
    return;
  static uint32_t s_cachedHandle = 0;
  static rex::system::object_ref<rex::system::XEvent> s_ev;
  if (handle != s_cachedHandle || !s_ev) {
    auto *ks = rex::system::kernel_state();
    if (ks == nullptr)
      return;
    s_ev = ks->object_table()->LookupObject<rex::system::XEvent>(handle);
    s_cachedHandle = handle;
  }
  if (s_ev)
    s_ev->Pulse(0, false);
}

// Persistent frame-sync gate driver: a dedicated thread pulses the gate so the
// guest game loop advances in EVERY game state (not just while the loading-screen
// frame loop runs). Safe because it pulses host-side (HostPulseFrameSyncGate).
// Rate via fm2_plume_gate_pulse_hz (0 = unlimited / uncapped framerate).
void StartGatePulseThreadOnce() {
  static std::once_flag s_once;
  std::call_once(s_once, [] {
    std::thread([] {
      static PresentDiagSlot s_gateThreadSlot;
      for (;;) {
        if (ShouldMirrorPlumeRenderState()) {
          HostPulseFrameSyncGate();
          CountPresentDiag("GateThread", s_gateThreadSlot, nullptr);
        }
        const uint32_t hz = REXCVAR_GET(fm2_plume_gate_pulse_hz);
        if (hz == 0) {
          std::this_thread::yield();
        } else {
          std::this_thread::sleep_for(std::chrono::microseconds(1000000u / hz));
        }
      }
    }).detach();
  });
}

uint32_t ReadGuestU32At(uint32_t guestAddress);

GuestDevice *DeviceForRenderContext(uint32_t renderContext) {
  if (renderContext != 0) {
    auto *device = ghp::ToHost<GuestDevice>(renderContext);
    rr::SetActiveGuestDevice(device);
    return device;
  }
  return rr::GetActiveGuestDevice();
}

void SetStreamSourceNative(GuestDevice *device, uint32_t stream,
                           GuestBuffer *buffer, uint32_t offset,
                           uint32_t stride, uint64_t mask);
void SetIndicesNative(GuestDevice *device, GuestBuffer *buffer);
void SetRenderTargetNative(GuestDevice *device, uint32_t index,
                           GuestSurface *surface);
void SetVertexShaderNative(GuestDevice *device, GuestShader *shader);
void SetPixelShaderNative(GuestDevice *device, GuestShader *shader);
void DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                         int32_t baseVertexIndex, uint32_t startIndex,
                         uint32_t indexCount);
void DrawVerticesUP(GuestDevice *device, uint32_t primType,
                    uint32_t vertexCount, void *data, uint32_t stride);

struct PendingImmediateDraw {
  GuestDevice *device = nullptr;
  uint32_t primType = 0;
  uint32_t vertexCount = 0;
  uint32_t stride = 0;
  uint32_t stagingAddr = 0;
  uint32_t stagingSize = 0;
};
PendingImmediateDraw g_pendingImmediateDraw;

void FlushImmediateVertices() {
  PendingImmediateDraw &p = g_pendingImmediateDraw;
  if (p.device == nullptr)
    return;
  GuestDevice *device = p.device;
  p.device = nullptr; 
  rr::DrawPrimitiveUP(device, p.primType, p.vertexCount,
                      ghp::ToHost<void>(p.stagingAddr), p.stride);
}

bool IsDepthFormat(plume::RenderFormat format) {
  return format == plume::RenderFormat::D16_UNORM ||
         format == plume::RenderFormat::D32_FLOAT ||
         format == plume::RenderFormat::D32_FLOAT_S8_UINT;
}

uint32_t BeginVertices(GuestDevice *device, uint32_t primType,
                       uint32_t vertexCount, uint32_t stride) {
  FlushImmediateVertices();
  const uint32_t size = vertexCount * stride;
  if (size == 0)
    return 0; // mirrors the guest's BeginRingAlloc-failure path
  PendingImmediateDraw &p = g_pendingImmediateDraw;
  if (size > p.stagingSize) {
    p.stagingAddr = ghp::GuestAllocRaw(size, 0x10); // grow-only staging
    p.stagingSize = size;
  }
  p.device = device;
  p.primType = primType;
  p.vertexCount = vertexCount;
  p.stride = stride;
  return p.stagingAddr;
}
constexpr uint32_t kScratchRingSize = 0x100000; // 1 MiB
constexpr uint32_t kScratchRingSlack =
    0x40000; // burst headroom past the threshold

uint32_t KickOff(GuestDevice *device) {
  static uint32_t s_scratchRing = 0;
  if (s_scratchRing == 0)
    s_scratchRing = ghp::GuestAllocRaw(kScratchRingSize, 0x100);
  auto *fields = reinterpret_cast<rex::be<uint32_t> *>(device);
  fields[48 / 4] = s_scratchRing; // write cursor
  fields[56 / 4] =
      s_scratchRing + kScratchRingSize - kScratchRingSlack; // kick threshold
  fields[13428 / 4] =
      s_scratchRing; // segment start (EndVertices cursor restore)
  return s_scratchRing;
}

void BlockOnSecondaryPosition(GuestDevice * /*device*/, uint32_t /*position*/,
                              uint32_t /*flags*/) {}

void Fm2Present(uint32_t presentChain) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2Present n={} presentChain=0x{:08X}",
                n, presentChain);
  }
  g_origFm2TryPresentAndUpdateStatus(presentChain);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  FlushImmediateVertices();
  // Diagnostic only: confirmed FM2_D3D_TryPresentAndUpdateStatus does NOT fire in
  // plume_native, so present is driven from Fm2GpuCommandBufferBuildAndSubmit
  // instead. Leave this so we notice if that ever changes.
  static PresentDiagSlot s_fm2PresentSlot;
  CountPresentDiag("Fm2Present", s_fm2PresentSlot, nullptr);
  Video::Present();
}

// The frontbuffer (final composited display) present source, decoded from the
// VdSwap fetch in Fm2GpuCommandBufferBuildAndSubmit. The RenderWorker present (which
// has no access to the VdSwap fetch) prefers this over GetLastDrawnColorRenderTarget.
static std::atomic<rr::GuestBaseTexture *> g_frontbufferPresentSource{nullptr};
// Defined later in this file (surface-aperture registry).
rr::GuestBaseTexture *LookupSurfaceAperture(uint32_t guestAddr);
// session 6P-2: with the draw pipeline healthy (PSO failures fixed), the guest
// frontbuffer RAM is the WRONG present source -- the disabled CP never resolves the
// scene into it, so it is black. Present the last-drawn color RT (actual rendered
// content) instead. Set true to restore the old VdSwap-fetch frontbuffer behavior.
static constexpr bool kPreferFrontbufferPresent = false;
void Fm2GpuCommandBufferBuildAndSubmit(uint32_t commandBuffer, uint32_t arg4,
                                       uint32_t arg5) {
  static uint32_t s_traceCount = 0;
  const uint32_t n = NextRenderHookTraceIndex(s_traceCount);
  const uint32_t cursorBefore = ReadGuestU32At(commandBuffer + 48u);
  const uint32_t kickThreshold = ReadGuestU32At(commandBuffer + 56u);
  const uint32_t ringBase = ReadGuestU32At(commandBuffer + 10768u);
  const uint32_t skipVdSwapFlag = ReadGuestU32At(commandBuffer + 21156u);
  const uint32_t pendingStart = ReadGuestU32At(commandBuffer + 21240u);
  const uint32_t pendingEnd = ReadGuestU32At(commandBuffer + 21244u);
  const uint32_t pendingEnabled = ReadGuestU32At(commandBuffer + 21248u);
  if (n != 0) {
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=GpuBuildSubmit n={} stage=entry "
        "cmd=0x{:08X} r4=0x{:08X} r5=0x{:08X} cursor=0x{:08X} "
        "kick=0x{:08X} ring=0x{:08X} skipVdSwapFlag=0x{:08X} "
        "pending={}->{} enabled=0x{:08X}",
        n, commandBuffer, arg4, arg5, cursorBefore, kickThreshold, ringBase,
        skipVdSwapFlag, pendingStart, pendingEnd, pendingEnabled);
  }

  if (ShouldMirrorPlumeRenderState()) {
    // plume_native / plume_clear: the Xenia CP thread is disabled, so the
    // original body would crash kicking the GPU ring buffer. We skip it -- but
    // the original ends with `bl VdSwap` (0x8236CD74), handing the FRONTBUFFER
    // (the final composited display image) to the present. The original NEVER
    // runs, so the FM2PlumeTraceVdSwap midasm (0x8236CD78) never fires. Replicate
    // it HERE using arg4: the frontbuffer D3D9 fetch constant is at arg4+28 (the
    // original does FM2_MemcpyAligned(&v48, a2+28, 24)); the frontbuffer base is
    // dword1 & 0xFFFFF000 (== v46). Present THAT, not the last-drawn RT (which is
    // just one black intermediate). Prefer a resolve-aliased surface at that base,
    // else upload the frontbuffer from guest RAM, else fall back to last-drawn.
    Video::ClaimPresentOwner();
    const uint32_t fetchAddr = arg4 + 28u;
    // 2026-07-02: the fetch word carries a 360 physical-alias address (e.g.
    // 0xE930F000, the 0xE0000000 cached-physical window); mask to the raw
    // physical page (0x0930F000) so it matches the aperture registry keys.
    const uint32_t fbBase =
        ReadGuestU32At(fetchAddr + 4u) & 0x1FFFFFFFu & ~0xFFFu;
    // 2026-07-02 (present-source fix): prefer the surface-aperture entry at
    // the frontbuffer fetch base. The D3DDevice_Resolve hook now registers
    // every resolve DESTINATION there, so an aperture hit here IS the game's
    // real composited display image (assembled from the EDRAM tile resolves).
    // Fall back to the last-drawn-RT heuristic only when no resolve has
    // targeted the frontbuffer base yet (early boot).
    const char *kind = "aperture";
    rr::GuestBaseTexture *presentSource = LookupSurfaceAperture(fbBase);
    if (presentSource == nullptr || presentSource->texture == nullptr) {
      presentSource = rr::GetLastDrawnColorRenderTarget();
      kind = "lastdrawn";
    }
    if (presentSource != nullptr && presentSource->texture != nullptr) {
      rr::SetPresentSource(presentSource);
      // Only remember APERTURE (real composite) sources for the RenderWorker
      // present path; storing the lastdrawn fallback would defeat its own
      // fallback ordering.
      if (kind[0] == 'a') {
        g_frontbufferPresentSource.store(presentSource,
                                         std::memory_order_relaxed);
      }
    }
    {
      const uint32_t sceneDraws =
          g_sceneDrawsThisCL.exchange(0, std::memory_order_relaxed);
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 120)
        LogReplayDbg(
            "FM2_GPUCMD_PRESENT fbBase=0x%08X kind=%s src=%p sceneDraws=%u",
            fbBase, kind, static_cast<void *>(presentSource), sceneDraws);
    }
    static PresentDiagSlot s_gpuCmdBufSlot;
    CountPresentDiag("GpuCmdBuf", s_gpuCmdBufSlot, presentSource);
    Video::Present();
    return;
  }

  g_origFm2GpuCommandBufferBuildAndSubmit(commandBuffer, arg4, arg5);

  if (n != 0) {
    const uint32_t cursorAfter = ReadGuestU32At(commandBuffer + 48u);
    const uint32_t lastStatus = ReadGuestU32At(commandBuffer + 21288u);
    const uint32_t flags = ReadGuestU32At(commandBuffer + 21896u);
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=GpuBuildSubmit n={} stage=exit "
        "cmd=0x{:08X} cursor=0x{:08X}->0x{:08X} lastStatus=0x{:08X} "
        "flags=0x{:08X}",
        n, commandBuffer, cursorBefore, cursorAfter, lastStatus, flags);
  }
}


void Fm2FmodIrqSubmitSafe(uint32_t device, uint32_t flags) {
  // In plume_native / plume_clear mode the Xenia GPU IRQ infrastructure is not
  // running, so the original would crash. Skip it entirely.
  if (ShouldMirrorPlumeRenderState()) {
    return;
  }
  g_origFm2FmodIrqSubmit8236C688(device, flags);
}

uint32_t Fm2ProducerProgressGuard(uint32_t state) {
  if (!ShouldMirrorPlumeRenderState() ||
      !REXCVAR_GET(fm2_plume_bypass_prod_guard_wait)) {
    return g_origFm2ProducerProgressGuard(state);
  }

  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO(
        "FM2_PLUME_RENDER_HOOK hook=ProducerProgressGuard n={} "
        "state=0x{:08X} bypass_return=0",
        n, state);
  }
  return 0;
}

void Fm2SetPixelShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetPixelShaderState(renderContext, shader);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=SetPixelShaderState n={} "
                "renderContext=0x{:08X} shader=0x{:08X} alias=0x{:08X}",
                n, renderContext, shader,
                ghp::ToGuest(rr::LookupShaderAlias(shader)));
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shader == 0) {
    return;
  }
  SetPixelShaderNative(device, ghp::ToHost<GuestShader>(shader));
}

void Fm2SetVertexShaderState(uint32_t renderContext, uint32_t shader) {
  g_origFm2SetVertexShaderState(renderContext, shader);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=SetVertexShaderState n={} "
                "renderContext=0x{:08X} shader=0x{:08X} alias=0x{:08X}",
                n, renderContext, shader,
                ghp::ToGuest(rr::LookupShaderAlias(shader)));
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || shader == 0) {
    return;
  }
  SetVertexShaderNative(device, ghp::ToHost<GuestShader>(shader));
}

void Fm2BindVertexStream(uint32_t renderContext, uint32_t slot,
                         uint32_t resource, uint32_t byte_offset,
                         uint32_t stride_bytes, uint64_t dirty_mask) {
  g_origFm2BindVertexStream(renderContext, slot, resource, byte_offset,
                            stride_bytes, dirty_mask);
  // TEMP DIAGNOSTIC 2026-07-01: does the standard SetStreamSource path ever
  // actually get called for the "big" streamed/compressed-geometry draws, or
  // do they bypass it entirely (leaving BindPm4GeometryFromContext reading
  // stale ctx+0x2F94 data from an unrelated earlier bind)? Log every call
  // with the resource's decoded physBase so it can be cross-checked against
  // FM2_PM4GEO_BIG's physBase for the same frame.
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 200) {
      const uint32_t physBase = resource ? (ReadGuestU32At(resource + 0x18u) & 0x1FFFFFFCu) : 0u;
      LogReplayDbg("FM2_BINDSTREAM ctx=0x%08X slot=%u res=0x%08X physBase=0x%08X "
                   "off=%u stride=%u",
                   renderContext, slot, resource, physBase, byte_offset,
                   stride_bytes);
    }
  }
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  // Prefer the native alias (uploaded Plume buffer); fall back to the raw XDK
  // header path in SetStreamSourceNative which reads the guest data pointer.
  GuestBuffer *buf = ghp::ToHost<GuestBuffer>(resource);
  if (!rr::IsFm2Resource(buf))
    if (GuestBuffer *alias = rr::LookupBufferAlias(resource))
      buf = alias;
  SetStreamSourceNative(device, slot, buf, byte_offset, stride_bytes, dirty_mask);
}

void Fm2BindIndexBuffer(uint32_t renderContext, uint32_t resource) {
  g_origFm2BindIndexBuffer(renderContext, resource);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  GuestBuffer *buf = ghp::ToHost<GuestBuffer>(resource);
  if (!rr::IsFm2Resource(buf))
    if (GuestBuffer *alias = rr::LookupBufferAlias(resource))
      buf = alias;
  SetIndicesNative(device, buf);
}

void Fm2SetBoundSurface(uint32_t renderContext, uint32_t surface,
                        uint32_t surfaceArg) {
  g_origFm2SetBoundSurface(renderContext, surface, surfaceArg);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surface == 0) {
    return;
  }
  // FM2_RenderContext_SetBoundSurface is the guest equivalent of BOTH
  // SetRenderTarget and SetDepthStencilSurface (per IDA). Classify the resource
  // and route depth surfaces to the depth slot instead of always binding color
  // index 0 -- otherwise depth is never bound and depth-tested draws sanitize to
  // depthStencilFormat=UNKNOWN.
  GuestSurface *gs = ghp::ToHost<GuestSurface>(surface);
  rr::GuestBaseTexture *tex = AsFm2(gs);
  if (tex == nullptr && gs != nullptr)
    tex = rr::TranslateGuestSurface(gs);
  if (tex != nullptr && tex->type == rr::ResourceType::DepthStencil) {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 8) {
      LogReplayDbg("FM2_BOUND_DEPTH surface=0x%08X tex=%p", surface,
                   (void *)tex);
    }
    // tex is already resolved + confirmed DepthStencil; bind it directly via the
    // native setter (the local d3d_hooks wrapper would just re-translate).
    rr::SetDepthStencilSurface(device, static_cast<GuestSurface *>(tex));
  } else {
    SetRenderTargetNative(device, 0, gs);
  }
}

void Fm2RememberGuestDevice(GuestDevice *device) {
  if (device != nullptr) {
    rr::SetActiveGuestDevice(device);
  }
}

// Forza binds its COLOR render target(s) via BindSurfaceInternal(ctx, slot, surface)
// (slot 0 = primary color RT), not the standard D3DDevice_SetRenderTarget. Without
// this, g_renderTarget is null, forward draws have no color attachment, and the
// screen stays at its clear color. Mirror the color surface into the native render
// target like the depth path mirrors SetBoundSurface.
void RegisterSurfaceApertures(uint32_t descriptorAddr,
                              rr::GuestBaseTexture *host);
void RegisterSurfaceAperture(uint32_t guestAddr, rr::GuestBaseTexture *host);

void Fm2BindSurface(uint32_t renderContext, uint32_t slot, uint32_t surface) {
  g_origBindSurfaceInternal(renderContext, slot, surface);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr || surface == 0) {
    return;
  }
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
    LogReplayDbg("FM2_BIND_SURFACE renderContext=0x%08X slot=%u surface=0x%08X",
                 renderContext, slot, surface);
  }
  GuestSurface *gs = ghp::ToHost<GuestSurface>(surface);
  rr::GuestBaseTexture *host = AsFm2(gs);
  if (host == nullptr && gs != nullptr)
    host = rr::TranslateGuestSurface(gs);
  RegisterSurfaceApertures(surface, host);
  SetRenderTargetNative(device, slot, gs);
}

// SetActivePassId (sub_8236E228) stores "passId" at renderContext+0x2D14. IDA
// (2026-06-26) shows this is a texture/shader-state token, NOT a guest
// D3DVERTEXELEMENT9 declaration: FM2_D3D_EmitIndexedDrawPacket packs it into
// FM2_D3D_EmitTextureStageStatePackets, and the real decl field is the separate
// GuestDevice+0x2E24 (never touched here).
// HOWEVER -- mirroring passId into device->vertexDeclaration is EMPIRICALLY
// LOAD-BEARING. It is the only thing that hands SyncVertexDeclarationFromDevice a
// non-zero handle to resolve (passId!=0 -> the alias path builds a usable layout;
// passId==0 -> MatchDeclarationForShader). Removing it (making the matcher the sole
// source) made MatchDeclarationForShader return null for ~EVERY draw -> ok=0, all
// draws skipped missing_decl -> total black (regression observed 2026-06-26,
// FM2_DRAW_OUTCOME ok=0 skip=1626). So keep the mirror as a band-aid until a proper
// per-draw declaration source (FM2 engine fetch constants) is wired in. See
// docs/FM2-ida-renames-2026-06-26.md + docs/FM2-plume-native-black-investigation.md.
void Fm2SetActivePassId(uint32_t renderContext, uint32_t passId) {
  g_origSetActivePassId(renderContext, passId);
  if (!ShouldMirrorPlumeRenderState())
    return;
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  device->vertexDeclaration = passId;
}

void Fm2DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                            int32_t baseVertexIndex, uint32_t startIndex,
                            uint32_t indexCount) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2DrawIndexedVertices n={} "
                "device={} prim={} base={} start={} indices={}",
                n, static_cast<const void *>(device), primType, baseVertexIndex,
                startIndex, indexCount);
  }
  Fm2RememberGuestDevice(device);
  DrawIndexedVertices(device, primType, baseVertexIndex, startIndex, indexCount);
}

void Fm2DrawVerticesUP(GuestDevice *device, uint32_t primType,
                       uint32_t vertexCount, void *data, uint32_t stride) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Fm2DrawVerticesUP n={} device={} "
                "prim={} vertices={} data={} stride={}",
                n, static_cast<const void *>(device), primType, vertexCount,
                data, stride);
  }
  Fm2RememberGuestDevice(device);
  DrawVerticesUP(device, primType, vertexCount, data, stride);
}

void Swap(GuestDevice *device, rr::GuestBaseTexture *frontBuffer,
          void * /*params*/) {
  static uint32_t s_traceCount = 0;
  const bool isFm2 = rr::IsFm2Resource(frontBuffer);
  const bool hasTexture = frontBuffer != nullptr && frontBuffer->texture != nullptr;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=Swap n={} device={} front={} "
                "isFm2={} hasTexture={} size={}x{} format={}",
                n, static_cast<const void *>(device),
                static_cast<const void *>(frontBuffer), isFm2, hasTexture,
                frontBuffer ? frontBuffer->width : 0,
                frontBuffer ? frontBuffer->height : 0,
                frontBuffer ? int(frontBuffer->format) : 0);
  }
  FlushImmediateVertices();
  static bool s_ringSeeded = false;
  if (!s_ringSeeded && device != nullptr) {
    KickOff(device);
    s_ringSeeded = true;
  }
  if (rr::IsFm2Resource(frontBuffer) && frontBuffer->texture != nullptr) {
    rr::SetImplicitRenderTarget(frontBuffer);
    rr::SetPresentSource(frontBuffer);
  }
  Video::Present();
}

// Neutralize these two wait primitives
void BlockOnFence() {}
void BlockUntilIdle() {}

void SynchronizeToPresentationInterval(GuestDevice * /*device*/,
                                       uint32_t /*interval*/) {}

void SetPredication(GuestDevice * /*device*/, uint32_t /*predicationMask*/) {}

void SetShaderGPRAllocation(GuestDevice * /*device*/, uint32_t /*flags*/,
                            uint32_t /*vsGprs*/, uint32_t /*psGprs*/) {}

GuestBuffer *CreateVertexBuffer(uint32_t length, uint32_t /*usage*/,
                                uint32_t /*pool*/) {
  return rr::CreateVertexBuffer(length);
}
GuestBuffer *CreateIndexBuffer(uint32_t length, uint32_t /*usage*/,
                               uint32_t format) {
  return rr::CreateIndexBuffer(length, format);
}
GuestTexture *CreateTexture(uint32_t width, uint32_t height, uint32_t depth,
                            uint32_t levels, uint32_t usage, uint32_t format,
                            uint32_t pool, uint32_t type) {
  return rr::CreateTexture(width, height, depth, levels, usage, format, pool,
                           type);
}
GuestSurface *CreateSurface(uint32_t width, uint32_t height, uint32_t format,
                            uint32_t multiSample) {
  return rr::CreateSurface(width, height, format, multiSample);
}
GuestVertexDeclaration *
CreateVertexDeclaration(rr::GuestVertexElement *elements) {
  return rr::CreateVertexDeclaration(elements);
}
GuestShader *CreateVertexShader(const uint32_t *function) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=CreateVertexShader n={} function={}",
                n, static_cast<const void *>(function));
  }
  return rr::CreateVertexShader(function);
}
GuestShader *CreatePixelShader(const uint32_t *function) {
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=CreatePixelShader n={} function={}",
                n, static_cast<const void *>(function));
  }
  return rr::CreatePixelShader(function);
}


uint32_t D3DXCreateTextureFromFileInMemoryEx(
    void * /*device*/, const uint8_t *srcData, uint32_t srcSize, uint32_t /*w*/,
    uint32_t /*h*/, uint32_t /*mips*/, uint32_t /*usage*/, uint32_t /*format*/,
    uint32_t /*pool*/, uint32_t /*filter*/, uint32_t /*mipFilter*/,
    uint32_t /*colorKey*/, void * /*srcInfo*/, void * /*palette*/,
    rex::be<uint32_t> *ppTexture) {
  GuestTexture *texture = rr::LoadTextureFromMemory(srcData, srcSize);
  if (ppTexture)
    *ppTexture = ghp::ToGuest(texture);
  return 0; // S_OK
}

uint32_t D3DXCreateTextureFromFileInMemory(void * /*device*/,
                                           const uint8_t *srcData,
                                           uint32_t srcSize,
                                           rex::be<uint32_t> *ppTexture) {
  GuestTexture *texture = rr::LoadTextureFromMemory(srcData, srcSize);
  if (ppTexture)
    *ppTexture = ghp::ToGuest(texture);
  return 0; // S_OK
}

uint32_t VertexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                          uint32_t flags);
uint32_t IndexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                         uint32_t flags);

void SurfaceLockRect(GuestTexture *texture, GuestLockedRect *lockedRect,
                     void *rect, uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origSurfaceLockRect(texture, lockedRect, rect, flags);
    return;
  }
  if (!rr::IsFm2Resource(texture))
    return; // genuine guest surface: ignore
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  if (lockedRect) {
    lockedRect->pitch = pitch;
    lockedRect->bits = bits;
  }
}

// Guest D3DLOCKED_TAIL { INT RowPitch; INT SlicePitch; void* pBits; } (BE).
struct GuestLockedTail {
  rex::be<int32_t> rowPitch;
  rex::be<int32_t> slicePitch;
  rex::be<uint32_t> bits;
};


uint32_t VertexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                          uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    return g_origVertexBufferLock(buffer, offset, size, flags);
  }
  if (rr::IsFm2Resource(buffer))
    return rr::LockVertexBuffer(buffer, flags);
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(buffer)))
    return rr::LockVertexBuffer(native, flags);
  return g_origVertexBufferLock(buffer, offset, size, flags);
}
uint32_t IndexBufferLock(GuestBuffer *buffer, uint32_t offset, uint32_t size,
                         uint32_t flags) {
  if (!ShouldMirrorPlumeRenderState()) {
    return g_origIndexBufferLock(buffer, offset, size, flags);
  }
  if (rr::IsFm2Resource(buffer))
    return rr::LockIndexBuffer(buffer, flags);
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(buffer)))
    return rr::LockIndexBuffer(native, flags);
  return g_origIndexBufferLock(buffer, offset, size, flags);
}

void XGSetVertexDeclaration(rr::GuestVertexElement *elements,
                            void *guestDeclaration) {
  rr::RegisterVertexDeclarationAlias(ghp::ToGuest(guestDeclaration),
                                     rr::CreateVertexDeclaration(elements));
}


struct GuestSurfaceDesc {
  rex::be<uint32_t> format;
  rex::be<uint32_t> type;
  rex::be<uint32_t> usage;
  rex::be<uint32_t> pool;
  rex::be<uint32_t> multiSampleType;
  rex::be<uint32_t> multiSampleQuality;
  rex::be<uint32_t> width;
  rex::be<uint32_t> height;
};

void BaseTextureLockTail(GuestTexture *texture, uint32_t arrayIndex,
                         GuestLockedTail *locked, uint32_t flags) {
  if (!rr::IsFm2Resource(texture)) {
    return;
  }
  if (locked == nullptr)
    return;
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  locked->rowPitch = int32_t(pitch);
  locked->slicePitch = int32_t(pitch * texture->height);
  locked->bits = bits;
}

void LockSurface(GuestTexture *texture, uint32_t arrayIndex, uint32_t level,
                 uint32_t flags, rex::be<uint32_t> *ppData,
                 rex::be<uint32_t> *pRowPitch, rex::be<uint32_t> *pSlicePitch,
                 rex::be<uint32_t> *pTailOffset) {
  if (!rr::IsFm2Resource(texture)) {
    return;
  }
  uint32_t pitch = 0, bits = 0;
  rr::LockTextureRect(texture, &pitch, &bits);
  if (ppData)
    *ppData = bits;
  if (pRowPitch)
    *pRowPitch = pitch;
  if (pSlicePitch)
    *pSlicePitch = pitch * texture->height;
  if (pTailOffset)
    *pTailOffset = 0;
}

// 2026-07-01 (black-screen fix, see docs/FM2-plume-native-vertex-pulling-gap-
// 2026-07-01.md): write the staged lock data through to the XDK buffer's real
// guest PHYSICAL memory. The Lock hook redirects the game's writes into a
// separate staging allocation (rr::LockBuffer) that Unlock uploads into the
// host Plume buffer only -- so the XDK buffer's own data address (the one the
// Xenos vertex/index FETCH constants reference, D3DResource+0x18) never
// received the data. The PM4 draw path (BindPm4GeometryFromContext) reads
// exactly that raw physical memory, so without this write-through every
// Lock-populated buffer looks like unwritten allocator fill at draw time
// (observed in RenderDoc: uniform 00 00 00 FF pattern -> NaN Float16 verts ->
// zero rasterized fragments -> black). ReOdyssey never needs this: its draws
// consume the host Plume buffer, never raw guest memory.
void WriteLockedDataThroughToGuest(rr::GuestResource *xdkResource,
                                   GuestBuffer *native) {
  if (native->lockedReadOnly || native->mappedMemory == nullptr)
    return;
  auto *mem = ghp::GuestMemory();
  if (mem == nullptr)
    return;
  const uint32_t xdkAddr = ghp::ToGuest(xdkResource);
  // Same base decode as BindPm4GeometryFromContext: vertex bases clear the 2
  // endian-select bits; index bases keep them (they are address bits there).
  const uint32_t baseMask = native->type == rr::ResourceType::IndexBuffer
                                ? 0x1FFFFFFFu
                                : 0x1FFFFFFCu;
  const uint32_t physBase = ReadGuestU32At(xdkAddr + 0x18u) & baseMask;
  // D3DResource+0x1C is fetch-constant encoded: bits[25:2] = size in dwords.
  const uint32_t fetchSize =
      ((ReadGuestU32At(xdkAddr + 0x1Cu) >> 2) & 0xFFFFFFu) * 4u;
  uint32_t copySize = native->dataSize;
  if (fetchSize != 0 && fetchSize < copySize)
    copySize = fetchSize;
  bool wrote = false;
  if (physBase != 0 && copySize != 0 && copySize <= 0x4000000u &&
      physBase + copySize <= 0x20000000u) {
    // Require writable pages: a host-side memcpy into a protected watch page
    // would raise an access violation the guest fault handler doesn't own.
    const auto access = mem->GetPhysicalHeap()->QueryRangeAccess(
        physBase, physBase + copySize - 1);
    if (access == rex::memory::PageAccess::kReadWrite ||
        access == rex::memory::PageAccess::kExecuteReadWrite) {
      if (uint8_t *dst = mem->TranslatePhysical<uint8_t *>(physBase)) {
        std::memcpy(dst, native->mappedMemory, copySize);
        wrote = true;
      }
    }
  }
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 32) {
    LogReplayDbg("FM2_LOCK_WRITETHROUGH xdk=0x%08X phys=0x%08X size=%u type=%d "
                 "ok=%d",
                 xdkAddr, physBase, copySize, int(native->type),
                 wrote ? 1 : 0);
  }
}

void UnlockResourceHook(rr::GuestResource *resource, uint32_t base,
                        uint32_t mip) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origUnlockResource(resource, base, mip);
    return;
  }
  if (rr::IsFm2Resource(resource)) {
    switch (resource->type) {
    case rr::ResourceType::VertexBuffer:
      rr::UnlockVertexBuffer(static_cast<GuestBuffer *>(resource)); break;
    case rr::ResourceType::IndexBuffer:
      rr::UnlockIndexBuffer(static_cast<GuestBuffer *>(resource)); break;
    case rr::ResourceType::Texture:
    case rr::ResourceType::VolumeTexture:
      rr::UnlockTextureRect(static_cast<GuestTexture *>(resource)); break;
    default: break;
    }
    return;
  }
  // XDK resource aliased by a creation hook: upload the staging data to the
  // host Plume buffer, then write it through to the XDK buffer's real guest
  // physical memory so the PM4 draw path (which reads raw guest memory via
  // the fetch constants) sees the same data.
  if (GuestBuffer *native = rr::LookupBufferAlias(ghp::ToGuest(resource))) {
    const bool wasLockedWritable =
        !native->lockedReadOnly && native->mappedMemory != nullptr;
    if (native->type == rr::ResourceType::VertexBuffer)
      rr::UnlockVertexBuffer(native);
    else
      rr::UnlockIndexBuffer(native);
    if (wasLockedWritable)
      WriteLockedDataThroughToGuest(resource, native);
  }
}

// ---------------------------------------------------------------------------
// State setters.
// ---------------------------------------------------------------------------

void SetTexture(GuestDevice *device, uint32_t sampler, GuestTexture *texture,
                uint64_t /*mask*/) {
  FlushImmediateVertices();
  GuestTexture *reo = AsFm2(texture);
  if (reo == nullptr && texture != nullptr)
    reo = rr::TranslateGuestTexture(texture,
                                    true); 
  if (reo == nullptr && texture != nullptr) {
    // Binding an untranslatable texture samples as zeros (black) -- warn so
    // dropped bindings are attributable instead of silent.
    static std::unordered_set<const void *> s_warned;
    if (s_warned.insert(texture).second) {
      REXGPU_WARN("SetTexture: slot {} texture {} untranslatable -- bound null",
                  sampler, static_cast<const void *>(texture));
    }
  }
  rr::SetTexture(device, sampler, reo);
}
GuestShader *ResolveShader(GuestShader *shader) {
  if (rr::IsFm2Resource(shader))
    return shader;
  return rr::LookupShaderAlias(ghp::ToGuest(shader));
}

GuestShader *ResolveFXeShader(void *fxeShader) {
  if (fxeShader == nullptr)
    return nullptr;
  if (auto *reoShader = AsFm2(static_cast<GuestShader *>(fxeShader)))
    return reoShader;
  if (auto *shader = rr::LookupShaderAlias(ghp::ToGuest(fxeShader)))
    return shader;
  auto *d3dShader = reinterpret_cast<rex::be<uint32_t> *>(
      static_cast<uint8_t *>(fxeShader) + 8);
  return rr::LookupShaderAlias(d3dShader->get());
}

GuestVertexDeclaration *ResolveVertexDeclaration(void *declaration) {
  auto *reoDecl = AsFm2(static_cast<GuestVertexDeclaration *>(declaration));
  if (reoDecl != nullptr)
    return reoDecl;
  return rr::LookupVertexDeclarationAlias(ghp::ToGuest(declaration));
}

struct BoundShaderStateInfo {
  GuestVertexDeclaration *declaration = nullptr;
  GuestShader *vertexShader = nullptr;
  GuestShader *pixelShader = nullptr;
};

struct BoundShaderStateKey {
  uint32_t vertexDeclaration = 0;
  uint32_t vertexShader = 0;
  uint32_t pixelShader = 0;
  std::array<uint8_t, 16> streamStrides{};

  bool operator==(const BoundShaderStateKey &) const = default;
};

struct BoundShaderStateKeyHash {
  size_t operator()(const BoundShaderStateKey &key) const {
    uint64_t hash = 1469598103934665603ull;
    auto mix = [&](uint32_t value) {
      hash ^= value;
      hash *= 1099511628211ull;
    };
    mix(key.vertexDeclaration);
    mix(key.vertexShader);
    mix(key.pixelShader);
    for (uint8_t stride : key.streamStrides)
      mix(stride);
    return static_cast<size_t>(hash);
  }
};

struct GuestBoundShaderState {
  rex::be<uint32_t> reserved = 0;
  rex::be<uint32_t> refCount = 1;
  rex::be<uint32_t> vertexShader = 0;
  rex::be<uint32_t> vertexDeclaration = 0;
};

std::unordered_map<uint32_t, BoundShaderStateInfo> g_boundShaderStates;
std::unordered_map<BoundShaderStateKey, uint32_t, BoundShaderStateKeyHash>
    g_boundShaderStateCache;

uint32_t ReadGuestU32(const void *p) {
  return p ? reinterpret_cast<const rex::be<uint32_t> *>(p)->get() : 0;
}

bool IsReadableGuestRange(uint32_t guestAddress, uint32_t byteCount) {
  if (guestAddress == 0 || byteCount == 0)
    return false;

  const uint32_t lastAddress = guestAddress + byteCount - 1u;
  if (lastAddress < guestAddress)
    return false;

  auto *memory = ghp::GuestMemory();
  auto *heap = memory ? memory->LookupHeap(guestAddress) : nullptr;
  if (heap == nullptr)
    return false;

  const rex::memory::PageAccess access =
      heap->QueryRangeAccess(guestAddress, lastAddress);
  return access != rex::memory::PageAccess::kNoAccess;
}

uint32_t ReadGuestU32At(uint32_t guestAddress) {
  if (!IsReadableGuestRange(guestAddress, sizeof(uint32_t)))
    return 0;
  return ReadGuestU32(ghp::ToHost<const void>(guestAddress));
}

uint32_t ReadShaderContainerByteCount(uint32_t shaderContainer) {
  if (!IsReadableGuestRange(shaderContainer, 12u))
    return 0;

  const uint32_t headerBytes = ReadGuestU32At(shaderContainer + 4u);
  const uint32_t payloadBytes = ReadGuestU32At(shaderContainer + 8u);
  if (headerBytes < 12u || payloadBytes == 0)
    return 0;

  const uint32_t totalBytes = headerBytes + payloadBytes;
  if (totalBytes < headerBytes || totalBytes > 0x40000u)
    return 0;
  if (!IsReadableGuestRange(shaderContainer, totalBytes))
    return 0;
  return totalBytes;
}

GuestShader *RegisterShaderAliasFromContainer(const char *hook,
                                              uint32_t guestShaderObject,
                                              uint32_t shaderContainer,
                                              bool vertexShader) {
  const uint32_t byteCount = ReadShaderContainerByteCount(shaderContainer);
  static uint32_t s_traceCount = 0;
  if (byteCount == 0) {
    if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
      REXGPU_WARN("FM2_PLUME_RENDER_HOOK hook={} n={} shader=0x{:08X} "
                  "container=0x{:08X} invalid shader container",
                  hook, n, guestShaderObject, shaderContainer);
    }
    return nullptr;
  }

  const auto *function = ghp::ToHost<const uint32_t>(shaderContainer);
  GuestShader *shader = vertexShader ? rr::CreateVertexShader(function)
                                     : rr::CreatePixelShader(function);
  rr::RegisterShaderAlias(guestShaderObject, shader);

  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    const uint64_t hash = XXH3_64bits(function, byteCount);
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook={} n={} shader=0x{:08X} "
                "container=0x{:08X} bytes=0x{:X} type={} hash=0x{:016X} "
                "alias=0x{:08X}",
                hook, n, guestShaderObject, shaderContainer, byteCount,
                vertexShader ? "vertex" : "pixel", hash, ghp::ToGuest(shader));
  }
  return shader;
}

bool IsPlausibleShaderPayloadByteCount(uint32_t byteCount) {
  return byteCount >= nr::kXenosUcodeInstructionByteStride &&
         byteCount <= 0x10000u && (byteCount & 3u) == 0;
}

uint32_t ChooseShaderPayloadByteCount(uint32_t guestEndianSizeField) {
  if (IsPlausibleShaderPayloadByteCount(guestEndianSizeField))
    return guestEndianSizeField;

  const uint32_t littleEndianCandidate =
      nr::DirectDrawLittleEndianValueFromGuestDword(guestEndianSizeField);
  if (IsPlausibleShaderPayloadByteCount(littleEndianCandidate))
    return littleEndianCandidate;

  return 0;
}

bool WriteBinaryFile(const std::filesystem::path &path, const void *data,
                     size_t size) {
  try {
    std::filesystem::create_directories(path.parent_path());
    FILE *f = nullptr;
#if defined(_MSC_VER)
    if (fopen_s(&f, path.string().c_str(), "wb") != 0)
      f = nullptr;
#else
    f = std::fopen(path.string().c_str(), "wb");
#endif
    if (f != nullptr) {
      const bool ok = std::fwrite(data, 1, size, f) == size;
      std::fclose(f);
      return ok;
    }
  } catch (const std::filesystem::filesystem_error &e) {
    REXGPU_WARN("FM2 shader resource dump path error: {}", e.what());
  }
  return false;
}

void DumpLoadedShaderResource(const char *kind, uint32_t outRef,
                              uint32_t shaderId, uint32_t payloadBaseOffset,
                              uint32_t ucodeOffset) {
  if (!REXCVAR_GET(fm2_shader_resource_dump))
    return;

  const uint32_t shaderResource = ReadGuestU32At(outRef);
  if (shaderResource == 0) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: null/unreadable output ref "
        "0x{:08X}",
        kind, shaderId, outRef);
    return;
  }

  if (!IsReadableGuestRange(shaderResource, 0x4Cu)) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: unreadable resource 0x{:08X}",
        kind, shaderId, shaderResource);
    return;
  }

  uint32_t shaderObject = shaderResource;
  uint32_t payloadBase = ReadGuestU32At(shaderObject + payloadBaseOffset);
  uint32_t resolvedObject = 0;
  if (payloadBase == 0) {
    resolvedObject =
        ReadGuestU32At(shaderResource + nr::kDirectDrawStateHandleResolvedObjectOffset);
    if (resolvedObject != 0 && IsReadableGuestRange(resolvedObject, 0x40u)) {
      const uint32_t resolvedPayloadBase =
          ReadGuestU32At(resolvedObject + payloadBaseOffset);
      if (resolvedPayloadBase != 0) {
        shaderObject = resolvedObject;
        payloadBase = resolvedPayloadBase;
      }
    }
  }

  if (payloadBase == 0) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: null payload field +0x{:02X} "
        "resource=0x{:08X} resolved=0x{:08X}",
        kind, shaderId, payloadBaseOffset, shaderResource, resolvedObject);
    return;
  }

  const uint32_t sizeField =
      ReadGuestU32At(shaderObject + nr::kDirectDrawPixelShaderPayloadByteCountOffset);
  const uint32_t knownPayloadBytes = ChooseShaderPayloadByteCount(sizeField);
  uint32_t payloadDumpBytes = nr::BoundedShaderPayloadDumpByteCount(
      nr::kDirectDrawShaderByteDumpMax, knownPayloadBytes);
  if (!IsReadableGuestRange(payloadBase, payloadDumpBytes)) {
    const uint32_t smallerPayloadDumpBytes =
        nr::BoundedShaderPayloadDumpByteCount(256u, knownPayloadBytes);
    if (smallerPayloadDumpBytes != payloadDumpBytes &&
        IsReadableGuestRange(payloadBase, smallerPayloadDumpBytes)) {
      payloadDumpBytes = smallerPayloadDumpBytes;
    }
  }

  if (payloadDumpBytes == 0 || !IsReadableGuestRange(payloadBase, payloadDumpBytes)) {
    REXGPU_WARN(
        "FM2 {} shader resource id=0x{:08X}: invalid payload 0x{:08X} "
        "object=0x{:08X} knownBytes=0x{:X} sizeField=0x{:08X}",
        kind, shaderId, payloadBase, shaderObject, knownPayloadBytes, sizeField);
    return;
  }

  const auto *payloadHost = ghp::ToHost<const uint8_t>(payloadBase);
  const uint64_t payloadHash = XXH3_64bits(payloadHost, payloadDumpBytes);
  static std::mutex s_dumpMutex;
  static std::unordered_set<uint64_t> s_dumpedPayloads;
  {
    std::lock_guard lock(s_dumpMutex);
    if (s_dumpedPayloads.contains(payloadHash))
      return;
    const uint32_t limit = REXCVAR_GET(fm2_shader_resource_dump_limit);
    if (limit != 0 && s_dumpedPayloads.size() >= limit)
      return;
    s_dumpedPayloads.insert(payloadHash);
  }

  const uint32_t ucodeDumpBytes = nr::BoundedShaderUcodeDumpByteCount(
      payloadDumpBytes, knownPayloadBytes, ucodeOffset);
  nr::DirectDrawShaderUcodeCandidate candidate;
  const uint32_t ucodeBase = payloadBase + ucodeOffset;
  const bool canScanUcode =
      ucodeBase >= payloadBase &&
      IsReadableGuestRange(ucodeBase, ucodeDumpBytes) &&
      ucodeDumpBytes >=
          nr::kXenosUcodeControlFlowPairDwordCount * sizeof(uint32_t);
  if (canScanUcode) {
    const auto *ucodeHost = ghp::ToHost<const uint8_t>(ucodeBase);
    candidate = nr::FindXenosUcodeCandidate(
        reinterpret_cast<const uint32_t *>(ucodeHost), ucodeDumpBytes / 4u);
  }

  const std::filesystem::path dumpDir =
      std::filesystem::path("missed_shaders") / "fm2_shader_resources";
  char payloadName[128];
  std::snprintf(payloadName, sizeof(payloadName),
                "%s_%08X_%016llX_payload.bin", kind, shaderId,
                static_cast<unsigned long long>(payloadHash));
  const std::filesystem::path payloadPath = dumpDir / payloadName;
  const bool wrotePayload =
      WriteBinaryFile(payloadPath, payloadHost, payloadDumpBytes);

  bool wroteUcode = false;
  uint64_t ucodeHash = 0;
  std::filesystem::path ucodePath;
  if (candidate.valid) {
    const uint32_t candidateByteOffset = ucodeOffset + candidate.byte_offset;
    const auto *candidateHost = payloadHost + candidateByteOffset;
    ucodeHash = XXH3_64bits(candidateHost, candidate.bounds.total_used_bytes);
    char ucodeName[128];
    std::snprintf(ucodeName, sizeof(ucodeName),
                  "%s_%08X_%016llX_ucode_o%04X.bin", kind, shaderId,
                  static_cast<unsigned long long>(ucodeHash),
                  candidateByteOffset);
    ucodePath = dumpDir / ucodeName;
    wroteUcode = WriteBinaryFile(ucodePath, candidateHost,
                                 candidate.bounds.total_used_bytes);
  }

  REXGPU_WARN(
      "FM2 {} shader resource captured id=0x{:08X} resource=0x{:08X} "
      "object=0x{:08X} payload=0x{:08X} sizeField=0x{:08X} knownBytes=0x{:X} "
      "dumpBytes=0x{:X} payloadHash=0x{:016X} candidate={} "
      "candidateOffset=0x{:X} candidateBytes=0x{:X} ucodeHash=0x{:016X} "
      "payloadFile={} ucodeFile={}",
      kind, shaderId, shaderResource, shaderObject, payloadBase, sizeField,
      knownPayloadBytes, payloadDumpBytes, payloadHash, candidate.valid,
      candidate.byte_offset, candidate.valid ? candidate.bounds.total_used_bytes : 0u,
      ucodeHash, wrotePayload ? payloadPath.string() : "<write-failed>",
      wroteUcode ? ucodePath.string() : "<none>");
}

uint32_t Fm2AllocGpuPassMemoryBlock(uint32_t shaderContainer) {
  const uint32_t shaderObject =
      g_origFm2AllocGpuPassMemoryBlock(shaderContainer);
  if (!ShouldMirrorPlumeRenderState()) {
    return shaderObject;
  }
  if (shaderObject != 0) {
    RegisterShaderAliasFromContainer("AllocGpuPassMemoryBlock", shaderObject,
                                     shaderContainer, false);
  }
  return shaderObject;
}

uint32_t Fm2CreateGpuMemoryBlock(uint32_t shaderContainer) {
  const uint32_t shaderObject = g_origFm2CreateGpuMemoryBlock(shaderContainer);
  if (!ShouldMirrorPlumeRenderState()) {
    return shaderObject;
  }
  if (shaderObject != 0) {
    RegisterShaderAliasFromContainer("CreateGpuMemoryBlock", shaderObject,
                                     shaderContainer, true);
  }
  return shaderObject;
}

void WriteGuestU32(void *p, uint32_t value) {
  if (p)
    *reinterpret_cast<rex::be<uint32_t> *>(p) = value;
}

BoundShaderStateKey MakeBoundShaderStateKey(void *vertexDeclaration,
                                            void *streamStrides,
                                            void *vertexShader,
                                            void *pixelShader) {
  BoundShaderStateKey key;
  key.vertexDeclaration = ghp::ToGuest(vertexDeclaration);
  key.vertexShader = ghp::ToGuest(vertexShader);
  key.pixelShader = ghp::ToGuest(pixelShader);
  auto *strides = static_cast<const rex::be<uint32_t> *>(streamStrides);
  if (strides != nullptr) {
    for (size_t i = 0; i < key.streamStrides.size(); ++i)
      key.streamStrides[i] = static_cast<uint8_t>(strides[i].get());
  }
  return key;
}

GuestBoundShaderState *CreateBoundShaderStateResource(void * /*cache*/,
                                                      void *vertexDeclaration,
                                                      void *streamStrides,
                                                      void *vertexShader,
                                                      void *pixelShader) {
  const BoundShaderStateKey key = MakeBoundShaderStateKey(
      vertexDeclaration, streamStrides, vertexShader, pixelShader);
  auto cached = g_boundShaderStateCache.find(key);
  if (cached != g_boundShaderStateCache.end())
    return ghp::ToHost<GuestBoundShaderState>(cached->second);

  uint32_t guestAddress =
      ghp::GuestAllocRaw(sizeof(GuestBoundShaderState), 0x10);
  if (guestAddress == 0)
    return nullptr;

  auto *state = ghp::ToHost<GuestBoundShaderState>(guestAddress);
  new (state) GuestBoundShaderState();
  state->vertexShader = ghp::ToGuest(vertexShader);
  state->vertexDeclaration = ghp::ToGuest(vertexDeclaration);

  g_boundShaderStates[guestAddress] = {
      ResolveVertexDeclaration(vertexDeclaration),
      ResolveFXeShader(vertexShader),
      ResolveFXeShader(pixelShader),
  };
  g_boundShaderStateCache[key] = guestAddress;
  return state;
}

void RHICreateBoundShaderState(void *outRef, void *vertexDeclaration,
                               void *streamStrides, void *vertexShader,
                               void *pixelShader) {
  GuestBoundShaderState *state = CreateBoundShaderStateResource(
      nullptr, vertexDeclaration, streamStrides, vertexShader, pixelShader);
  WriteGuestU32(outRef, ghp::ToGuest(state));
  WriteGuestU32(static_cast<uint8_t *>(outRef) + 4, 0);
}

void *AssignBoundShaderStateRef(void *dst, const void *src) {
  WriteGuestU32(dst, ReadGuestU32(src));
  WriteGuestU32(static_cast<uint8_t *>(dst) + 4, 0);
  return dst;
}

void CopyConstructBoundShaderStateRef(void *dst, const void *src) {
  AssignBoundShaderStateRef(dst, src);
}

void ReleaseBoundShaderStateRef(void *ref) {
  WriteGuestU32(ref, 0);
  WriteGuestU32(static_cast<uint8_t *>(ref) + 4, 0);
}

void SetBoundShaderState(GuestDevice *device, void *boundStateRef) {
  FlushImmediateVertices();
  const uint32_t boundState = ReadGuestU32(boundStateRef);
  if (boundState == 0) {
    rr::SetVertexDeclaration(device, nullptr);
    rr::SetVertexShader(device, nullptr);
    rr::SetPixelShader(device, nullptr);
    return;
  }
  auto it = g_boundShaderStates.find(boundState);
  if (it == g_boundShaderStates.end()) {
    REXGPU_WARN("SetBoundShaderState: unknown bound state 0x{:08X}",
                boundState);
    rr::SetVertexDeclaration(device, nullptr);
    rr::SetVertexShader(device, nullptr);
    rr::SetPixelShader(device, nullptr);
    return;
  }

  const BoundShaderStateInfo &info = it->second;
  rr::SetVertexDeclaration(device, info.declaration);
  rr::SetVertexShader(device, info.vertexShader);
  rr::SetPixelShader(device, info.pixelShader);
}

void SetVertexShaderNative(GuestDevice *device, GuestShader *shader) {
  FlushImmediateVertices();
  rr::SetVertexShader(device, ResolveShader(shader));
}

void SetPixelShaderNative(GuestDevice *device, GuestShader *shader) {
  FlushImmediateVertices();
  rr::SetPixelShader(device, ResolveShader(shader));
}

void Fm2LoadPixelShaderResourceById(uint32_t outRef, uint32_t shaderId) {
  g_origFm2LoadPixelShaderResourceById(outRef, shaderId);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=LoadPixelShaderResource n={} "
                "outRef=0x{:08X} shaderId=0x{:08X} resource=0x{:08X}",
                n, outRef, shaderId, ReadGuestU32At(outRef));
  }
  DumpLoadedShaderResource(
      "pixel", outRef, shaderId,
      nr::kDirectDrawPixelShaderPayloadGpuBaseOffset,
      nr::kDirectDrawPixelShaderPayloadUcodeOffset);
}

void Fm2LoadVertexShaderResourceById(uint32_t outRef, uint32_t shaderId) {
  g_origFm2LoadVertexShaderResourceById(outRef, shaderId);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=LoadVertexShaderResource n={} "
                "outRef=0x{:08X} shaderId=0x{:08X} resource=0x{:08X}",
                n, outRef, shaderId, ReadGuestU32At(outRef));
  }
  DumpLoadedShaderResource(
      "vertex", outRef, shaderId,
      nr::kDirectDrawVertexShaderPayloadGpuBaseOffset,
      nr::kDirectDrawVertexShaderPayloadUcodeOffset);
}

void SetVertexShaderConstantFN(GuestDevice *device, uint32_t startRegister,
                               const uint32_t *data, uint32_t vector4fCount) {
  FlushImmediateVertices();
  if (device == nullptr || data == nullptr || startRegister >= 0x100)
    return;
  const uint32_t count = std::min(vector4fCount, 0x100u - startRegister);
  std::memcpy(device->vertexShaderFloatConstants + startRegister * 4, data,
              count * 16);
}

void SetPixelShaderConstantFN(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t vector4fCount) {
  FlushImmediateVertices();
  if (device == nullptr || data == nullptr || startRegister >= 0x100)
    return;
  const uint32_t count = std::min(vector4fCount, 0x100u - startRegister);
  std::memcpy(device->pixelShaderFloatConstants + startRegister * 4, data,
              count * 16);
}

void SetShaderConstantB(rex::be<uint32_t> *constants, uint32_t constantCount,
                        uint32_t startRegister, const uint32_t *data,
                        uint32_t boolCount) {
  if (constants == nullptr || data == nullptr ||
      startRegister >= constantCount * 32)
    return;
  const uint32_t count =
      std::min(boolCount, constantCount * 32 - startRegister);
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t bit = startRegister + i;
    uint32_t value = constants[bit / 32].get();
    const uint32_t mask = 1u << (bit & 31);
    if ((std::byteswap(data[i]) & 1u) != 0)
      value |= mask;
    else
      value &= ~mask;
    constants[bit / 32] = value;
  }
}

void SetVertexShaderConstantB(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t boolCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantB(device->vertexShaderBoolConstants,
                     uint32_t(std::size(device->vertexShaderBoolConstants)),
                     startRegister, data, boolCount);
}

void SetPixelShaderConstantB(GuestDevice *device, uint32_t startRegister,
                             const uint32_t *data, uint32_t boolCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantB(device->pixelShaderBoolConstants,
                     uint32_t(std::size(device->pixelShaderBoolConstants)),
                     startRegister, data, boolCount);
}

void SetShaderConstantI(rex::be<uint32_t> *constants, uint32_t constantCount,
                        uint32_t startRegister, const uint32_t *data,
                        uint32_t intCount) {
  if (constants == nullptr || data == nullptr || startRegister >= constantCount)
    return;
  const uint32_t count = std::min(intCount, constantCount - startRegister);
  for (uint32_t i = 0; i < count; ++i) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data + i * 4);
    constants[startRegister + i] = (uint32_t(bytes[11]) << 16) |
                                   (uint32_t(bytes[7]) << 8) |
                                   uint32_t(bytes[3]);
  }
}

void SetVertexShaderConstantI(GuestDevice *device, uint32_t startRegister,
                              const uint32_t *data, uint32_t intCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantI(device->vertexShaderIntConstants,
                     uint32_t(std::size(device->vertexShaderIntConstants)),
                     startRegister, data, intCount);
}

void SetPixelShaderConstantI(GuestDevice *device, uint32_t startRegister,
                             const uint32_t *data, uint32_t intCount) {
  FlushImmediateVertices();
  if (device == nullptr)
    return;
  SetShaderConstantI(device->pixelShaderIntConstants,
                     uint32_t(std::size(device->pixelShaderIntConstants)),
                     startRegister, data, intCount);
}

struct GuestD3DVertexBufferHeader {
  rex::be<uint32_t> common; // type 1 in the low nibble
  rex::be<uint32_t> refCount;
  rex::be<uint32_t> fence;
  rex::be<uint32_t> readFence;
  rex::be<uint32_t> identifier;
  rex::be<uint32_t> baseFlush;
  rex::be<uint32_t> format0; // byteAddress | 3 (vertex fetch constant)
  rex::be<uint32_t> format1; // length in bits 2..25, endian bits low
};

struct GuestD3DIndexBufferHeader {
  rex::be<uint32_t> common; // type 2 in the low nibble, D3DFORMAT << 29
  rex::be<uint32_t> refCount;
  rex::be<uint32_t> fence;
  rex::be<uint32_t> readFence;
  rex::be<uint32_t> identifier;
  rex::be<uint32_t> baseFlush;
  rex::be<uint32_t> address; // guest byte address of the index data
  rex::be<uint32_t> size;    // byte size
};

void SetStreamSourceNative(GuestDevice *device, uint32_t stream,
                           GuestBuffer *buffer, uint32_t offset,
                           uint32_t stride, uint64_t /*mask*/) {
  FlushImmediateVertices();
  GuestBuffer *reo = AsFm2(buffer);
  if (reo == nullptr && buffer != nullptr) {
    const auto *header =
        reinterpret_cast<const GuestD3DVertexBufferHeader *>(buffer);
    if ((header->common.get() & 0xF) == 1) {
      const uint32_t address = header->format0.get() & ~3u;
      const uint32_t size = header->format1.get() & 0x3FFFFFCu;
      if (address != 0 && size != 0 && offset < size) {
        rr::SetStreamSourceGuestData(device, stream,
                                     ghp::ToHost<void>(address + offset),
                                     size - offset, stride);
        return;
      }
    }
  }
  rr::SetStreamSource(device, stream, reo, offset, stride);
}
void SetIndicesNative(GuestDevice *device, GuestBuffer *buffer) {
  FlushImmediateVertices();
  GuestBuffer *reo = AsFm2(buffer);
  const char *path = "none";
  if (reo == nullptr && buffer != nullptr) {
    const auto *header =
        reinterpret_cast<const GuestD3DIndexBufferHeader *>(buffer);
    if ((header->common.get() & 0xF) == 2) {
      const uint32_t address = header->address.get();
      const uint32_t size = header->size.get();
      // D3DFORMAT in the top 3 bits: 1 = INDEX16, 6 = INDEX32.
      const uint32_t indexStride = (header->common.get() >> 29) == 6 ? 4 : 2;
      if (address != 0 && size != 0) {
        rr::SetIndicesGuestData(device, ghp::ToHost<void>(address), size,
                                indexStride);
        static std::atomic<uint32_t> s_n{0};
        if (s_n.fetch_add(1, std::memory_order_relaxed) < 8)
          LogReplayDbg("FM2_SETIDX header addr=0x%08X size=%u stride=%u", address,
                       size, indexStride);
        return;
      }
    }
    path = "header_fail";
  } else if (reo != nullptr) {
    path = "fm2res";
  }
  static std::atomic<uint32_t> s_n2{0};
  if (s_n2.fetch_add(1, std::memory_order_relaxed) < 8)
    LogReplayDbg("FM2_SETIDX path=%s reo=%p buffer=%p", path, (void *)reo,
                 (void *)buffer);
  rr::SetIndices(device, reo);
}
void SetViewport(GuestDevice *device, rr::GuestViewport *viewport) {
  FlushImmediateVertices();
  rr::SetViewport(device, viewport);
}
void SetScissorRect(GuestDevice *device, rr::GuestRect *rect) {
  FlushImmediateVertices();
  rr::SetScissorRect(device, rect);
}
void SetRenderTargetNative(GuestDevice *device, uint32_t index,
                           GuestSurface *surface) {
  FlushImmediateVertices();
  rr::GuestBaseTexture *reo = AsFm2(surface);
  if (reo == nullptr && surface != nullptr)
    reo = rr::TranslateGuestSurface(
        surface); // UE3 RHI-built (XGSetSurfaceHeader) surface
  rr::SetRenderTarget(device, index, reo);
}
void SetDepthStencilSurface(GuestDevice *device, GuestSurface *surface) {
  FlushImmediateVertices();
  GuestSurface *reo = AsFm2(surface);
  if (reo == nullptr && surface != nullptr) {
    rr::GuestBaseTexture *translated = rr::TranslateGuestSurface(surface);
    if (translated != nullptr &&
        translated->type == rr::ResourceType::DepthStencil) {
      reo = static_cast<GuestSurface *>(translated);
    } else if (translated != nullptr) {
      static std::unordered_set<const void *> s_warned;
      if (s_warned.insert(surface).second) {
        REXGPU_WARN("SetDepthStencilSurface: {} translated to non-depth "
                    "resource (type {}) -- bound null",
                    (const void *)surface, int(translated->type));
      }
    }
  }
  rr::SetDepthStencilSurface(device, reo);
}

// ?RHISetDepthState@@YAXPAVFD3DDepthState@@@Z

void RHISetDepthState(GuestDevice *device, void *depthStateGuest) {
  FlushImmediateVertices();

  const auto *ds = reinterpret_cast<const rex::be<uint32_t> *>(depthStateGuest);
  if (ds == nullptr)
    return;
  const uint32_t zEnable = ds[1].get();
  const uint32_t zWrite = ds[2].get();
  const uint32_t cmpFunc = ds[3].get();

  rr::SetDepthState(zEnable, zWrite, cmpFunc);
}

void RHISetStencilState(GuestDevice *device, void *stencilStateGuest) {
  FlushImmediateVertices();

  const auto *ss =
      reinterpret_cast<const rex::be<uint32_t> *>(stencilStateGuest);
  if (ss == nullptr)
    return;

  rr::GuestStencilState s;
  s.enable = ss[1].get() != 0;
  s.twoSided = ss[2].get() != 0;
  s.frontFunc = ss[3].get();
  s.frontFail = ss[4].get();
  s.frontDepthFail = ss[5].get();
  s.frontPass = ss[6].get();
  s.backFunc = ss[7].get();
  s.backFail = ss[8].get();
  s.backDepthFail = ss[9].get();
  s.backPass = ss[10].get();
  s.readMask = ss[11].get();
  s.writeMask = ss[12].get();
  s.ref = ss[13].get();
  rr::SetStencilState(s);
}


uint32_t RHIGetOcclusionQueryResult(void * /*query*/, void *outNumPixels,
                                    uint32_t /*bWait*/) {
  if (outNumPixels != nullptr)
    *reinterpret_cast<rex::be<uint32_t> *>(outNumPixels) = 0x00100000u;
  return 1; // S_OK: result available
}

// D3DDevice_ClearF(device, flags, rect, D3DVECTOR4* color, Z, d3dColor). Color
// is 4 big-endian floats in guest memory.
void ClearF(GuestDevice *device, uint32_t flags, void * /*rect*/,
            const uint32_t *color, float z, uint32_t /*d3dColor*/) {
  FlushImmediateVertices();
  float rgba[4] = {0, 0, 0, 0};
  if (color) {
    for (int i = 0; i < 4; ++i) {
      uint32_t bits = std::byteswap(color[i]);
      rgba[i] = std::bit_cast<float>(bits);
    }
  }
  rr::Clear(device, flags, rgba, z);
}

void Resolve(GuestDevice *device, uint32_t flags, const rr::GuestRect *source,
             GuestBaseTexture *destination, const rr::GuestPoint *destPoint,
             uint32_t /*destLevel*/, uint32_t /*destSliceOrFace*/,
             const void * /*clearColor*/, float /*clearZ*/,
             uint32_t /*clearStencil*/, const void * /*parameters*/) {
  FlushImmediateVertices();

  GuestBaseTexture *reo = AsFm2(destination);
  if (reo == nullptr && destination != nullptr) {
    reo = rr::TranslateGuestTexture(destination, false);
  }
  if (reo == nullptr && destination != nullptr) {
    static std::unordered_set<const void *> s_warned;
    if (s_warned.insert(destination).second) {
      REXGPU_WARN("Resolve: destination {} untranslatable -- resolve DROPPED",
                  static_cast<const void *>(destination));
    }
  }
  // TEMP DIAGNOSTIC 2026-07-01: confirm D3DDevice_Resolve now fires at all
  // (it was never hooked before today) and whether destination translates.
  // Remove once the present pipeline is confirmed to consume resolved data.
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 96) {
      GuestBaseTexture *rt = rr::GetCurrentColorRenderTarget();
      LogReplayDbg("FM2_RESOLVE_HOOK dest=%p reo=%p reoTex=%p srcRT=%p "
                   "srcRTtex=%p flags=0x%X destPt=(%d,%d) srcRect=(%d,%d,%d,%d)",
                   static_cast<void *>(destination), static_cast<void *>(reo),
                   reo ? static_cast<void *>(reo->texture) : nullptr,
                   static_cast<void *>(rt),
                   rt ? static_cast<void *>(rt->texture) : nullptr, flags,
                   destPoint ? destPoint->x.get() : -1,
                   destPoint ? destPoint->y.get() : -1,
                   source ? source->left.get() : -1,
                   source ? source->top.get() : -1,
                   source ? source->right.get() : -1,
                   source ? source->bottom.get() : -1);
    }
  }
  // 2026-07-02 (present-source fix): register the resolve DESTINATION in the
  // surface-aperture registry keyed by its guest data address (scanned from
  // the destination texture's header words). The D3DDevice_Swap frontbuffer
  // fetch lookup (LookupSurfaceAperture(fbBase)) then presents the real
  // composited frame instead of the last-drawn tile RT. This is the correct,
  // resolve-driven replacement for the aliasing that was stripped from the
  // misidentified SetPending_Predicated hook.
  if (reo != nullptr && reo->texture != nullptr && destination != nullptr) {
    // Register ONLY the destination texture's data base: D3DBaseTexture header
    // word at +32 (fetch form; confirmed in the D3DDevice_Resolve decompile as
    // `*(dest+32) & 0xFFFFF000` = the resolve copy-dest base), masked to the
    // physical page. A broad header scan would register every plausible word
    // and alias unrelated textures over the frontbuffer base -> garbage frame.
    const uint32_t destAddr = ghp::ToGuest(destination);
    const uint32_t dataBase =
        ReadGuestU32At(destAddr + 32u) & 0x1FFFFFFFu & ~0xFFFu;
    RegisterSurfaceAperture(dataBase, reo);
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 24) {
      LogReplayDbg("FM2_RESOLVE_APERTURE dest=0x%08X dataBase=0x%08X reo=%p "
                   "%ux%u",
                   destAddr, dataBase, static_cast<void *>(reo), reo->width,
                   reo->height);
    }
  }
  rr::StretchRect(device, flags, source, reo, destPoint);
}

#define RENDER_STATE_HOOK(fn, d3drs)                                           \
  void fn(GuestDevice *device, uint32_t value) {                               \
    FlushImmediateVertices();                                                  \
    rr::SetRenderState(device, rr::d3drs, value);                              \
  }
RENDER_STATE_HOOK(RsAlphaBlendEnable, D3DRS_ALPHABLENDENABLE)
RENDER_STATE_HOOK(RsAlphaTestEnable, D3DRS_ALPHATESTENABLE)
RENDER_STATE_HOOK(RsBlendOp, D3DRS_BLENDOP)
RENDER_STATE_HOOK(RsBlendOpAlpha, D3DRS_BLENDOPALPHA)
RENDER_STATE_HOOK(RsColorWriteEnable, D3DRS_COLORWRITEENABLE)
RENDER_STATE_HOOK(RsDepthBias, D3DRS_DEPTHBIAS)
RENDER_STATE_HOOK(RsDestBlend, D3DRS_DESTBLEND)
RENDER_STATE_HOOK(RsDestBlendAlpha, D3DRS_DESTBLENDALPHA)
RENDER_STATE_HOOK(RsSlopeScaleDepthBias, D3DRS_SLOPESCALEDEPTHBIAS)
RENDER_STATE_HOOK(RsSrcBlend, D3DRS_SRCBLEND)
RENDER_STATE_HOOK(RsSrcBlendAlpha, D3DRS_SRCBLENDALPHA)
RENDER_STATE_HOOK(RsZEnable, D3DRS_ZENABLE)
#undef RENDER_STATE_HOOK

void MirrorFm2RenderState(uint32_t renderContext, uint32_t state,
                          uint32_t value) {
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  FlushImmediateVertices();
  rr::SetRenderState(device, state, value);
}

void MirrorFm2ClipPlanes(uint32_t renderContext) {
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  GuestDevice *device = DeviceForRenderContext(renderContext);
  if (device == nullptr)
    return;
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

// Rewired 2026-07-01: correct XDK-identity render-state mirrors (see the
// import block comment). Each calls the original first (mirror architecture),
// then forwards the API-level value into rr:: under its REAL D3DRS semantic.
void Fm2RsAlphaBlendEnable(uint32_t device, uint32_t value) {
  g_origRsAlphaBlendEnable(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_ALPHABLENDENABLE, value);
}

void Fm2RsAlphaTestEnable(uint32_t device, uint32_t value) {
  g_origRsAlphaTestEnable(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_ALPHATESTENABLE, value);
}

void Fm2RsZEnable(uint32_t device, uint32_t value) {
  g_origRsZEnable(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_ZENABLE, value);
}

void Fm2RsZWriteEnable(uint32_t device, uint32_t value) {
  g_origRsZWriteEnable(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_ZWRITEENABLE, value);
}

void Fm2RsZFunc(uint32_t device, uint32_t value) {
  g_origRsZFunc(device, value);
  // value is the 360 D3DCMP enum (0-based, == Xenos hw ZFUNC field), exactly
  // what rr::SetRenderState's ConvertCmpFunc expects (guest_device.h enums).
  MirrorFm2RenderState(device, rr::D3DRS_ZFUNC, value);
}

void Fm2RsColorWriteEnable(uint32_t device, uint32_t value) {
  g_origRsColorWriteEnable(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_COLORWRITEENABLE, value);
}

void Fm2RsBlendOp(uint32_t device, uint32_t value) {
  g_origRsBlendOp(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_BLENDOP, value);
}

void Fm2RsSrcBlend(uint32_t device, uint32_t value) {
  g_origRsSrcBlend(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_SRCBLEND, value);
}

void Fm2RsDestBlend(uint32_t device, uint32_t value) {
  g_origRsDestBlend(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_DESTBLEND, value);
}

void Fm2RsSrcBlendAlpha(uint32_t device, uint32_t value) {
  g_origRsSrcBlendAlpha(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_SRCBLENDALPHA, value);
}

void Fm2RsDestBlendAlpha(uint32_t device, uint32_t value) {
  g_origRsDestBlendAlpha(device, value);
  MirrorFm2RenderState(device, rr::D3DRS_DESTBLENDALPHA, value);
}

void Fm2SetClipPlane0Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane0Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane1Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane1Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane2Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane2Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void Fm2SetClipPlane3Enable(uint32_t renderContext, uint32_t value) {
  g_origFm2SetClipPlane3Enable(renderContext, value);
  MirrorFm2ClipPlanes(renderContext);
}

void ApplyRasterizerState(GuestDevice *device, void *rasterizerStateGuest) {
  FlushImmediateVertices();

  if (rasterizerStateGuest == nullptr)
    return;
  const auto *rs =
      reinterpret_cast<const GuestRasterizerState *>(rasterizerStateGuest);
  rr::SetRenderState(device, rr::D3DRS_CULLMODE, rs->cullMode.get());
}

void SetColorWriteEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_COLORWRITEENABLE,
                     value != 0 ? 0xFu : 0u);
}

void SetZWriteEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_ZWRITEENABLE, value);
}

void SetCullMode(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetRenderState(device, rr::D3DRS_CULLMODE, value);
}

void RsClipPlaneEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

void RsViewportEnable(GuestDevice *device, uint32_t value) {
  FlushImmediateVertices();
  rr::SetViewportEnable(device, value);
}

void SetPendingClipPlanes(GuestDevice *device, uint64_t dirtyMask) {
  FlushImmediateVertices();
  rr::UpdateClipPlaneConstants(device);
}

void SurfaceGetDesc(GuestSurface *surface, GuestSurfaceDesc *desc) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origSurfaceGetDesc(surface, desc);
    return;
  }
  if (!rr::IsFm2Resource(surface)) {
    g_origSurfaceGetDesc(surface, desc);
    return;
  }
  if (desc == nullptr)
    return;
  desc->format = surface->guestFormat;
  desc->type = 1; // D3DRTYPE_SURFACE
  desc->usage = 0;
  desc->pool = 0;
  desc->multiSampleType = 0;
  desc->multiSampleQuality = 0;
  desc->width = surface->width;
  desc->height = surface->height;
}

void SetVertexDeclarationBind(GuestDevice *device,
                              GuestVertexDeclaration *decl) {
  FlushImmediateVertices();
  GuestVertexDeclaration *reoDecl = AsFm2(decl);
  if (reoDecl == nullptr)
    reoDecl = rr::LookupVertexDeclarationAlias(ghp::ToGuest(decl));
  device->vertexDeclaration = reoDecl ? ghp::ToGuest(reoDecl) : 0;
  device->dirtyFlags[2] = device->dirtyFlags[2].get() | 0x00080000;
  if (reoDecl != nullptr)
    rr::SetVertexDeclaration(device, reoDecl);
}
void *FXeVertexShaderInit(uint8_t *self, uint32_t *blob) {
  uint32_t d3dShader = *reinterpret_cast<rex::be<uint32_t> *>(self + 8);
  rr::RegisterShaderAlias(d3dShader, rr::CreateVertexShader(blob));
  return self;
}

void *FXePixelShaderInit(uint8_t *self, uint32_t *blob) {
  uint32_t d3dShader = *reinterpret_cast<rex::be<uint32_t> *>(self + 8);
  rr::RegisterShaderAlias(d3dShader, rr::CreatePixelShader(blob));
  return self;
}

void DrawVertices(GuestDevice *device, uint32_t primType, uint32_t startVertex,
                  uint32_t vertexCount) {
  FlushImmediateVertices();
  rr::DrawPrimitive(device, primType, startVertex, vertexCount);
}
void DrawIndexedVertices(GuestDevice *device, uint32_t primType,
                         int32_t baseVertexIndex, uint32_t startIndex,
                         uint32_t indexCount) {
  FlushImmediateVertices();
  g_drawsSinceLastPresent.fetch_add(1, std::memory_order_relaxed);
  rr::DrawIndexedPrimitive(device, primType, baseVertexIndex, startIndex,
                           indexCount);
}
void DrawVerticesUP(GuestDevice *device, uint32_t primType,
                    uint32_t vertexCount, void *data, uint32_t stride) {
  FlushImmediateVertices();
  rr::DrawPrimitiveUP(device, primType, vertexCount, data, stride);
}

void RHIDrawIndexedPrimitiveUP(GuestDevice *device, uint32_t primType,
                               uint32_t minVertexIndex, uint32_t numVertices,
                               uint32_t numPrimitives, const void *indexData,
                               uint32_t indexStride, const void *vertexData,
                               uint32_t vertexStride) {
  FlushImmediateVertices();
  if (device == nullptr || indexData == nullptr || vertexData == nullptr)
    return;
  uint32_t d3dPrimType = rr::D3DPT_TRIANGLELIST;
  switch (primType) {
  case 1:
    d3dPrimType = rr::D3DPT_TRIANGLEFAN;
    break;
  case 2:
    d3dPrimType = rr::D3DPT_TRIANGLESTRIP;
    break;
  case 3:
    d3dPrimType = rr::D3DPT_LINELIST;
    break;
  case 4:
    d3dPrimType = rr::D3DPT_QUADLIST;
    break;
  default:
    break;
  }
  rr::DrawIndexedPrimitiveUP(device, d3dPrimType, minVertexIndex, numVertices,
                             numPrimitives, indexData, indexStride, vertexData,
                             vertexStride);
}

// FM2 keeps the live color-write mask in the render context (== D3D CDevice) at
// offset 10420, bits 14-16 -- guest FM2_RenderContext_SetColorWriteMaskBits packs
// (value << 14) & 0x1C000. The PM4 draw emitters bypass the RenderContext Set*
// hook path, so the native renderer's mirrored colorWriteEnable goes stale
// (observed stuck at 0 -> every draw sanitized to no color attachment ->
// rejected NoAttachments). Read the mask live per draw and push it into the
// native mirror so the bound color RT is actually used.
void ApplyLiveColorWriteFromContext(GuestDevice *device, uint32_t context) {
  if (context == 0)
    return;
  const uint32_t rs = ReadGuestU32At(context + 10420u);
  const uint32_t cw = (rs >> 14) & 0x7u; // 3-bit RGB write-enable field
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 24) {
    LogReplayDbg("FM2_LIVE_COLORWRITE ctx=0x%08X rs=0x%08X cw=0x%X", context, rs,
                 cw);
  }
  // Any nonzero live mask means the game wants color output for this draw; map
  // to a full RGBA write so the color attachment is kept and written.
  rr::SetRenderState(device, rr::D3DRS_COLORWRITEENABLE, cw ? 0xFu : 0u);
}

// Forza keeps the per-sampler bound texture pointers at context+12264+4*slot
// (read by sub_823708D8 as *(ctx + 4*(slot+3066))). The PM4 emit path bypasses
// the standard D3D SetTexture, so g_textures stays empty and the pixel shaders
// sample nothing -> the geometry renders black (confirmed via RenderDoc). Mirror
// ---------------------------------------------------------------------------
// Surface-aperture registry. Forza references textures by raw GPU address; we
// render those surfaces into host plume textures. This maps each surface's
// backing guest address (page-aligned) -> its host texture, so a fetch-constant
// sample of that address binds the real rendered surface instead of reading
// black guest memory. (The native equivalent of UnleashedRecomp's per-object
// host-resource map, keyed by address instead of object pointer.)
// ---------------------------------------------------------------------------
std::mutex g_surfaceApertureMutex;
std::unordered_map<uint32_t, rr::GuestBaseTexture *> g_surfaceAperture;

void RegisterSurfaceAperture(uint32_t guestAddr, rr::GuestBaseTexture *host) {
  // 2026-07-02: upper bound widened 0x1A000000 -> 0x20000000; the swap
  // framebuffer resolve destination lives high (e.g. ~0x1F90F000) and was
  // rejected by the old guard. Also strip 360 physical-alias windows
  // (0xA/0xC/0xE0000000) so header words in fetch form match raw physical.
  guestAddr &= 0x1FFFFFFFu;
  if (host == nullptr || host->texture == nullptr || guestAddr < 0x08000000u ||
      guestAddr >= 0x20000000u)
    return;
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  g_surfaceAperture[guestAddr & ~0xFFFu] = host;
}

rr::GuestBaseTexture *LookupSurfaceAperture(uint32_t guestAddr) {
  std::lock_guard<std::mutex> lk(g_surfaceApertureMutex);
  // 2026-07-02: registry keys are raw-physical (alias windows stripped at
  // registration); strip them on lookup too so 0xA/0xC/0xE0000000-form fetch
  // bases match. Covers all callers (present + texture sampling).
  auto it = g_surfaceAperture.find(guestAddr & 0x1FFFFFFFu & ~0xFFFu);
  return it != g_surfaceAperture.end() ? it->second : nullptr;
}

// Scan a guest surface descriptor for texture-range backing addresses and map
// each to the host surface. We register every candidate (the descriptor holds
// the address in several forms/offsets) -- a later sample of any of them binds
// the rendered surface.
void RegisterSurfaceApertures(uint32_t descriptorAddr,
                              rr::GuestBaseTexture *host) {
  if (host == nullptr || host->texture == nullptr || descriptorAddr == 0)
    return;
  for (uint32_t off = 0; off < 512u; off += 4u) {
    // 2026-07-02: accept physical-alias forms too (0xA/0xC/0xE0000000 windows)
    // and the widened high range; RegisterSurfaceAperture masks + bounds them.
    const uint32_t v = ReadGuestU32At(descriptorAddr + off) & 0x1FFFFFFFu;
    if (v >= 0x08000000u && v < 0x20000000u)
      RegisterSurfaceAperture(v, host); // raw page-aligned == fetch-form base
  }
}

// the live table into the native sampler bindings so textures actually show.
void ApplyLiveTexturesFromContext(GuestDevice *device, uint32_t context) {
  static std::atomic<uint32_t> s_n{0};
  const bool trace = s_n.fetch_add(1, std::memory_order_relaxed) < 24;

  // Forza binds textures via FM2_D3D_ApplyGpuMemoryPatches, which writes the
  // canonical 6-dword GPU texture fetch constant into the engine GPU block at
  // block + 1024 + slot*24 (and the descriptor pointer into block+12264+slot*4,
  // which we found empty for these draws). The block base is the value passed
  // to the draw emitter (== *dword_82A41BEC). The native draw bypasses the PM4
  // ring, so we decode the live fetch constants here and mirror them into the
  // native sampler bindings. Each slot is a standard xe_gpu_texture_fetch_t.
  const uint32_t block = context != 0 ? context : ReadGuestU32At(0x82A41BECu);
  if (block == 0)
    return;

  // Bind textures the engine resolved into the fetch-constant array. The upload
  // path is now bounds-checked (skips unmapped guest memory instead of
  // faulting), so we can translate+upload directly. TranslateGuestTextureFetch
  // checks the alias cache first, so repeated draws of the same texture reuse
  // the created resource.
  for (uint32_t slot = 0; slot < 16u; ++slot) {
    const uint32_t fcAddr = block + 1024u + slot * 24u;
    const uint32_t fc0 = ReadGuestU32At(fcAddr);
    const uint32_t fc1 = ReadGuestU32At(fcAddr + 4u);
    if ((fc0 & 0x3u) != 2u && fc1 == 0u)
      continue;
    const uint32_t base = ((fc1 >> 12) & 0xFFFFFu) << 12;
    // Prefer a registered host surface (the rendered/resolved plume texture)
    // over uploading from black guest memory.
    rr::GuestBaseTexture *surf = LookupSurfaceAperture(base);
    if (surf != nullptr) {
      rr::SetTextureBase(device, slot, surf);
      rr::SetTestGameTexture(surf);
      // NOTE: do NOT record aperture/snapshot surfaces into the VRAM viewer --
      // SnapshotSurfaceForResolve recreates them on format change, so the viewer's
      // stored pointer dangles -> blit crash. Only the persistent upload-path
      // textures (cached in g_guestTextureAliases) are recorded (below).
      if (trace)
        LogReplayDbg("FM2_LIVE_TEX slot=%u base=0x%08X -> APERTURE host=%p",
                     slot, base, static_cast<void *>(surf));
      continue;
    }
    rr::GuestTexture *tex =
        rr::TranslateGuestTextureFetch(ghp::ToHost<void>(fcAddr), true);
    if (trace) {
      LogReplayDbg("FM2_LIVE_TEX slot=%u fc0=%08X fc1=%08X base=0x%08X tex=%p",
                   slot, fc0, fc1, base, static_cast<void *>(tex));
    }
    if (tex != nullptr) {
      rr::SetTexture(device, slot, tex);
      rr::SetTestGameTexture(tex); // diagnostic: expose to the present test grid
      rr::RecordVramViewTexture(base, tex);
    }
  }
}

// PM4 indexed draw emitters → replaced with native Plume draw calls.
// Renamed 2026-07-01 (docs/FM2-ida-renames-2026-07-01.md): these are inlined
// copies of the XDK D3DDevice_Draw* primitives, not FM2-specific emitters.
// Signatures corrected 2026-07-01 (decompile evidence: the dword after the
// PM4 0x2102/VGT_INDX_OFFSET header is StartVertex / BaseVertexIndex):
// D3DDevice_DrawVertices:                              (device, primType, startVertex, vertexCount) [NON-indexed]
// D3DDevice_DrawIndexedVertices:                       (device, primType, baseVertexIndex, startIndex, indexCount)
// D3DDevice_DrawIndexedVertices_WithVertexFormatSetup: (device, primType, baseVertexIndex, startIndex, indexCount)
// Bind the PM4 draw's REAL geometry directly from the render context. In
// plume_native the D3D9 SetStreamSource/SetIndices hooks receive NULL -- the
// geometry comes through the PM4 stream, with the bound D3DResources stored in the
// context (FM2_RenderContext_BindVertexStream / BindIndexBuffer):
//   index resource    @ ctx+0x2F7C
//   vtx stream s res   @ ctx+0x2F94 + 4*s ; stride/4 byte @ ctx+0x2FD8 + s
//   in each D3DResource: GPU phys base @ +0x18, size @ +0x1C, common(format) @ +0.
// Bases are guest PHYSICAL addresses (read via TranslatePhysical).
// DIAGNOSTIC: the per-object world-view matrix (VS ALU consts, regs c7..c18) is
// emitted as PM4 SET_CONSTANT into the FM2 command buffer (ctx+0x30 = write ptr,
// a GPU write-combined physical-alias addr) which plume_native's disabled CP
// never processes -> those regs read zero -> car geometry collapses. Scan the
// command-buffer delta since the last draw for PM4_SET_CONSTANT(type=0 ALU)
// packets and log them; idx is the ALU register (const = idx/4). Confirm the
// WVP (idx ~28..75) is here before feeding g_passVsConstants.
void ProcessPm4VsConstantsDiag(uint32_t context) {
  auto *mem = ghp::GuestMemory();
  if (mem == nullptr) return;
  auto rd = [&](uint32_t ga) -> uint32_t {
    auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
    return p ? p->get() : 0u;
  };
  const uint32_t curPhys = rd(context + 0x30u) & 0x1FFFFFFFu;
  static uint32_t s_lastPhys = 0;
  const uint32_t lastPhys = s_lastPhys;
  s_lastPhys = curPhys;
  if (lastPhys == 0 || curPhys <= lastPhys) return; // first call / wrap / drain
  const uint32_t len = curPhys - lastPhys;
  if (len > 0x40000u) return;
  const auto *buf = mem->TranslatePhysical<const uint8_t *>(lastPhys);
  if (buf == nullptr) return;
  auto be = [&](uint32_t o) -> uint32_t {
    const uint8_t *p = buf + o;
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
  };
  uint32_t off = 0, guard = 0;
  while (off + 4u <= len && guard++ < 8192u) {
    const uint32_t hdr = be(off);
    const uint32_t type = hdr >> 30;
    uint32_t adv;
    if (type == 3u) {
      const uint32_t cnt = ((hdr >> 16) & 0x3FFFu) + 1u; // data dwords
      const uint32_t op = (hdr >> 8) & 0x7Fu;
      adv = 1u + cnt;
      if (op == 0x2Du && off + 8u <= len) { // PM4_SET_CONSTANT
        const uint32_t ot = be(off + 4u);
        const uint32_t idx = ot & 0x7FFu;
        const uint32_t ctype = (ot >> 16) & 0xFFu;
        // Log each DISTINCT (type,idx) once -> full histogram of which ALU regs
        // the game SET_CONSTANTs. WVP for the VS is regs ~28..75 (c7..c18).
        static bool s_seen[6][2048] = {};
        const uint32_t tt = ctype < 6u ? ctype : 5u;
        if (idx < 2048u && !s_seen[tt][idx]) {
          s_seen[tt][idx] = true;
          LogReplayDbg("FM2_PM4SETC type=%u idx=%u (c%u.%u) regs=%u v=%08X %08X "
                       "%08X %08X",
                       ctype, idx, idx / 4u, idx % 4u, cnt - 1u,
                       off + 8u <= len ? be(off + 8u) : 0u,
                       off + 12u <= len ? be(off + 12u) : 0u,
                       off + 16u <= len ? be(off + 16u) : 0u,
                       off + 20u <= len ? be(off + 20u) : 0u);
        }
      } else if (op == 0x2Fu && off + 16u <= len) { // PM4_LOAD_ALU_CONSTANT
        const uint32_t addr = be(off + 4u) & 0x3FFFFFFFu;
        const uint32_t ot = be(off + 8u);
        const uint32_t idx = ot & 0x7FFu;
        const uint32_t ctype = (ot >> 16) & 0xFFu;
        const uint32_t sz = be(off + 12u) & 0xFFFu;
        static bool s_seenL[2048] = {};
        if (ctype == 0u && idx < 2048u && !s_seenL[idx]) {
          s_seenL[idx] = true;
          const auto *src =
              mem->TranslatePhysical<const uint8_t *>(addr & 0x1FFFFFFFu);
          auto sbe = [&](uint32_t o) -> uint32_t {
            if (!src) return 0u;
            const uint8_t *p = src + o;
            return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
                   (uint32_t(p[2]) << 8) | uint32_t(p[3]);
          };
          LogReplayDbg("FM2_PM4LOADALU idx=%u (c%u) sz=%u addr=%08X v=%08X %08X "
                       "%08X %08X",
                       idx, idx / 4u, sz, addr, sbe(0), sbe(4), sbe(8), sbe(12));
        }
      }
    } else if (type == 0u) {
      const uint32_t cnt0 = ((hdr >> 16) & 0x3FFFu) + 1u;
      const uint32_t base = hdr & 0x7FFFu;
      if (base >= 0x4000u && base < 0x4400u) {
        static bool s_seen0[2048] = {};
        const uint32_t r = base - 0x4000u;
        if (r < 2048u && !s_seen0[r]) {
          s_seen0[r] = true;
          LogReplayDbg("FM2_PM4T0ALU reg=%u (c%u.%u) count=%u v=%08X", r, r / 4u,
                       r % 4u, cnt0, off + 4u <= len ? be(off + 4u) : 0u);
        }
      }
      adv = 1u + cnt0;
    } else if (type == 1u) {
      adv = 3u;
    } else {
      adv = 1u; // type 2 NOP filler
    }
    off += 4u * adv;
  }
}

void BindPm4GeometryFromContext(GuestDevice *device, uint32_t context,
                                uint32_t startIndex, uint32_t indexCount) {
  if (context == 0) return;
  ProcessPm4VsConstantsDiag(context);
  auto *mem = ghp::GuestMemory();
  if (mem == nullptr) return;
  auto rd = [&](uint32_t ga) -> uint32_t {
    auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
    return p ? p->get() : 0u;
  };
  auto physReadable = [&](uint32_t physBase, uint32_t size) -> bool {
    return size != 0 && size <= 0x4000000u && (physBase + size) <= 0x20000000u &&
           mem->GetPhysicalHeap()->QueryRangeAccess(physBase, physBase + size - 1) !=
               rex::memory::PageAccess::kNoAccess;
  };
  // Decode a Xenos vertex/index FETCH-constant size word (D3DResource+0x1C):
  // bits[1:0]=endian, bits[25:2]=size-in-dwords, high bits=format/clamp flags.
  // (Confirmed against FM2_RenderContext_BindVertexStream @0x82370E48 and the
  //  index emitter @0x827317A0.) Reading it raw yields a bogus ~256MB size for
  //  any resource that has flag bits set, which physReadable then rejects.
  auto decodeFetchSize = [](uint32_t w) -> uint32_t {
    return ((w >> 2) & 0xFFFFFFu) * 4u;
  };
  // Index buffer.
  const uint32_t idxRes = rd(context + 0x2F7Cu);
  bool idxBound = false;
  if (idxRes != 0) {
    const uint32_t common = rd(idxRes);
    // Index base keeps its low 2 bits: the emitter forms the read address as
    // (2*startIndex + base) & 0x1FFFFFFF without clearing them.
    const uint32_t physBase = rd(idxRes + 0x18u) & 0x1FFFFFFFu;
    const uint32_t size = decodeFetchSize(rd(idxRes + 0x1Cu));
    // 32-bit indices iff the common dword's sign bit is set (IDA: `*v13 < 0`).
    const uint32_t stride = (common & 0x80000000u) ? 4u : 2u;
    if (physReadable(physBase, size)) {
      const void *host = mem->TranslatePhysical<const void *>(physBase);
      if (host) {
        rr::SetIndicesGuestData(device, host, size, stride);
        idxBound = true;
      }
    }
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 16)
      LogReplayDbg("FM2_PM4GEO_IDX ctx=0x%08X idxRes=0x%08X common=0x%08X "
                   "physBase=0x%08X size=%u stride=%u readable=%d bound=%d",
                   context, idxRes, common, physBase, size, stride,
                   physReadable(physBase, size) ? 1 : 0, idxBound ? 1 : 0);
  } else {
    static std::atomic<uint32_t> s_n0{0};
    if (s_n0.fetch_add(1, std::memory_order_relaxed) < 8)
      LogReplayDbg("FM2_PM4GEO_IDX ctx=0x%08X idxRes=0 (no index resource)",
                   context);
  }
  // Vertex streams.
  for (uint32_t s = 0; s < 8u; ++s) {
    const uint32_t vbRes = rd(context + 0x2F94u + 4u * s);
    if (vbRes == 0) continue;
    // Vertex base clears the 2 endian bits (BindVertexStream uses addr & ~3;
    // the low bits select the GPU fetch endian swap, not part of the address).
    const uint32_t physBase = rd(vbRes + 0x18u) & 0x1FFFFFFCu;
    const uint32_t size = decodeFetchSize(rd(vbRes + 0x1Cu));
    auto *sb = ghp::ToHost<uint8_t>(context + 0x2FD8u + s);
    const uint32_t stride = (sb ? *sb : 0u) * 4u;
    bool vbBound = false;
    if (stride != 0 && physReadable(physBase, size)) {
      const void *host = mem->TranslatePhysical<const void *>(physBase);
      if (host) {
        rr::SetStreamSourceGuestData(device, s, host, size, stride);
        vbBound = true;
      }
    }
    static std::atomic<uint32_t> s_nv{0};
    if (s == 0 && s_nv.fetch_add(1, std::memory_order_relaxed) < 10) {
      // Dump the raw resource dwords + the fetch constants BindVertexStream also
      // writes (base @ ctx+0x6F8, size @ ctx+0x6FC for stream 0), to decode the
      // real base/size offsets in the D3DResource.
      const uint32_t fcBase = rd(context + 0x6F8u);
      const uint32_t fcSize = rd(context + 0x6FCu);
      LogReplayDbg("FM2_PM4GEO_VTX s=0 vbRes=0x%08X res[0..9]="
                   "%08X %08X %08X %08X %08X %08X %08X %08X %08X %08X | "
                   "fetchBase=0x%08X fetchSize=%u stride=%u | decBase=0x%08X "
                   "decSize=%u bound=%d",
                   vbRes, rd(vbRes), rd(vbRes + 4), rd(vbRes + 8), rd(vbRes + 12),
                   rd(vbRes + 16), rd(vbRes + 20), rd(vbRes + 24), rd(vbRes + 28),
                   rd(vbRes + 32), rd(vbRes + 36), fcBase, fcSize, stride,
                   physBase, size, vbBound ? 1 : 0);
    }
  }
  // VERBOSE for scene-sized draws: dump EVERY bound stream (slots 0..15; the
  // D3D12 debug layer reported the input layout referencing slot 15), so we can
  // tell whether the big indexed draws actually get a large vertex buffer or
  // just the 24-vert UI stream that the first draws bind.
  static std::atomic<uint32_t> s_big{0};
  if (indexCount >= 600u && s_big.fetch_add(1, std::memory_order_relaxed) < 24) {
    char buf[640];
    int off = 0;
    for (uint32_t s = 0; s < 16u; ++s) {
      const uint32_t vbRes = rd(context + 0x2F94u + 4u * s);
      if (vbRes == 0) continue;
      const uint32_t base = rd(vbRes + 0x18u) & 0x1FFFFFFCu;
      const uint32_t size = decodeFetchSize(rd(vbRes + 0x1Cu));
      auto *sb = ghp::ToHost<uint8_t>(context + 0x2FD8u + s);
      const uint32_t stride = (sb ? *sb : 0u) * 4u;
      const uint32_t nverts = stride ? size / stride : 0u;
      // Raw guest source bytes at the decoded phys base (PRE-byteswap). If this
      // is uniform fill (e.g. FF000000) the geometry is not actually there.
      const auto *vsrc = mem->TranslatePhysical<const uint8_t *>(base);
      uint32_t vs0 = 0, vs1 = 0;
      if (vsrc) { std::memcpy(&vs0, vsrc, 4); std::memcpy(&vs1, vsrc + 4, 4); }
      off += std::snprintf(buf + off, sizeof(buf) - off,
                           " s%u[res=%08X base=%08X sz=%u str=%u nv=%u src=%08X%08X]",
                           s, vbRes, base, size, stride, nverts, vs0, vs1);
      if (off >= (int)sizeof(buf) - 80) break;
    }
    const uint32_t idxPhys = idxRes ? (rd(idxRes + 0x18u) & 0x1FFFFFFFu) : 0u;
    const auto *isrc =
        idxPhys ? mem->TranslatePhysical<const uint8_t *>(idxPhys) : nullptr;
    uint32_t is0 = 0, is1 = 0;
    if (isrc) { std::memcpy(&is0, isrc, 4); std::memcpy(&is1, isrc + 4, 4); }
    LogReplayDbg("FM2_PM4GEO_BIG idxCnt=%u start=%u idxRes=0x%08X idxPhys=%08X "
                 "idxSz=%u idxSrc=%08X%08X |%s",
                 indexCount, startIndex, idxRes, idxPhys,
                 idxRes ? decodeFetchSize(rd(idxRes + 0x1Cu)) : 0u, is0, is1, buf);
  }
}

// TEMP DIAGNOSTIC 2026-07-02: g_FM2_ActivePassRenderContext_ (0x82A41BEC) is
// per-pass and switches; if UI passes draw with a DIFFERENT context object,
// our live-constant read (ctx+0x700) targets the wrong object for them.
// Log the first few draws whose context differs from the first one seen.
void NoteDrawContext(uint32_t context, uint32_t primType) {
  static std::atomic<uint32_t> s_firstCtx{0};
  uint32_t expected = 0;
  if (!s_firstCtx.compare_exchange_strong(expected, context,
                                          std::memory_order_relaxed)) {
    if (expected != context) {
      static std::atomic<uint32_t> s_n{0};
      if (s_n.fetch_add(1, std::memory_order_relaxed) < 16)
        LogReplayDbg("FM2_PM4_CTX_ALT ctx=0x%08X first=0x%08X prim=%u", context,
                     expected, primType);
    }
  }
}

// CORRECTED 2026-07-01: this hooks D3DDevice_DrawVertices, the NON-indexed
// XDK draw primitive -- real args are (device, primType, StartVertex,
// VertexCount), confirmed by decompile diff vs Lost Odyssey (see
// docs/FM2-ida-renames-2026-07-01.md; the PM4 packet writes StartVertex to
// VGT_INDX_OFFSET and tags the initiator |0x80 = auto-index). The old handler
// body predated that identification (the function was misnamed
// "EmitIndexedDrawPm4Packets") and submitted these as INDEXED draws with
// whatever stale index buffer was bound. Now matches ReOdyssey: DrawVertices
// -> rr::DrawPrimitive. BindPm4GeometryFromContext still runs to bind the PM4
// vertex streams (its index-buffer bind is unused by a non-indexed draw).
void Fm2EmitIndexedDrawPm4Base(uint32_t context, uint32_t primType,
                               uint32_t startVertex,
                               uint32_t vertexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4Packets(context, primType, startVertex,
                                       vertexCount);
    return;
  }
  static PresentDiagSlot s_drawEmitSlot;
  CountPresentDiag("DrawEmit", s_drawEmitSlot, nullptr);
  NoteDrawContext(context, primType);
  GuestDevice *device = rr::GetActiveGuestDevice();
  static DrawDeviceSlot s_drawEmitDevSlot;
  CountDrawDevice("DrawEmit", s_drawEmitDevSlot, device != nullptr);
  if (device == nullptr) return;
  ApplyLiveColorWriteFromContext(device, context);
  ApplyLiveTexturesFromContext(device, context);
  BindPm4GeometryFromContext(device, context, 0, vertexCount);
  // Source VS/PS float constants from the ISSUING context's live ALU blocks
  // at ctx+0x700/+0x1700. IMPORTANT (2026-07-02, hard-won): our XenosRecomp-
  // translated shaders index their constant buffer by the game shader
  // container's register table, which is aligned to the 0x700-based block
  // (register 0 = the block's first vec4). Xenia's files LOOK shifted by one
  // relative to this because its ucode translation uses a different internal
  // alignment -- do NOT "fix" this to 0x710 (tried; semantic check of
  // offsetScale/Proj/horizon table slots + an all-black screen proved the
  // 0x700 base is what OUR shaders expect). Reading
  // g_FM2_ActivePassRenderContext_ instead was also tried and went all-black
  // (the global tracks the pass being BUILT on another thread).
  rr::SetLiveFloatConstantFiles(ghp::ToHost<const void>(context + 0x700u),
                                ghp::ToHost<const void>(context + 0x1700u));
  DrawVertices(device, primType, startVertex, vertexCount);
  rr::SetLiveFloatConstantFiles(nullptr, nullptr);
  rr::SetScenePresentRT(rr::GetCurrentColorRenderTarget());
  g_sceneDrawsThisCL.fetch_add(1, std::memory_order_relaxed);
}

// Aliased creation hooks: call the original XDK function (FM2 reads the result
// as a genuine D3D resource), then create a shadow GuestBuffer backed by Plume
// and register it in the alias map so Lock/Bind can use the native resource.
uint32_t CreateVertexBufferAliased(uint32_t length) {
  uint32_t xdkHandle = g_origCreateVertexBuffer(length);
  if (!ShouldMirrorPlumeRenderState()) {
    return xdkHandle;
  }
  if (!xdkHandle) return 0;
  GuestBuffer *native = rr::CreateVertexBuffer(length);
  if (native)
    rr::RegisterBufferAlias(xdkHandle, native);
  return xdkHandle;
}

// CORRECTED 2026-07-01: arg3 of the XDK D3DDevice_DrawIndexedVertices is the
// BASE VERTEX INDEX (the original writes it verbatim into the PM4 TYPE-0
// packet for Xenos register 0x2102 = VGT_INDX_OFFSET; ReOdyssey's hook on the
// same XDK function names it baseVertexIndex and passes it through). The old
// code called it "gpuOffset" and dropped it (passed 0), so every indexed draw
// from a shared vertex pool fetched from the pool's start instead of the
// draw's sub-range.
void SubmitNativeIndexedDrawPm4(uint32_t context, uint32_t primType,
                                uint32_t baseVertexIndex, uint32_t startIndex,
                                uint32_t indexCount) {
  static PresentDiagSlot s_drawSubmitSlot;
  CountPresentDiag("DrawSubmit", s_drawSubmitSlot, nullptr);
  NoteDrawContext(context, primType);
  GuestDevice *device = rr::GetActiveGuestDevice();
  static DrawDeviceSlot s_drawSubmitDevSlot;
  CountDrawDevice("DrawSubmit", s_drawSubmitDevSlot, device != nullptr);
  if (device == nullptr) return;
  ApplyLiveColorWriteFromContext(device, context);
  ApplyLiveTexturesFromContext(device, context);
  // STAGE 1: what the game asked to draw (the intent), before we bind/submit.
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 24)
      LogReplayDbg("FM2_PM4_DRAW ctx=0x%08X prim=%u baseVertex=%u startIndex=%u "
                   "indexCount=%u",
                   context, primType, baseVertexIndex, startIndex, indexCount);
  }
  BindPm4GeometryFromContext(device, context, startIndex, indexCount);
  // TEMP DIAGNOSTIC 2026-07-02: verify the context register file holds live
  // data at both low (transform) and high (material/global) registers.
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 8) {
      auto rf = [&](uint32_t reg, uint32_t comp) -> double {
        const uint32_t v =
            ReadGuestU32At(context + 0x710u + reg * 16u + comp * 4u);
        return double(std::bit_cast<float>(v));
      };
      LogReplayDbg("FM2_LIVECONST ctx=0x%08X dev=0x%08X c0=[%.3f %.3f %.3f "
                   "%.3f] c9=[%.3f %.3f %.3f %.3f] c250=[%.3f %.3f %.3f %.3f] "
                   "c253=[%.3f %.3f %.3f %.3f]",
                   context, ghp::ToGuest(device), rf(0, 0), rf(0, 1), rf(0, 2),
                   rf(0, 3), rf(9, 0), rf(9, 1), rf(9, 2), rf(9, 3), rf(250, 0),
                   rf(250, 1), rf(250, 2), rf(250, 3), rf(253, 0), rf(253, 1),
                   rf(253, 2), rf(253, 3));
    }
  }
  // Source VS/PS float constants from the ISSUING context's live ALU blocks
  // at +0x700/+0x1700 (see the DrawVertices hook comment for why NOT 0x710).
  rr::SetLiveFloatConstantFiles(ghp::ToHost<const void>(context + 0x700u),
                                ghp::ToHost<const void>(context + 0x1700u));
  DrawIndexedVertices(device, primType, int32_t(baseVertexIndex), startIndex,
                      indexCount);
  rr::SetLiveFloatConstantFiles(nullptr, nullptr);
  // B: present the SCENE RT (the RT these PM4 3D draws render to), not the
  // last-touched UI RT. A: count scene draws to see if they reach the submit.
  rr::SetScenePresentRT(rr::GetCurrentColorRenderTarget());
  g_sceneDrawsThisCL.fetch_add(1, std::memory_order_relaxed);
}

// Decode the current native state snapshot + PM4 draw args and submit a debug
// replay draw to the side-by-side replay window (shadow mode only).
// Mirrors the UnleashedRecomp/ReOdyssey pattern: hook the draw call and read
// prior stream/index state from the recorder populated by midasm hooks.
static void TryBuildAndSubmitDebugReplayForPm4Draw(uint32_t context,
                                                   uint32_t primType,
                                                   uint32_t gpuOffset,
                                                   uint32_t startIndex,
                                                   uint32_t indexCount) {
  (void)context;
  static std::atomic<uint32_t> s_dbg_calls{0};
  const uint32_t call_n = s_dbg_calls.fetch_add(1, std::memory_order_relaxed) + 1;

  if (!nr::WantsDirectDebugReplay() || indexCount == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP wants=%d idxcnt=%u",
                   call_n, (int)nr::WantsDirectDebugReplay(), indexCount);
    }
    return;
  }

  const nr::NativeStateSnapshot snapshot = nr::SnapshotLastNativeState();
  if (!snapshot.valid) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP snapshot_invalid", call_n);
    }
    return;
  }

  // Locate slot 0 and slot 1 from the snapshot (populated by midasm hooks).
  const nr::NativeStateVertexStreamBinding *s0 = nullptr;
  const nr::NativeStateVertexStreamBinding *s1 = nullptr;
  for (const auto &s : snapshot.streams) {
    if (!s.valid) continue;
    if (s.slot == 0u && !s0) s0 = &s;
    else if (s.slot == 1u && !s1) s1 = &s;
  }
  if (!s0 || s0->resource == 0 || s0->stride_bytes == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP no_s0 s0=%p res=%08X stride=%u",
                   call_n, (void*)s0,
                   s0 ? s0->resource : 0u, s0 ? s0->stride_bytes : 0u);
    }
    return;
  }
  if (!snapshot.index_buffer.valid || snapshot.index_buffer.resource == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP no_ib valid=%d res=%08X",
                   call_n, (int)snapshot.index_buffer.valid,
                   snapshot.index_buffer.resource);
    }
    return;
  }

  // Decode stream-0 vertex buffer header (XDK vertex fetch constant layout).
  const auto *s0hdr =
      ghp::ToHost<const GuestD3DVertexBufferHeader>(s0->resource);
  if (!s0hdr || (s0hdr->common.get() & 0xFu) != 1u) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP s0hdr_type hdr=%p common=%08X",
                   call_n, (void*)s0hdr,
                   s0hdr ? s0hdr->common.get() : 0u);
    }
    return;
  }
  const uint32_t s0_gpu_base = s0hdr->format0.get() & ~3u;
  const uint32_t s0_raw_size = s0hdr->format1.get() & 0x3FFFFFCu;
  const uint32_t s0_readable = s0_raw_size & nr::kD3DResourceByteSizeMask;
  if (s0_gpu_base == 0 || s0_readable == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP s0_empty base=%08X readable=%u",
                   call_n, s0_gpu_base, s0_readable);
    }
    return;
  }
  const uint32_t s0_stride = s0->stride_bytes;
  const uint32_t s0_upload_base = nr::DirectDrawReplayUploadGuestBase(s0_gpu_base);
  const uint32_t s0_vcount = s0_readable / s0_stride;
  const uint32_t s0_view_bytes = s0_vcount * s0_stride;
  const uint32_t s0_hash_bytes =
      (s0_view_bytes != 0 && s0_view_bytes < s0_readable) ? s0_view_bytes
                                                           : s0_readable;

  // Decode stream-1 (optional — plan.ready will be false if absent).
  uint32_t s1_gpu_base = 0, s1_raw_size = 0, s1_stride = 0;
  uint32_t s1_upload_base = 0, s1_vcount = 0, s1_view_bytes = 0, s1_hash_bytes = 0;
  bool s1_valid = false;
  if (s1 && s1->resource != 0 && s1->stride_bytes != 0) {
    const auto *s1hdr =
        ghp::ToHost<const GuestD3DVertexBufferHeader>(s1->resource);
    if (s1hdr && (s1hdr->common.get() & 0xFu) == 1u) {
      const uint32_t base = s1hdr->format0.get() & ~3u;
      const uint32_t raw  = s1hdr->format1.get() & 0x3FFFFFCu;
      const uint32_t readable = raw & nr::kD3DResourceByteSizeMask;
      if (base != 0 && readable != 0) {
        s1_gpu_base    = base;
        s1_raw_size    = raw;
        s1_stride      = s1->stride_bytes;
        s1_upload_base = nr::DirectDrawReplayUploadGuestBase(s1_gpu_base);
        s1_vcount      = readable / s1_stride;
        s1_view_bytes  = s1_vcount * s1_stride;
        s1_hash_bytes  = (s1_view_bytes != 0 && s1_view_bytes < readable)
                             ? s1_view_bytes : readable;
        s1_valid = true;
      }
    }
  }

  // When stream1 is absent, synthesize a stride-12 position-only view from
  // stream0 (first 12 bytes = float3 position of each vertex). Both debug
  // replay pipeline layouts (kDebugRaw32Side12, kNativePosition28Side12)
  // require stream1 stride=12.
  std::vector<uint8_t> s1_synth_bytes;
  if (!s1_valid && s0_stride >= 12u && s0_vcount > 0) {
    const uint8_t* s0_ptr = ghp::ToHost<const uint8_t>(s0_upload_base);
    if (s0_ptr) {
      s1_synth_bytes.resize(s0_vcount * 12u);
      for (uint32_t vi = 0; vi < s0_vcount; ++vi) {
        std::memcpy(&s1_synth_bytes[vi * 12u], s0_ptr + vi * s0_stride, 12u);
      }
      s1_gpu_base    = s0_gpu_base;
      s1_raw_size    = s0_raw_size;
      s1_stride      = 12u;
      s1_upload_base = s0_upload_base;
      s1_vcount      = s0_vcount;
      s1_view_bytes  = s0_vcount * 12u;
      s1_hash_bytes  = s1_view_bytes;
      s1_valid       = true;
    }
  }

  // Decode index buffer header.
  const auto *ihdr =
      ghp::ToHost<const GuestD3DIndexBufferHeader>(snapshot.index_buffer.resource);
  if (!ihdr || (ihdr->common.get() & 0xFu) != 2u) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP ihdr_type hdr=%p common=%08X",
                   call_n, (void*)ihdr,
                   ihdr ? ihdr->common.get() : 0u);
    }
    return;
  }
  // D3DFORMAT in top 3 bits of common: 1=INDEX16, 6=INDEX32.
  const uint32_t d3d_idx_fmt = (ihdr->common.get() >> 29) & 0x7u;
  const uint32_t idx_desc_fmt = (d3d_idx_fmt == 6u) ? 2u : 1u;
  const uint32_t idx_elem_bytes = nr::DirectDrawIndexElementByteCount(idx_desc_fmt);
  if (idx_elem_bytes == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP idx_elem_zero d3dfmt=%u",
                   call_n, d3d_idx_fmt);
    }
    return;
  }
  const uint32_t idx_gpu_base  = ihdr->address.get();
  const uint32_t idx_raw_size  = ihdr->size.get();
  const uint32_t idx_readable  = idx_raw_size & nr::kD3DResourceByteSizeMask;
  if (idx_gpu_base == 0 || idx_readable == 0) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP idx_empty base=%08X readable=%u",
                   call_n, idx_gpu_base, idx_readable);
    }
    return;
  }
  const uint32_t idx_upload_base = nr::DirectDrawReplayUploadGuestBase(idx_gpu_base);
  const uint32_t idx_view_bytes  = indexCount * idx_elem_bytes;
  const uint32_t idx_hash_bytes  =
      (idx_view_bytes != 0 && idx_view_bytes < idx_readable) ? idx_view_bytes
                                                              : idx_readable;

  // Hash each buffer range (guest memory; BigEndian bytes, swapped on upload).
  const auto hash_range = [](uint32_t upload_base, uint32_t byte_count,
                              uint64_t &out_hash) -> bool {
    if (upload_base == 0 || byte_count == 0) return false;
    if (!IsReadableGuestRange(upload_base, byte_count)) return false;
    const auto *ptr = ghp::ToHost<const uint8_t>(upload_base);
    if (!ptr) return false;
    out_hash = XXH3_64bits(ptr, byte_count);
    return true;
  };

  uint64_t s0_hash = 0, s1_hash = 0, idx_hash = 0;
  const bool s0_hash_ok = hash_range(s0_upload_base, s0_hash_bytes, s0_hash);
  bool s1_hash_ok = false;
  if (!s1_synth_bytes.empty()) {
    s1_hash    = XXH3_64bits(s1_synth_bytes.data(), s1_synth_bytes.size());
    s1_hash_ok = true;
  } else {
    s1_hash_ok = s1_valid && hash_range(s1_upload_base, s1_hash_bytes, s1_hash);
  }
  const bool idx_hash_ok = hash_range(idx_upload_base, idx_hash_bytes, idx_hash);

  // Build per-buffer view summaries.
  const nr::DirectDrawBufferViewSummary sv0 = nr::BuildDirectDrawBufferViewSummary(
      s0_gpu_base, s0_raw_size, s0_vcount, s0_stride, false, s0_hash_ok, s0_hash);
  const nr::DirectDrawBufferViewSummary sv1 =
      s1_valid ? nr::BuildDirectDrawBufferViewSummary(s1_gpu_base, s1_raw_size,
                                                      s1_vcount, s1_stride, false,
                                                      s1_hash_ok, s1_hash)
               : nr::BuildDirectDrawBufferViewSummary(0u, 0u, 0u, 0u, false, false, 0u);
  const nr::DirectDrawBufferViewSummary svi = nr::BuildDirectDrawBufferViewSummary(
      idx_gpu_base, idx_raw_size, indexCount, idx_desc_fmt, true,
      idx_hash_ok, idx_hash);

  const nr::DirectDrawReplayTopology topology =
      nr::DirectDrawReplayTopologyFromDirectIfacePrimitiveType(primType);
  const nr::DirectDrawShaderKeySummary empty_shader{};
  const nr::DirectDrawIndexedPacketSummary packet =
      nr::BuildDirectDrawIndexedPacketSummary(0u, topology, startIndex,
                                              indexCount, sv0, sv1, svi,
                                              empty_shader, empty_shader);

  nr::DirectDrawDebugReplayPlan plan =
      nr::BuildDirectDrawDebugReplayPlan(packet, snapshot);
  if (!plan.ready) {
    if (call_n <= 4) {
      LogReplayDbg("FM2_REPLAY_DBG n=%u SKIP plan_not_ready prim=%u topo=%d "
                   "idx=%u s0base=%08X s0stride=%u s1valid=%d s1base=%08X "
                   "s1stride=%u idxbase=%08X s0hash_ok=%d idxhash_ok=%d",
                   call_n, primType, (int)topology,
                   indexCount, s0_gpu_base, s0_stride,
                   (int)s1_valid, s1_gpu_base, s1_stride,
                   idx_gpu_base, (int)s0_hash_ok, (int)idx_hash_ok);
    }
    return;
  }
  if (call_n <= 4) {
    LogReplayDbg("FM2_REPLAY_DBG n=%u SUBMIT prim=%u idx=%u s0base=%08X "
                 "idxbase=%08X s0hash_ok=%d idxhash_ok=%d",
                 call_n, primType, indexCount,
                 s0_gpu_base, idx_gpu_base,
                 (int)s0_hash_ok, (int)idx_hash_ok);
  }

  plan.transform = nr::BuildLastVSDebugReplayTransform();

  const nr::DirectDrawReplaySourceBytes sources{
      .stream0 = ghp::ToHost<const uint8_t>(s0_upload_base),
      .stream1 = !s1_synth_bytes.empty()
                     ? s1_synth_bytes.data()
                     : (s1_valid ? ghp::ToHost<const uint8_t>(s1_upload_base) : nullptr),
      .index   = ghp::ToHost<const uint8_t>(idx_upload_base),
  };
  nr::QueueDirectDebugReplay(plan, sources);
}

void Fm2EmitIndexedDrawPm4WithGpuOffset(uint32_t context, uint32_t primType,
                                        uint32_t baseVertexIndex,
                                        uint32_t startIndex,
                                        uint32_t indexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4PacketsWithGpuOffset(
        context, primType, baseVertexIndex, startIndex, indexCount);
    TryBuildAndSubmitDebugReplayForPm4Draw(context, primType, baseVertexIndex,
                                          startIndex, indexCount);
    return;
  }
  SubmitNativeIndexedDrawPm4(context, primType, baseVertexIndex, startIndex,
                             indexCount);
}

void Fm2EmitIndexedDrawPm4WithVertexFormatSetup(uint32_t context,
                                                uint32_t primType,
                                                uint32_t baseVertexIndex,
                                                uint32_t startIndex,
                                                uint32_t indexCount) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2EmitIndexedDrawPm4WithVertexFormatSetup(
        context, primType, baseVertexIndex, startIndex, indexCount);
    TryBuildAndSubmitDebugReplayForPm4Draw(context, primType, baseVertexIndex,
                                          startIndex, indexCount);
    return;
  }
  SubmitNativeIndexedDrawPm4(context, primType, baseVertexIndex, startIndex,
                             indexCount);
}

// Forward/opaque object-pass draw emitter. DIAGNOSTIC: call the original (keeps
// the game's state machine consistent -- it already runs safely in plume_native)
// then dump the draw-list structure at drawNode+116 so we can decode the per-draw
// (primType, startIndex, indexCount) and translate to native Plume draws.
void Fm2EmitDirtyStateAndDrawList(uint32_t context, uint32_t drawNode,
                                  uint32_t flags) {
  g_origFm2EmitDirtyStateAndDrawList(context, drawNode, flags);
  if (!ShouldMirrorPlumeRenderState())
    return;
  static PresentDiagSlot s_dirtyDrawSlot;
  CountPresentDiag("DirtyDraw", s_dirtyDrawSlot, nullptr);
  static std::atomic<uint32_t> s_n{0};
  if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
    const uint32_t nodeFlags = ReadGuestU32At(drawNode + 108u);
    const uint32_t listHead = ReadGuestU32At(drawNode + 116u);
    const uint32_t count = listHead ? ReadGuestU32At(listHead + 4u) : 0;
    LogReplayDbg(
        "FM2_DIRTY_DRAW ctx=0x%08X node=0x%08X nodeFlags=0x%08X list=0x%08X "
        "count=%u e=[0x%08X 0x%08X | 0x%08X 0x%08X | 0x%08X 0x%08X]",
        context, drawNode, nodeFlags, listHead, count,
        listHead ? ReadGuestU32At(listHead + 8u) : 0,
        listHead ? ReadGuestU32At(listHead + 12u) : 0,
        listHead ? ReadGuestU32At(listHead + 16u) : 0,
        listHead ? ReadGuestU32At(listHead + 20u) : 0,
        listHead ? ReadGuestU32At(listHead + 24u) : 0,
        listHead ? ReadGuestU32At(listHead + 28u) : 0);
  }
}

} // namespace

void FM2PlumeTraceVdSwap(PPCRegister &r3, PPCRegister &r4, PPCRegister &r8,
                         PPCRegister &r9, PPCRegister &r10,
                         PPCRegister &r1) {
  nr::FlushDebugReplayOnPresent();

  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }

  auto readGuestU32 = [](uint32_t guestAddress) -> uint32_t {
    auto *value = ghp::ToHost<rex::be<uint32_t>>(guestAddress);
    return value != nullptr ? value->get() : 0;
  };

  const uint32_t fetchAddress = r4.u32;
  const uint32_t frontbufferAddress = readGuestU32(r8.u32);
  const uint32_t textureFormat = readGuestU32(r9.u32);
  const uint32_t colorSpace = readGuestU32(r10.u32);
  const uint32_t widthPtr = readGuestU32(r1.u32 + 84);
  const uint32_t heightPtr = readGuestU32(r1.u32 + 92);
  const uint32_t width = readGuestU32(widthPtr);
  const uint32_t height = readGuestU32(heightPtr);

  // In plume_native mode, draws go to GPU-side GuestSurface textures, so
  // TranslateGuestTextureFetch (which reads stale guest CPU memory) produces
  // a blank texture. Use the last-bound color render target instead, since
  // all rendering is complete by the time VdSwap fires.
  // Prefer the surface the game hands to VdSwap (its final composited display
  // image) over the last-drawn RT (which is often just one menu layer/strip).
  // Decode the front-buffer base from the fetch constant; if the resolve hook
  // aliased that address to a rendered plume surface, present that composite.
  const uint32_t fbBase =
      ((ReadGuestU32At(fetchAddress + 4u) >> 12) & 0xFFFFFu) << 12;
  rr::GuestBaseTexture *presentSource = LookupSurfaceAperture(fbBase);
  const char *srcKind = "aperture";
  if (presentSource == nullptr || presentSource->texture == nullptr) {
    presentSource =
        rr::TranslateGuestTextureFetch(ghp::ToHost<void>(fetchAddress), true);
    srcKind = "fbfetch";
  }
  if (presentSource == nullptr || presentSource->texture == nullptr) {
    presentSource = rr::GetLastDrawnColorRenderTarget();
    srcKind = "lastdrawn";
  }
  const bool hasTexture =
      presentSource != nullptr && presentSource->texture != nullptr;
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 16)
      LogReplayDbg("FM2_PRESENT_SRC fbBase=0x%08X kind=%s src=%p", fbBase,
                   srcKind, static_cast<void *>(presentSource));
  }

  static uint32_t s_traceCount = 0;
  if (const uint32_t n = NextRenderHookTraceIndex(s_traceCount)) {
    REXGPU_INFO("FM2_PLUME_RENDER_HOOK hook=VdSwap n={} buffer=0x{:08X} "
                "fetch=0x{:08X} frontbuffer=0x{:08X} textureFormat=0x{:08X} "
                "colorSpace=0x{:08X} size={}x{} source={} hasTexture={}",
                n, r3.u32, fetchAddress, frontbufferAddress, textureFormat,
                colorSpace, width, height, static_cast<const void *>(presentSource),
                hasTexture);
  }

  if (!REXCVAR_GET(fm2_plume_vdswap_present)) {
    return;
  }

  if (hasTexture) {
    rr::SetPresentSource(presentSource);
  }
  // FM2PlumeTraceVdSwap fires inside D3DDevice_Swap
  // (0x8236CB28), NOT inside FM2_D3D_TryPresentAndUpdateStatus (0x824F83D8).
  // Fm2Present hooks TryPresentAndUpdateStatus but that function is NOT on the
  // FMOD pump path that drives VdSwap, so Video::Present() would never be
  // called without doing it here.
  Video::Present();
}

// ===========================================================================
// FM2 native renderer hooks: call generated FM2 bodies, then mirror state.
// ===========================================================================
REX_HOOK(FM2_RenderContext_SetPixelShaderState, Fm2SetPixelShaderState);
REX_HOOK(FM2_RenderContext_SetVertexShaderState, Fm2SetVertexShaderState);
// REX_HOOK_RAW: forward full PPCContext to original in passthrough mode.
// uint64_t dirty_mask (arg 5) spans r8:r9 in 32-bit PPC ABI; the standard
// REX_HOOK arg-translation reads only ctx.r8.u32 and zeroes r9 in the
// auto-isolating call, silently discarding the low 32 bits of dirty_mask.
REX_HOOK_RAW(FM2_RenderContext_BindVertexStream) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origFm2BindVertexStream.fn(ctx, base);
    return;
  }
  rex::ppc::HostToGuestFunction<Fm2BindVertexStream>(ctx, base);
}
REX_HOOK(FM2_RenderContext_BindIndexBuffer, Fm2BindIndexBuffer);
REX_HOOK(FM2_RenderContext_SetBoundSurface, Fm2SetBoundSurface);
REX_HOOK(sub_8236E228, Fm2SetActivePassId);
REX_HOOK(sub_823716F8, Fm2BindSurface);
REX_HOOK(FM2_Render_LoadPixelShaderResourceById,
         Fm2LoadPixelShaderResourceById);
REX_HOOK(FM2_Render_LoadVertexShaderResourceById,
         Fm2LoadVertexShaderResourceById);

// Rewired 2026-07-01 to the true XDK identities (see import-block comment).
// The three wrongly-hooked stencil-op setters (TwoSidedStencilMode 0x8236F268,
// StencilFail 0x8236F2D0, StencilPass 0x8236F340) are deliberately NOT hooked
// anymore -- their generated originals run untouched.
REX_HOOK(D3DDevice_SetRenderState_AlphaBlendEnable, Fm2RsAlphaBlendEnable);
REX_HOOK(D3DDevice_SetRenderState_AlphaTestEnable, Fm2RsAlphaTestEnable);
REX_HOOK(D3DDevice_SetRenderState_ZEnable, Fm2RsZEnable);
REX_HOOK(D3DDevice_SetRenderState_ZWriteEnable, Fm2RsZWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_ZFunc, Fm2RsZFunc);
REX_HOOK(D3DDevice_SetRenderState_ColorWriteEnable, Fm2RsColorWriteEnable);
REX_HOOK(D3DDevice_SetRenderState_BlendOp, Fm2RsBlendOp);
REX_HOOK(D3DDevice_SetRenderState_SrcBlend, Fm2RsSrcBlend);
REX_HOOK(D3DDevice_SetRenderState_DestBlend, Fm2RsDestBlend);
REX_HOOK(D3DDevice_SetRenderState_SrcBlendAlpha, Fm2RsSrcBlendAlpha);
REX_HOOK(D3DDevice_SetRenderState_DestBlendAlpha, Fm2RsDestBlendAlpha);
REX_HOOK(FM2_RenderContext_SetClipPlane0Enable, Fm2SetClipPlane0Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane1Enable, Fm2SetClipPlane1Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane2Enable, Fm2SetClipPlane2Enable);
REX_HOOK(FM2_RenderContext_SetClipPlane3Enable, Fm2SetClipPlane3Enable);

REX_HOOK(FM2_D3D_TryPresentAndUpdateStatus, Fm2Present);
REX_HOOK(D3DDevice_Swap,
         Fm2GpuCommandBufferBuildAndSubmit);
// DIAGNOSTIC (session 6P-2, test b): present the SCENE RT directly instead of
// GetLastDrawnColorRenderTarget() (which returns the UI/backbuffer 0x130C41000 not the
// scene 0x130C7F000). g_sceneResolveSource was captured from the (since-corrected,
// see docs/FM2-ida-renames-2026-07-01.md) SetPending_Predicated hook and is no longer
// populated -- this path is dead until the real D3DDevice_Resolve hook sets it. If the
// scene appears -> geometry IS rendered, only present-selection is wrong. If still
// black -> the scene RT is empty (the colorWrite=0 per-draw issue).
// session 6P-2: OFF. true forced presenting g_sceneResolveSource (a black scene
// snapshot), overriding GetLastDrawnColorRenderTarget -- which for the 2D menu is
// the menu RT. Present the last-drawn RT so the menu actually shows.
static constexpr bool kPresentSceneResolveSource = false;
static std::atomic<rr::GuestBaseTexture *> g_sceneResolveSource{nullptr};
namespace fm2::render {
// Accessor so the VRAM viewer can show the latest resolve source = the composited
// frame candidate (incl. the widened swap-framebuffer resolve).
GuestBaseTexture *GetSceneResolveSource() {
  return g_sceneResolveSource.load(std::memory_order_relaxed);
}
} // namespace fm2::render
// Render-worker per-frame dispatch (guest sub_82288948), called in an infinite
// loop by sub_82289640. RAW hook: the original needs its full PPC context
// (r13 thread base, etc.); auto-marshaling would isolate it and trap. After the
// original builds the frame, present on this same thread (the one recording into
// g_commandList) in plume_native/plume_clear, where no other present trigger fires.
REX_HOOK_RAW(sub_82288948) {
  // GATE PROBE (session 6P): RunFrame picks the drain's a2 via `if(v33||v23)`. Read
  // both at frame start (a1=ctx.r3=renderThread): v23 = movie/wait-renderer
  // (a1+2524) active; v33 component = submitted(queue+120) vs processed(a1+2100)
  // backlog. Tells whether the wait-renderer or the backlog keeps a2=1 in plume.
  if (ShouldMirrorPlumeRenderState()) {
    auto rd = [&](uint32_t ga) -> uint32_t {
      auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
      return p ? p->get() : 0u;
    };
    const uint32_t a1 = ctx.r3.u32;
    const uint32_t queue = a1 + 2224u;
    const uint32_t movie = rd(a1 + 2524u);
    // v23 reads a single BYTE at movie+4 (big-endian high byte of the dword).
    const uint32_t movieByte = movie ? ((rd(movie + 4u) >> 24) & 0xFFu) : 0u;
    const uint32_t v23 = (movie != 0u && movieByte != 0u) ? 1u : 0u;
    const uint32_t submitted = rd(queue + 120u);
    const uint32_t processed = rd(a1 + 2100u);
    const int backlog = static_cast<int>(submitted - processed);
    static std::atomic<uint32_t> s_lastSec{0};
    const uint32_t nowSec = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    uint32_t last = s_lastSec.load(std::memory_order_relaxed);
    if (nowSec != last &&
        s_lastSec.compare_exchange_strong(last, nowSec, std::memory_order_relaxed)) {
      // Call the game's own selectors (auto-isolating ctx; safe, lock released
      // before g_orig runs). cd38>1 = backlog pressure; d3e0 = head-node flag.
      const uint32_t cd38 = g_cbQueueCd38(queue);
      const uint32_t d3e0 = g_cbQueueD3e0(queue);
      const uint32_t v33 =
          ((backlog > 1 && cd38 > 1u) || d3e0 != 0u) ? 1u : 0u;
      LogReplayDbg("FM2_RUNFRAME_GATE movie=0x%08X movieByte=%u v23=%u | "
                   "submitted=%u processed=%u backlog=%d cd38=%u d3e0=%u v33=%u "
                   "=> a2=%u",
                   movie, movieByte, v23, submitted, processed, backlog, cd38,
                   d3e0, v33, (v33 || v23) ? 1u : 0u);
    }
  }
  // Start the RenderDoc capture BEFORE the original runs the scene draws (their
  // GPU submit happens inside it), so a real geometry frame is captured in full.
  const bool rdocCapturing =
      ShouldMirrorPlumeRenderState() ? RenderDocFrameBegin() : false;
  g_origRenderWorkerFrame.fn(ctx, base);
  if (!ShouldMirrorPlumeRenderState()) {
    return;
  }
  rr::GuestBaseTexture *presentSource = rr::GetLastDrawnColorRenderTarget();
  // 2026-07-02: prefer the aperture-sourced composite. g_frontbufferPresentSource
  // is now only stored when the Swap hook's frontbuffer-fetch lookup hit a
  // resolve-registered aperture entry (a REAL composited frame assembled by the
  // D3DDevice_Resolve hook), so preferring it unconditionally is correct; the
  // last-drawn RT remains the early-boot fallback only.
  {
    rr::GuestBaseTexture *fb =
        g_frontbufferPresentSource.load(std::memory_order_relaxed);
    if (fb != nullptr && fb->texture != nullptr) {
      presentSource = fb;
    }
  }
  if (kPresentSceneResolveSource) {
    rr::GuestBaseTexture *scene =
        g_sceneResolveSource.load(std::memory_order_relaxed);
    if (scene != nullptr && scene->texture != nullptr) {
      presentSource = scene;
    }
  }
  static PresentDiagSlot s_renderWorkerSlot;
  CountPresentDiag("RenderWorker", s_renderWorkerSlot, presentSource);
  if (presentSource != nullptr && presentSource->texture != nullptr) {
    rr::SetPresentSource(presentSource);
    Video::Present();
  }
  // A (CL-flow diagnostic): how many draws (total) and PM4 scene draws were
  // recorded into g_commandList since the last present, i.e. submitted in THIS
  // frame. sceneDraws>0 => scene draws reach the submitted CL (so an empty scene
  // RT means a rasterization bug, not a submission bug); sceneDraws==0 => they go
  // to a different/discarded recording.
  const uint32_t totalDraws =
      g_drawsSinceLastPresent.exchange(0, std::memory_order_relaxed);
  const uint32_t sceneDraws =
      g_sceneDrawsThisCL.exchange(0, std::memory_order_relaxed);
  {
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 120)
      LogReplayDbg("FM2_CL_SUBMIT path=RenderWorker src=%p totalDraws=%u "
                   "sceneDraws=%u",
                   static_cast<void *>(presentSource), totalDraws, sceneDraws);
  }
  RenderDocFrameEnd(rdocCapturing, totalDraws);
  // Ensure the persistent host-side gate driver is running (starts once; keeps
  // pulsing in every game state even after this loading-screen loop ends).
  StartGatePulseThreadOnce();
}

// Mirror the game's per-draw VS constant uploads into the unified VS const buffer
// that FlushRenderState reads. RAW hook: run the original (it does VMX stores to the
// render-context object), then copy the same a4 vec4 regs (src=r5) at register r4
// into our buffer so the scene transform actually reaches the shader.
REX_HOOK_RAW(sub_8236D958) {
  const uint32_t destReg = ctx.r4.u32;
  const uint32_t srcAddr = ctx.r5.u32;
  const uint32_t count = ctx.r6.u32;
  g_origUploadMatrixConstants.fn(ctx, base);
  if (ShouldMirrorPlumeRenderState()) {
    const void *src = ghp::ToHost<void>(srcAddr);
    if (src != nullptr)
      rr::MirrorPassVsConstants(destReg, src, count);
    static std::atomic<uint32_t> s_n{0};
    if (s_n.fetch_add(1, std::memory_order_relaxed) < 16) {
      LogReplayDbg("FM2_UPLOADMTX destReg=%u count=%u src=0x%08X srcHost=%p",
                   destReg, count, srcAddr, src);
    }
  }
}

REX_HOOK(FM2_FmodIrqSubmit_8236C688, Fm2FmodIrqSubmitSafe);
REX_HOOK(FM2_ProducerProgressGuard_82369340, Fm2ProducerProgressGuard);

REX_HOOK(FM2_D3DDevice_CreateVertexBuffer, CreateVertexBufferAliased);
REX_HOOK_RAW(FM2_D3DDevice_CreateIndexBuffer) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origCreateIndexBuffer.fn(ctx, base);
    return;
  }
  uint32_t length = ctx.r3.u32;
  uint32_t format = ctx.r4.u32;
  uint32_t xdkHandle = g_origCreateIndexBuffer(length, format);
  ctx.r3.u32 = xdkHandle;
  if (!xdkHandle) return;
  GuestBuffer *native = rr::CreateIndexBuffer(length, format);
  if (native)
    rr::RegisterBufferAlias(xdkHandle, native);
}

REX_HOOK(FM2_Render_AllocGpuPassMemoryBlock, Fm2AllocGpuPassMemoryBlock);
REX_HOOK(FM2_D3D_CreateGpuMemoryBlock, Fm2CreateGpuMemoryBlock);

// Lock / unlock
REX_HOOK(FM2_D3DVertexBuffer_Lock, VertexBufferLock);
REX_HOOK(FM2_D3D_LockGpuBufferRaw, IndexBufferLock);
REX_HOOK(FM2_D3DSurface_LockRect, SurfaceLockRect);
REX_HOOK(FM2_D3DResource_UnlockResource, UnlockResourceHook);
REX_HOOK(FM2_D3DSurface_GetDesc, SurfaceGetDesc);

REX_HOOK(D3DDevice_DrawVertices, Fm2EmitIndexedDrawPm4Base);
REX_HOOK(D3DDevice_DrawIndexedVertices,
         Fm2EmitIndexedDrawPm4WithGpuOffset);
REX_HOOK(D3DDevice_DrawIndexedVertices_WithVertexFormatSetup,
         Fm2EmitIndexedDrawPm4WithVertexFormatSetup);
REX_HOOK(FM2_D3D_EmitDirtyStateAndDrawList, Fm2EmitDirtyStateAndDrawList);
// Added 2026-07-01 (docs/FM2-ida-renames-2026-07-01.md): D3DDevice_Resolve was
// only correctly identified today (previously misattributed as the audio-mix
// path). `Resolve()` above (line ~2019) is an existing verbatim port of
// ReOdyssey's Resolve hook -- it was already written, along with the
// StretchRect/FlushPendingStretchRects deferred-resolve machinery in
// render_state.cpp, but had never been wired to a REX_HOOK because nothing
// pointed at the right guest address until now.
// Mode-gated RAW (upgraded same day): unlike ReOdyssey (whose hooks ARE the
// only renderer), FM2 still runs the real guest path in kXenos mode, so the
// original body must run untouched there. `.fn(ctx, base)` passes the full
// PPC context through with no marshaling; the mirror path marshals to the
// ported handler exactly as a plain REX_HOOK would.
REX_HOOK_RAW(D3DDevice_Resolve) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origD3DResolve.fn(ctx, base);
    return;
  }
  rex::ppc::HostToGuestFunction<Resolve>(ctx, base);
}
// Wired 2026-07-01: same story as Resolve -- ClearF and DrawVerticesUP were
// verbatim ReOdyssey-ported handlers sitting orphaned (no REX_HOOK) because
// their FM2 addresses (0x827306C0 / 0x82730D60, both confirmed against Lost
// Odyssey by decompile diff) were only named in the manifest today. ReOdyssey
// hooks both; without them the game's clears and immediate-mode UP draws
// never reach the native renderer.
REX_HOOK_RAW(D3DDevice_ClearF) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origD3DClearF.fn(ctx, base);
    return;
  }
  rex::ppc::HostToGuestFunction<ClearF>(ctx, base);
}
REX_HOOK_RAW(D3DDevice_DrawVerticesUP) {
  if (!ShouldMirrorPlumeRenderState()) {
    g_origD3DDrawVerticesUP.fn(ctx, base);
    return;
  }
  rex::ppc::HostToGuestFunction<DrawVerticesUP>(ctx, base);
}

// Diagnostic: count how often Forza's PM4 draw-list interpreter runs. Confirms
// whether the 3D-scene render path executes in plume_native (vs only the
// loading/wait-screen frame loop). RAW hook: preserve full context for original.
REX_HOOK_RAW(FM2_Render_WalkAndDispatchPm4DrawList) {
  static PresentDiagSlot s_walkSlot;
  CountPresentDiag("WalkDispatch", s_walkSlot, nullptr);
  g_origWalkAndDispatchPm4DrawList.fn(ctx, base);
}

// PATH-LOGGING: forward/object-pass call chain. Counting which of these fire shows
// where the render thread diverges from the forward COLOR pass in plume_native.
REX_HOOK_RAW(FM2_Render_ExecuteSortedDrawLists) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("ExecSorted", s_slot, nullptr);
  g_origExecuteSortedDrawLists.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_SubmitSortedObjectDrawLists) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("SubmitSorted", s_slot, nullptr);
  g_origSubmitSortedObjectDrawLists.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_ObjectPassDrawTraversal) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("ObjPassTrav", s_slot, nullptr);
  g_origObjectPassDrawTraversal.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_PrepareAndWalkObjectPassDrawPackets) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("PrepWalkObj", s_slot, nullptr);
  g_origPrepareAndWalkObjectPassDrawPackets.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_UiOrScreenDrawListSubmit) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("UiScreenSubmit", s_slot, nullptr);
  g_origUiOrScreenDrawListSubmit.fn(ctx, base);
}
// SCENE-entry chain counters (FramePipeline may be table-dispatched; if its counter
// stays 0 while ViewTraversal/SubmitPassWrapper also 0, the scene entry isn't reached).
REX_HOOK_RAW(FM2_Render_FramePipeline) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("FramePipe", s_slot, nullptr);
  // One-shot caller capture. FramePipe is vtable-dispatched and runs in Xenos but
  // never in plume_native; logging its guest caller (ctx.lr) + object (r3) for the
  // first calls identifies the scene-render DISPATCHER, so we can find the gating
  // condition that is false in plume_native. Mode-independent (only fires in the
  // mode where FramePipe actually runs -> Xenos).
  static std::atomic<uint32_t> s_callerLogs{0};
  const uint32_t cnt = s_callerLogs.fetch_add(1, std::memory_order_relaxed);
  if (cnt < 16u) {
    LogReplayDbg("FM2_FRAMEPIPE_CALLER n=%u lr=0x%08X obj=0x%08X tid=%u mirror=%d",
                 cnt, (uint32_t)ctx.lr, (uint32_t)ctx.r3.u32,
                 (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  g_origFramePipeline.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_SubmitPassWrapper) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("SubmitPass", s_slot, nullptr);
  g_origSubmitPassWrapper.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_SceneSliceEntry) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("SceneSlice", s_slot, nullptr);
  g_origSceneSliceEntry.fn(ctx, base);
}
REX_HOOK_RAW(FM2_Render_ViewTraversal) {
  static PresentDiagSlot s_slot;
  CountPresentDiag("ViewTraverse", s_slot, nullptr);
  g_origViewTraversal.fn(ctx, base);
}
// Per-view scene-render dispatcher (calls FramePipe via vtable[+44], gated on
// *(this+2379)). Probe whether plume_native reaches it and the gate/pass-object
// values. Logs the first calls in BOTH modes + a per-second counter.
REX_HOOK_RAW(sub_825E8BF8) {
  const uint32_t self = ctx.r3.u32;
  uint32_t enable2379 = 0xFFFFFFFFu;
  if (auto *p = ghp::ToHost<uint8_t>(self + 2379u))
    enable2379 = *p;
  uint32_t passObj = 0xFFFFFFFFu;
  if (auto *p = ghp::ToHost<rex::be<uint32_t>>(self + 20u))
    passObj = p->get();
  static PresentDiagSlot s_slot;
  CountPresentDiag("SceneViewDisp", s_slot, nullptr);
  static std::atomic<uint32_t> s_logs{0};
  const uint32_t c = s_logs.fetch_add(1, std::memory_order_relaxed);
  if (c < 24u) {
    LogReplayDbg("FM2_SCENEVIEW_DISP n=%u this=0x%08X enable2379=%u passObj=0x%08X "
                 "caller=0x%08X tid=%u mirror=%d",
                 c, self, enable2379, passObj, (uint32_t)ctx.lr,
                 (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  g_origSceneViewDispatch.fn(ctx, base);
}
// Deferred-callback queue dispatcher probe. Counts invocation and walks the node
// list to report whether the scene-render callback (guest 0x825E8BF8) is enqueued.
// session 6P fix toggles.  kForceRunFrameA2ZeroBranch: force RunFrame's v33=0 at the
// sub_8245CD38/sub_8245D3E0 call sites so it takes the a2=0 branch (the proper fix).
// kForceDrainA2Zero: legacy workaround that forces only the drain's a2=0 (kept for
// A/B comparison; superseded by the branch fix above -- leave false).
static constexpr bool kForceRunFrameA2ZeroBranch = true;
static constexpr bool kForceDrainA2Zero = false;
// kForceCarNodeEnqueue: bypass sub_8245CED8's backpressure-drop for the car/scene
// execute node (fn=0x82279610) by forcing its a5 flag !=0, so it enqueues like Xenos.
static constexpr bool kForceCarNodeEnqueue = true;

// RunFrame v33 selector A: sub_8245CD38(queue) returns *(q+60)?1:*(q+128). RunFrame
// uses `cd38 > 1` as the backlog-pressure term. Called from RunFrame at 0x82288D80
// (ctx.lr ~ 0x82288D84). Force <=1 there so that term is false -> pushes v33 toward 0.
REX_HOOK_RAW(sub_8245CD38) {
  const uint32_t lr = (uint32_t)ctx.lr;
  g_cbQueueCd38.fn(ctx, base);  // original result in ctx.r3
  const bool runFrameCaller = (lr >= 0x82288D80u && lr <= 0x82288D88u);
  if (runFrameCaller && ShouldMirrorPlumeRenderState()) {
    static std::atomic<bool> s_logged{false};
    if (!s_logged.exchange(true, std::memory_order_relaxed)) {
      LogReplayDbg("FM2_V33_CD38 lr=0x%08X raw=%u", lr, ctx.r3.u32);
    }
    if (kForceRunFrameA2ZeroBranch && ctx.r3.u32 > 1u) {
      ctx.r3.u32 = 1u;
    }
  }
}
// RunFrame v33 selector B: sub_8245D3E0(queue) returns the head deferred node's flag
// byte. RunFrame ORs it into v33. Called from RunFrame at 0x82288D90
// (ctx.lr ~ 0x82288D94). Force 0 there so v33 collapses to 0 -> a2=0 branch.
REX_HOOK_RAW(sub_8245D3E0) {
  const uint32_t lr = (uint32_t)ctx.lr;
  g_cbQueueD3e0.fn(ctx, base);  // original result in ctx.r3
  const bool runFrameCaller = (lr >= 0x82288D90u && lr <= 0x82288D98u);
  if (runFrameCaller && ShouldMirrorPlumeRenderState()) {
    static std::atomic<bool> s_logged{false};
    if (!s_logged.exchange(true, std::memory_order_relaxed)) {
      LogReplayDbg("FM2_V33_D3E0 lr=0x%08X raw=%u", lr, ctx.r3.u32);
    }
    if (kForceRunFrameA2ZeroBranch) {
      ctx.r3.u32 = 0u;
    }
  }
}
REX_HOOK_RAW(sub_8245D048) {
  auto rd = [&](uint32_t ga) -> uint32_t {
    auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
    return p ? p->get() : 0u;
  };
  // Single byte at guest addr X (big-endian high byte of the dword at X).
  auto rb = [&](uint32_t ga) -> uint32_t { return (rd(ga) >> 24) & 0xFFu; };
  const uint32_t a1 = ctx.r3.u32;
  const uint32_t a2 = ctx.r4.u32;
  const uint32_t v4 = rd(a1 + 56u);
  // Walk the list to find the car/scene EXECUTE node (fn==sub_82279610). For that
  // node, replicate the drain's per-node dispatch decision so we can see WHY it is
  // (or isn't) dispatched.  v6=(*(q+60)!=*(q+56)) || *(q+148); per node:
  // v10=!(*(q+136)||node[16]); v11=v6||v10; dispatch = v11 && (!a2 || !v10).
  uint32_t carNode = 0, carNode16 = 0, nodeCount = 0;
  if (v4) {
    for (uint32_t i = rd(v4); i != 0 && nodeCount < 512u;
         i = rd(i + 20u), ++nodeCount) {
      if (rd(i) == 0x82279610u) { carNode = i; carNode16 = rb(i + 16u); break; }
    }
  }
  static PresentDiagSlot s_slot;
  CountPresentDiag("CbQueueDisp", s_slot, nullptr);
  if (carNode != 0 && ShouldMirrorPlumeRenderState()) {
    const uint32_t q60 = rd(a1 + 60u);
    const uint32_t q148 = rb(a1 + 148u);
    const uint32_t q136 = rd(a1 + 136u);
    const uint32_t v6 = ((q60 != v4) || q148 != 0u) ? 1u : 0u;
    const uint32_t v10 = (q136 != 0u || carNode16 != 0u) ? 0u : 1u;
    const uint32_t v11 = (v6 || v10) ? 1u : 0u;
    const uint32_t disp = (v11 && (a2 == 0u || v10 == 0u)) ? 1u : 0u;
    static std::atomic<uint32_t> s_carLogs{0};
    if (s_carLogs.fetch_add(1, std::memory_order_relaxed) < 40u) {
      LogReplayDbg("FM2_CBQ_CARNODE a1=0x%08X a2=%u node=0x%08X node16=%u "
                   "q56=0x%08X q60=0x%08X q148=%u q136=0x%08X v6=%u v10=%u "
                   "v11=%u disp=%u tid=%u",
                   a1, a2, carNode, carNode16, v4, q60, q148, q136, v6, v10, v11,
                   disp, (unsigned)::GetCurrentThreadId());
    }
  }
  static std::atomic<uint32_t> s_logs{0};
  const uint32_t c = s_logs.fetch_add(1, std::memory_order_relaxed);
  if (c < 24u) {
    LogReplayDbg("FM2_CBQUEUE_DISP n=%u a1=0x%08X a2=%u v4=0x%08X nodes=%u "
                 "car=0x%08X tid=%u mirror=%d",
                 c, a1, a2, v4, nodeCount, carNode,
                 (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  // PROPER FIX (session 6P): the forced-drain-a2=0 below is now DISABLED. Root cause
  // is confirmed (see FM2_RUNFRAME_GATE: v23=0, v33=1 from cd38=7/d3e0=1 backlog). The
  // proper fix forces RunFrame's v33=0 at the sub_8245CD38/sub_8245D3E0 call sites
  // (see those hooks) so RunFrame takes the a2=0 BRANCH naturally -- which calls this
  // drain with a2=0 AND runs the correct GPU-wait/metrics path + avoids the a2=1
  // branch's (*dev+196)(dev,1/0) bracketing. With the branch fix, r4 is already 0 here.
  if (kForceDrainA2Zero && ShouldMirrorPlumeRenderState() && a1 == 0x4001CA20u &&
      ctx.r4.u32 != 0u) {
    ctx.r4.u32 = 0u;
  }
  g_origCbQueueDispatch.fn(ctx, base);
}
// Enqueue API probe: capture the PRODUCER that schedules the scene render
// (fn==sub_825E8BF8). Fires in Xenos (scene enqueued); silent in plume_native.
REX_HOOK_RAW(sub_8245CED8) {
  // Fire-confirmation counter: proves the hook is installed + its call rate.
  static PresentDiagSlot s_slot;
  CountPresentDiag("SchedCb", s_slot, nullptr);
  // FIXED FILTER (session 6P): sub_8245CED8 IS the enqueue that feeds the drain
  // (its append target *(Q+52) = the input buffer's cmd list, swapped to Q+56 by
  // sub_8245D448 and drained by sub_8245D048). The VIEW render node fn is
  // sub_82279610 (=0x82279610), which the OLD scene-only [0x825E0000,0x825F0000)
  // filter EXCLUDED -> we never saw it. Now catch the view-render fns (sub_82279610
  // & siblings at 0x82279xxx, AND sub_825E8BF8 at 0x825E8xxx). caller=ctx.lr = the
  // GUEST producer that schedules the view render. Compare Xenos vs plume.
  const uint32_t fn = ctx.r4.u32;
  if ((fn >= 0x82279000u && fn < 0x8227A000u) ||
      (fn >= 0x825E0000u && fn < 0x825F0000u)) {
    static std::atomic<uint32_t> s_logs{0};
    const uint32_t c = s_logs.fetch_add(1, std::memory_order_relaxed);
    if (c < 48u) {
      LogReplayDbg("FM2_VIEW_ENQUEUE n=%u fn=0x%08X queue=0x%08X this=0x%08X "
                   "arg2=0x%08X caller=0x%08X tid=%u mirror=%d",
                   c, fn, ctx.r3.u32, ctx.r5.u32, ctx.r6.u32, (uint32_t)ctx.lr,
                   (unsigned)::GetCurrentThreadId(),
                   ShouldMirrorPlumeRenderState() ? 1 : 0);
    }
  }
  // CAR-NODE ENQUEUE PROBE (session 6P-2): the car/scene EXECUTE fn (0x82279610) is
  // enqueued here. Only 2/16 reach the drain queue. sub_8245CED8 has 3 internal paths:
  //   path144: *(pool+144)        -> execute fn NOW (not enqueued)
  //   pathBP : *(pool+145) && !a5 && *(pool+140)<=0 -> spin then FreeIfOutsidePool (DROPPED)
  //   else   : ENQUEUE into *(pool+52) list
  // Log a5(r7)/pool flags + predicted path to see why 14/16 are lost.
  if (fn == 0x82279610u && ShouldMirrorPlumeRenderState()) {
    auto rd = [&](uint32_t ga) -> uint32_t {
      auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
      return p ? p->get() : 0u;
    };
    const uint32_t pool = ctx.r3.u32;
    const uint32_t a5 = ctx.r7.u32;
    const uint32_t p144 = (rd(pool + 144u) >> 24) & 0xFFu;
    const uint32_t p145 = (rd(pool + 144u) >> 16) & 0xFFu;
    const int32_t p140 = static_cast<int32_t>(rd(pool + 140u));
    const char *path = p144 ? "exec144"
                       : (p145 && a5 == 0u && p140 <= 0) ? "DROP_bp"
                                                         : "enqueue";
    // FIX TEST (session 6P-2): the backpressure-drop path requires `!a5`. Force the
    // node flag a5(r7)=1 so the condition is false -> the car node ENQUEUEs instead of
    // being freed. a5 also becomes node[16]=1; the drain still dispatches it when v6=1
    // (verified true for the nodes that previously survived). This matches Xenos, where
    // all 16 car renders enqueue.
    if (kForceCarNodeEnqueue && a5 == 0u && p145 && p140 <= 0) {
      ctx.r7.u32 = 1u;
    }
    static std::atomic<uint32_t> s_carLogs{0};
    if (s_carLogs.fetch_add(1, std::memory_order_relaxed) < 40u) {
      LogReplayDbg("FM2_CARENQ pool=0x%08X a5=%u p144=%u p145=%u p140=%d "
                   "arg1=0x%08X arg2=0x%08X caller=0x%08X path=%s forced=%u tid=%u",
                   pool, a5, p144, p145, p140, ctx.r5.u32, ctx.r6.u32,
                   (uint32_t)ctx.lr, path, (kForceCarNodeEnqueue ? 1u : 0u),
                   (unsigned)::GetCurrentThreadId());
    }
  }
  g_origScheduleCallback.fn(ctx, base);
}
// Render-pass trigger probe: which owners have a non-empty entry list, per mode.
static thread_local uint32_t g_curTrigA1 = 0;
static thread_local uint32_t g_curTrigA2 = 0;
REX_HOOK_RAW(FM2_TriggerMatchingListEntryActions) {
  auto rd = [&](uint32_t ga) -> uint32_t {
    auto *p = ghp::ToHost<rex::be<uint32_t>>(ga);
    return p ? p->get() : 0u;
  };
  const uint32_t a1 = ctx.r3.u32;
  const uint32_t a2 = ctx.r4.u32;
  const uint32_t savedA1 = g_curTrigA1, savedA2 = g_curTrigA2;
  g_curTrigA1 = a1;
  g_curTrigA2 = a2;
  const uint32_t sentinel = rd(a1 + 20u);
  const uint32_t nextNode = sentinel ? rd(sentinel) : 0u;
  const bool nonEmpty = (sentinel != 0u && nextNode != 0u && nextNode != sentinel);
  // Per-(a1,a2) status map; log on first sight + whenever empty<->nonEmpty flips.
  static std::atomic<uint32_t> s_mapCount{0};
  static uint64_t s_mapKey[64];
  static uint8_t s_mapStatus[64];
  const uint64_t key = (static_cast<uint64_t>(a1) << 32) | a2;
  const uint8_t status = nonEmpty ? 2u : 1u;
  uint32_t n = s_mapCount.load(std::memory_order_relaxed);
  int idx = -1;
  for (uint32_t i = 0; i < n && i < 64u; ++i)
    if (s_mapKey[i] == key) { idx = static_cast<int>(i); break; }
  if (idx < 0 && n < 64u) {
    idx = static_cast<int>(n);
    s_mapKey[n] = key;
    s_mapStatus[n] = 0u;
    s_mapCount.store(n + 1u, std::memory_order_relaxed);
  }
  if (idx >= 0 && s_mapStatus[idx] != status) {
    s_mapStatus[idx] = status;
    LogReplayDbg("FM2_TRIG a1=0x%08X a2=0x%08X empty=%d sentinel=0x%08X next=0x%08X "
                 "tid=%u mirror=%d",
                 a1, a2, nonEmpty ? 0 : 1, sentinel, nextNode,
                 (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  g_origTriggerListActions.fn(ctx, base);
  g_curTrigA1 = savedA1;
  g_curTrigA2 = savedA2;
}
REX_HOOK_RAW(sub_82279630) {
  static std::atomic<uint32_t> s_logs{0};
  const uint32_t c = s_logs.fetch_add(1, std::memory_order_relaxed);
  if (c < 16u) {
    LogReplayDbg("FM2_CAR_FIRED trigA1=0x%08X trigA2=0x%08X tid=%u mirror=%d",
                 g_curTrigA1, g_curTrigA2, (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  g_origCarReflectionCtor.fn(ctx, base);
}
REX_HOOK_RAW(sub_82279610) {
  static std::atomic<uint32_t> s_logs{0};
  const uint32_t c = s_logs.fetch_add(1, std::memory_order_relaxed);
  if (c < 16u) {
    LogReplayDbg("FM2_CARREFLEXEC n=%u drain_lr=0x%08X this=0x%08X tid=%u mirror=%d",
                 c, (uint32_t)ctx.lr, (uint32_t)ctx.r3.u32,
                 (unsigned)::GetCurrentThreadId(),
                 ShouldMirrorPlumeRenderState() ? 1 : 0);
  }
  g_origCarReflectionExec.fn(ctx, base);
}
// Pool-routing probe: which pool each render-CParams ctor allocs its node into.
REX_HOOK_RAW(sub_82363800) {
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  if (lr >= 0x82278000u && lr < 0x8227B000u) {
    const uint32_t pool = ctx.r3.u32;
    const uint32_t size = ctx.r4.u32;
    static std::atomic<uint32_t> s_cnt{0};
    static uint64_t s_seen[96];
    const uint64_t key = (static_cast<uint64_t>(lr) << 32) | (pool ^ (size << 24));
    uint32_t n = s_cnt.load(std::memory_order_relaxed);
    bool isNew = true;
    for (uint32_t i = 0; i < n && i < 96u; ++i)
      if (s_seen[i] == key) { isNew = false; break; }
    if (isNew && n < 96u) {
      s_seen[n] = key;
      s_cnt.store(n + 1u, std::memory_order_relaxed);
      LogReplayDbg("FM2_CPARAMS_ALLOC lr=0x%08X pool=0x%08X size=%u tid=%u mirror=%d",
                   lr, pool, size, (unsigned)::GetCurrentThreadId(),
                   ShouldMirrorPlumeRenderState() ? 1 : 0);
    }
  }
  g_origAllocPoolBump.fn(ctx, base);
}

// Renamed 2026-07-01 (was Fm2EmitSurfaceResolve, hooking the then-misnamed
// FM2_D3D_EmitSurfaceResolvePackets -- see docs/FM2-ida-renames-2026-07-01.md).
// This hooks D3D::SetPending_Predicated, a per-draw GPU predication-state
// flush called from BeginVertices/DrawVertices/DrawIndexedVertices/Resolve
// alike. It is NOT an EDRAM resolve emitter: the resolve-surface-aliasing
// logic that used to live here (RegisterSurfaceAperture / SnapshotSurfaceFor-
// Resolve / g_sceneResolveSource, reading context+10652/context+12160 as if
// this call meant "a resolve is happening now") fired on every draw, not just
// actual resolves, with whatever stale value those offsets happened to hold.
// Left as a plain passthrough. The real resolve-destination tracking belongs
// on D3DDevice_Resolve (0x8237D158), which genuine render-pass code (not just
// the audio-mix path) calls with a proper typed destination surface -- same
// shape as ReOdyssey's D3DDevice_Resolve/StretchRect hook.
uint32_t SetPendingPredicated(uint32_t context, uint32_t flags, uint32_t a3) {
  return g_origSetPendingPredicated(context, flags, a3);
}
REX_HOOK(D3D_SetPending_Predicated, SetPendingPredicated);
