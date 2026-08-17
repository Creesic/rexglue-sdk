// render/render_internal.h
// Shared internals between the render translation units

#pragma once

#include <cstdint>
#include <memory>
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

// FM2's guest frame, and the EDRAM tile height it binds during predicated
// tiling. These describe the *guest*: the host swapchain follows the window and
// is unrelated, so never substitute Video::s_viewport* for them.
inline constexpr uint32_t kFm2FrameWidth = 1280;
inline constexpr uint32_t kFm2FrameHeight = 720;
inline constexpr uint32_t kFm2TileHeight = 256;

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

// Resolve-destination aperture: guest page base -> host texture that received
// D3DDevice_Resolve copies (FM2's composited frontbuffer). Swap looks this up
// from the frontbuffer fetch; Present prefers it over sticky color RTs.
void RegisterResolveSurfaceAperture(uint32_t guestAddr, GuestBaseTexture* host);
GuestBaseTexture* LookupResolveSurfaceAperture(uint32_t guestAddr);
void ClearResolveSurfaceAperture(GuestBaseTexture* host);
void SetFrontbufferPresentSource(GuestBaseTexture* tex);
GuestBaseTexture* ConsumeFrontbufferPresentSource();
// When StretchRect skipped a format-mismatched resolve, Present prefers this
// live scene RT over the empty aperture Swap registered.
GuestBaseTexture* ConsumeStretchRectPresentOverride();

plume::RenderDescriptorSet* TextureDescriptorSet();

plume::RenderDescriptorSet* SamplerDescriptorSet();

plume::RenderPipelineLayout* PipelineLayout();

plume::RenderPipeline* GetBlitPipeline(plume::RenderFormat format);

uint32_t AllocTextureDescriptor();
void FreeTextureDescriptor(uint32_t index);

// Frame upload scratch (graphics CL path for UnlockTextureRect / constants).
plume::RenderBufferReference UploadFrameData(const void* src, uint64_t size, bool byteSwap = false);
// Keep a staging upload buffer alive until the recording frame's fence retires.
void RetainTempUploadBuffer(std::unique_ptr<plume::RenderBuffer> buffer);

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
struct GuestDevice;
struct GuestVertexDeclaration;
std::vector<GuestVertexDeclaration*> SnapshotGameDeclarations();

// Constant transport from FM2's render contexts into the native draw path.
// Inputs are raw big-endian register files and remain byte-swapped until the
// per-draw upload.
// Recorded command-buffer object-pass draws are captured and replayed from
// EmitDirty. While this is true, do not substitute the placeholder PS during
// the record bracket — replay submits with the real shaders.
inline constexpr bool kObjPassRecordReplay = true;

void MirrorPassVsConstants(uint32_t startRegister, const void* src, uint32_t vector4fCount);
void CaptureObjPassWvp(uint32_t startRegister, const void* src, uint32_t vector4fCount);
void SetLiveFloatConstantFiles(const void* vsFile, const void* psFile);
void SetLastDrawCallerLr(uint32_t lr);
void SetScene3dVsOverlayFromDevice(const void* beDwords, uint32_t firstReg, uint32_t regCount);

GuestShader* GetPipelineVertexShader();
GuestShader* GetPipelinePixelShader();
GuestVertexDeclaration* GetPipelineVertexDeclaration();

// Raster/output snapshot paired with a recorded object-pass draw.
struct ObjReplayRenderState {
  bool zEnable = true;
  bool zWriteEnable = true;
  uint32_t zFunc = 0;
  uint32_t cullMode = 0;
  bool alphaBlendEnable = false;
  uint32_t srcBlend = 0;
  uint32_t destBlend = 0;
  uint32_t blendOp = 0;
  uint32_t srcBlendAlpha = 0;
  uint32_t destBlendAlpha = 0;
  uint32_t blendOpAlpha = 0;
  uint32_t colorWriteEnable = 0xFu;
  bool stencilEnable = false;
  uint8_t stencilReadMask = 0xFF;
  uint8_t stencilWriteMask = 0xFF;
  uint8_t stencilRef = 0;
  uint32_t stencilFrontFunc = 0;
  uint32_t stencilFrontFail = 0;
  uint32_t stencilFrontDepthFail = 0;
  uint32_t stencilFrontPass = 0;
  uint32_t stencilBackFunc = 0;
  uint32_t stencilBackFail = 0;
  uint32_t stencilBackDepthFail = 0;
  uint32_t stencilBackPass = 0;
  float slopeScaledDepthBias = 0.0f;
  int32_t depthBias = 0;
  bool depthClipEnabled = true;
  float clipPlane[4]{};
  uint32_t clipPlaneEnabled = 0;
  float alphaThreshold = 0.0f;
};

void CaptureObjReplayRenderState(ObjReplayRenderState& out);
void RestoreObjReplayRenderState(const ObjReplayRenderState& in);

}  // namespace fm2::render
