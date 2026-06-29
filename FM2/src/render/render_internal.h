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

struct GuestVertexDeclaration;
GuestVertexDeclaration *LookupVertexDeclarationAlias(uint32_t guestAddress);
// Snapshot of every D3DVERTEXELEMENT9 declaration FM2 has created. Used to match
// a vertex shader's header usage set to its real input layout (FM2 never binds
// the declaration via the device field).
std::vector<GuestVertexDeclaration *> SnapshotGameDeclarations();

} // namespace fm2::render
