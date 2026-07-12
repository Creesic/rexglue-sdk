// render/render_internal.h
// Shared internals between the render translation units

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
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

// Matches Video's 2-frame command-list / upload pipelining.
inline constexpr uint32_t kNumFrames = 2;

// The active Plume interface and device created by Video::Init(), or nullptr.
plume::RenderInterface* Interface();
plume::RenderDevice* Device();

// Slot currently being recorded into (0 .. kNumFrames-1). Safe to call from
// the render thread while recording draws / flushing state.
uint32_t CurrentRecordingFrame();

// Called by Video after waiting on a frame slot's fence, just before that
// slot is reused for recording -- resets that frame's upload allocator.
void OnRecordingFrameReady(uint32_t frame);

struct GuestBaseTexture;
void SetPresentSource(GuestBaseTexture* frontBuffer);

plume::RenderDescriptorSet* TextureDescriptorSet();

plume::RenderDescriptorSet* SamplerDescriptorSet();

plume::RenderPipelineLayout* PipelineLayout();

plume::RenderPipeline* GetBlitPipeline(plume::RenderFormat format);

uint32_t AllocTextureDescriptor();
void FreeTextureDescriptor(uint32_t index);

void ExecuteUpload(
    const std::function<void(plume::RenderCommandList*)>& record);
plume::RenderCommandList* CommandList();

// Serializes all recording into the global Plume command list. FM2's guest
// job system can call Clear/Swap/draws from multiple host threads; without
// this, Present's setFramebuffer(nullptr) races Clear's clearColor and
// EnsureFrameStarted's swapchain resize.
std::recursive_mutex& RecordingMutex();

// One-shot DEVICE_REMOVED / create-fail latch. After the first failure, skip
// further GPU creates and Present work to avoid spam/hangs.
bool IsDeviceLost();
void NoteDeviceLost(const char* why);

struct GuestShader;
plume::RenderShader* LoadShader(GuestShader* guestShader, uint32_t specConstants = 0);

// Every D3DVERTEXELEMENT9 declaration FM2 has created. Used to match a vertex
// shader's header usage set to its real input layout (FM2 never binds the
// declaration via the device field).
struct GuestVertexDeclaration;
std::vector<GuestVertexDeclaration*> SnapshotGameDeclarations();

}  // namespace fm2::render
