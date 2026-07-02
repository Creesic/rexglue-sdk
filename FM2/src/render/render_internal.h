// render/render_internal.h
// Shared internals between the render translation units 

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <plume_render_interface.h>

namespace fm2::render {

// Bindless table sizes (match the shader-side descriptor arrays / ABI).
inline constexpr uint32_t kTextureDescriptorSize = 65536;
inline constexpr uint32_t kSamplerDescriptorSize = 1024;

inline constexpr uint32_t kNullTexture2DDescriptor = 0;
inline constexpr uint32_t kNullTexture3DDescriptor = 1;
inline constexpr uint32_t kNullTextureCubeDescriptor = 2;
inline constexpr uint32_t kNullTextureDescriptorCount = 3;

// The active Plume interface and device created by Video::Init(), or nullptr.
plume::RenderInterface *Interface();
plume::RenderDevice *Device();

struct GuestBaseTexture;
void SetPresentSource(GuestBaseTexture *frontBuffer);

plume::RenderDescriptorSet *TextureDescriptorSet();

plume::RenderDescriptorSet *SamplerDescriptorSet();

plume::RenderPipelineLayout *PipelineLayout();

plume::RenderPipeline *GetBlitPipeline(plume::RenderFormat format);

uint32_t AllocTextureDescriptor();
void FreeTextureDescriptor(uint32_t index);

void ExecuteUpload(
    const std::function<void(plume::RenderCommandList *)> &record);
plume::RenderCommandList *CommandList();

struct GuestShader;
plume::RenderShader *LoadShader(GuestShader *guestShader,
                                uint32_t specConstants = 0);

// session 6P-3: the game uploads per-draw VS transform constants via
// FM2_RenderContext_UploadMatrixConstants, which writes to a render-context object
// that FlushRenderState's GuestDevice never sees (so device constants are zero for
// scene draws -> geometry collapses). The hook mirrors those uploads here (register-
// indexed, raw big-endian); FlushRenderState uploads this buffer when valid.
void MirrorPassVsConstants(uint32_t startRegister, const void *src,
                           uint32_t vector4fCount);

// 2026-07-02: live float-constant files on the game's PM4 render context.
// UploadMatrixConstants (0x8236D958) writes register r to ctx+0x700+r*16 (VS);
// the PS file sits at ctx+0x1700. The PM4 draw hooks point these host pointers
// at the context around each draw so FlushRenderState uploads the game's real
// register file (raw big-endian) instead of the GuestDevice struct fields the
// PM4 path never writes. Pass nullptr/nullptr to restore struct sourcing.
void SetLiveFloatConstantFiles(const void *vsFile, const void *psFile);

// 2026-07-02: the UI submit (FM2_Render_UiOrScreenDrawListSubmit) stages the
// glyph ModelView (registers 0-3) on ITS context right before emitting the UI
// draw list, but our hooks execute the glyph draws at list-RECORD time. The
// UI-submit hook captures the staged rows (raw big-endian, 4 vec4) here; the
// flush overlays them for no-POSITION shaders.
void SetUiGlyphModelView(const void *rows4);

// 2026-07-02 session 3 (2D spike root cause): the per-element 2D placement
// matrices are emitted ONLY as PM4 SET_CONSTANT / Type-0 ALU / LOAD_ALU
// packets in the FM2 command buffer -- plume_native's disabled CP never
// applied them, so 2D vertex shaders read c0-c3 = leftovers (colors) + zeros
// and elements collapse into wedges/spikes. The command-buffer scanner in
// d3d_hooks (ProcessPm4VsConstantsDiag) now APPLIES ALU float writes here in
// stream order (dword-indexed VS file, raw big-endian dwords, coverage per
// vec4 register); FlushRenderState overlays covered registers over the live
// context file for no-POSITION (2D) shaders.
void ApplyPm4VsConstants(uint32_t dwordIndex, const void *beDwords,
                         uint32_t dwordCount);
// PS half of the PM4 ALU space (packet idx 0x400-0x7FF, rebase before call).
void ApplyPm4PsConstants(uint32_t dwordIndex, const void *beDwords,
                         uint32_t dwordCount);
// Reset the per-delta "fresh" register set (see render_state.cpp); the
// scanner calls this before walking each command-buffer delta so 3D draws
// only overlay constants written immediately before them.
void BeginPm4ConstantDelta();

struct GuestVertexDeclaration;
GuestVertexDeclaration *LookupVertexDeclarationAlias(uint32_t guestAddress);
// Snapshot of every D3DVERTEXELEMENT9 declaration FM2 has created. Used to match
// a vertex shader's header usage set to its real input layout (FM2 never binds
// the declaration via the device field).
std::vector<GuestVertexDeclaration *> SnapshotGameDeclarations();

} // namespace fm2::render
