// render/guest_device.h

#pragma once

#include <cstddef>
#include <cstdint>

#include <rex/types.h>

namespace fm4::render {

// One Xenos fetch-constant slot: 6 dwords. The device's fetch-constant file is
// 32 of these (0x300 bytes) at kGuestFetchConstantBase; texture fetch constants
// occupy the low slots (SetTexture writes device + 0x480 + 24*sampler) and the
// vertex fetch constants live in the tail (SetStreamSource writes the pair at
// device + 0x778 - 8*stream).
struct GuestFetchConstant {
  rex::be<uint32_t> data[6];
};

// Byte-exact overlay of FM4's live D3D::CDevice. Derived from
// FM4/docs/fm2p-port-hook-map.md sections 7.1/7.2 and
// FM4/docs/native-render-plume-deepdive.md section 4.2, both against ida40.
//
// FM4 is NOT layout-identical to FM2: m_Constants sits at 0x480 rather than
// 0x400, so every field from 0x400 up is shifted by exactly +0x80, and the
// private tail past the 0x2B00 public struct was reordered rather than shifted
// (measured deltas +0x160, +0x1A0, +0x278, +0x2A4, +0x2B0). Do not extrapolate
// a single delta across the tail; each field below cites its own derivation.
struct GuestDevice {
  // 0x0000 m_Pending.m_Mask[5] + m_Predicated_PendingMask2 + m_pRing/Limit/
  // Guarantee. Unchanged from FM2: every setter in both binaries still uses
  // ld/std r11,0x10(r3) etc. for the pending masks.
  rex::be<uint64_t> dirtyFlags[8];

  rex::be<uint32_t> setRenderStateFunctions[0x65];  // 0x0040 m_SetRenderStateCall
  uint32_t setSamplerStateFunctions[0x14];          // 0x01D4 m_SetSamplerStateCall

  uint8_t padding224[0x25C];  // 0x0224 m_GetRenderStateCall / m_GetSamplerStateCall

  // 0x0480 m_Constants. 32 slots x 24 bytes = 0x300, ending exactly where the
  // VS float file begins. FM2P shipped 16 slots at 0x480 plus 0x100 of padding,
  // which was correct for FM2's 0x400 base only by accident; for FM4 the full
  // 32-slot file at 0x480 is both the real layout and the one that lines the
  // float files up at 0x780/0x1780.
  GuestFetchConstant textureFetchConstants[0x20];

  // Raw host-endian shader constant register files. Bool and loop files follow
  // the float files immediately, as in FM2.
  uint32_t vertexShaderFloatConstants[0x400];  // 0x0780
  uint32_t pixelShaderFloatConstants[0x400];   // 0x1780

  rex::be<uint32_t> vertexShaderBoolConstants[0x4];  // 0x2780 (setter word index 0x9E0)
  rex::be<uint32_t> pixelShaderBoolConstants[0x4];   // 0x2790 (setter word index 0x9E4)

  rex::be<uint32_t> vertexShaderIntConstants[0x10];  // 0x27A0
  rex::be<uint32_t> pixelShaderIntConstants[0x10];   // 0x27E0

  rex::be<float> clipPlanes[6][4];  // 0x2820 m_ClipPlanes

  // 0x2880..0x2B00: m_DestinationPacket, m_WindowPacket, m_ValuesPacket,
  // m_ProgramPacket, m_ControlPacket, m_Tessellator/Misc/PointPacket,
  // m_MaxAnisotropy[26], m_ZFilter[26]. Read through the kGuest*Offset
  // constants below rather than as members: the sub-struct field order is only
  // partly known and byte offsets are what every derivation in the hook map is
  // expressed in.
  uint8_t packets[0x280];

  uint8_t padding2B00[0x4B8];         // 0x2B00 private CDevice: ring ptr 0x2B10,
                                      // current token 0x2B08, flags 0x2B3D,
                                      // force-timer-refresh 0x2B8C
  rex::be<uint32_t> vertexDeclaration;  // 0x2FB8 (FM2 0x2D14, +0x2A4)
  uint8_t padding2FBC[0x14];
  uint8_t streamStrideShadow[0x10];   // 0x2FD0 compare copy of stride/4 (FM2 0x2D20, +0x2B0)
  rex::be<uint32_t> immediateMirrorWord;   // 0x2FE0 (FM2 0x2D30, +0x2B0)
  rex::be<uint32_t> immediateMirrorFlags;  // 0x2FE4 (FM2 0x2D34, +0x2B0)
  uint8_t padding2FE8[0x20C];         // 0x2FE8 scissor-test enable (see below)
  rex::be<uint32_t> boundIndexBuffer;      // 0x31F4 (FM2 0x2F7C, +0x278)
  uint8_t padding31F8[0x14];               // 0x31F8 the 4 colour + 1 depth surfaces
  rex::be<uint32_t> boundVertexStreams[0x10];  // 0x320C (FM2 0x2F94, +0x278)
  uint8_t padding324C[0x4];
  uint8_t streamStrideDwords[0x10];        // 0x3250 stride/4 per stream (FM2 0x2FD8, +0x278)
  uint8_t padding3260[0xCC];               // 0x32C8 viewport floats, 0x32E4 scissor rect
  uint8_t tileRects[0x2D54];               // 0x332C BeginTiling rect array; the
                                           // element count is measured in Task 8
};

static_assert(sizeof(GuestDevice) == 0x6080);
static_assert(offsetof(GuestDevice, textureFetchConstants) == 0x480);
static_assert(offsetof(GuestDevice, vertexShaderFloatConstants) == 0x780);
static_assert(offsetof(GuestDevice, pixelShaderFloatConstants) == 0x1780);
static_assert(offsetof(GuestDevice, vertexShaderBoolConstants) == 0x2780);
static_assert(offsetof(GuestDevice, pixelShaderBoolConstants) == 0x2790);
static_assert(offsetof(GuestDevice, vertexShaderIntConstants) == 0x27A0);
static_assert(offsetof(GuestDevice, pixelShaderIntConstants) == 0x27E0);
static_assert(offsetof(GuestDevice, clipPlanes) == 0x2820);
static_assert(offsetof(GuestDevice, packets) == 0x2880);
static_assert(offsetof(GuestDevice, vertexDeclaration) == 0x2FB8);
static_assert(offsetof(GuestDevice, streamStrideShadow) == 0x2FD0);
static_assert(offsetof(GuestDevice, immediateMirrorWord) == 0x2FE0);
static_assert(offsetof(GuestDevice, immediateMirrorFlags) == 0x2FE4);
static_assert(offsetof(GuestDevice, boundIndexBuffer) == 0x31F4);
static_assert(offsetof(GuestDevice, boundVertexStreams) == 0x320C);
static_assert(offsetof(GuestDevice, streamStrideDwords) == 0x3250);
static_assert(offsetof(GuestDevice, tileRects) == 0x332C);

// Byte offsets used directly by render_state.cpp and the draw-time register
// sampler. Every value is +0x80 from its FM2 counterpart except where noted.
inline constexpr uint32_t kGuestFetchConstantBase = 0x480;   // FM2 0x400
inline constexpr uint32_t kGuestTextureFetchStride = 24;     // Xenos slot stride
inline constexpr uint32_t kGuestVertexFetchBase = 0x778;     // FM2 0x6F8; stream N at base - 8*N
inline constexpr uint32_t kGuestVertexFetchStride = 8;
inline constexpr uint32_t kGuestClipPlanesOffset = 0x2820;   // FM2 0x2820 (member move, not +0x80)
inline constexpr uint32_t kGuestClipPlaneMask = 0x3F;        // 6 planes

inline constexpr uint32_t kGuestValuesPacketOffset = 0x28CC;   // m_ValuesPacket
inline constexpr uint32_t kGuestCcwStencilWriteMaskOffset = 0x28FD;  // FM2 0x287D
inline constexpr uint32_t kGuestCcwStencilMaskOffset = 0x28FE;       // FM2 0x287E
inline constexpr uint32_t kGuestCcwStencilRefOffset = 0x28FF;        // FM2 0x287F
inline constexpr uint32_t kGuestStencilWriteMaskOffset = 0x2901;     // FM2 0x2881
inline constexpr uint32_t kGuestStencilMaskOffset = 0x2902;          // FM2 0x2882
inline constexpr uint32_t kGuestStencilRefOffset = 0x2903;           // FM2 0x2883
inline constexpr uint32_t kGuestControlPacketOffset = 0x2934;  // DepthControl, FM2 0x28B4
inline constexpr uint32_t kGuestBlendControl0Offset = 0x2938;  // FM2 0x28B8
inline constexpr uint32_t kGuestColorControlOffset = 0x293C;   // alpha test/func, FM2 0x28BC
inline constexpr uint32_t kGuestBlendMirror1Offset = 0x2958;   // BlendControl1, FM2 0x28D8
inline constexpr uint32_t kGuestBlendMirror2Offset = 0x295C;   // BlendControl2, FM2 0x28DC
inline constexpr uint32_t kGuestBlendMirror3Offset = 0x2960;   // BlendControl3, FM2 0x28E0
inline constexpr uint32_t kGuestModeControlOffset = 0x2948;    // raster/cull, FM2 0x28C8

// Derived in Task 4 step 3 against ida40.
//
// Viewport: D3D_SetViewport (0x822FEB28) stores its six float parameters at
// device + 0x32C8 (x), 0x32CC (y), 0x32D0 (width), 0x32D4 (height), 0x32D8
// (minZ) and 0x32DC (maxZ), then derives m_ValuesPacket's VportXScale/XOffset/
// YScale/YOffset/ZScale/ZOffset at 0x2908.. from them. The same floats are read
// back by D3DDevice_SetScissorRect (0x822FF050) and copied by
// D3DDevice_BeginTiling (addi r4,r31,0x32C8). FM2 had 0x3168: a +0x160 move.
inline constexpr uint32_t kGuestViewportOffset = 0x32C8;
// Scissor-test enable: D3DDevice_SetRenderState_ScissorTestEnable (0x822BF500)
// is the whole thunk -- "stw r4,0x2FE8(r3); addi r4,r3,0x32E4;
// b D3DDevice_SetScissorRect". D3DDevice_SetScissorRect clamps the incoming
// RECT to the viewport only when this dword is nonzero, and the clear path
// (sub_822EBE20) intersects against the rect at 0x32E4 only when it is nonzero.
// FM2 had 0x2E48 (rect at 0x306C): a +0x1A0 move.
inline constexpr uint32_t kGuestScissorEnableOffset = 0x2FE8;
// D3DDevice_SetScissorRect writes the live RECT here (left/top/right/bottom).
inline constexpr uint32_t kGuestScissorRectOffset = 0x32E4;

// Six big-endian floats: x, y, width, height, minZ, maxZ. Returns nullptr when
// the offset has not been located, which every caller must handle.
inline const rex::be<float>* GuestViewportFloats(const GuestDevice* device) {
  if (kGuestViewportOffset == 0 || device == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<const rex::be<float>*>(reinterpret_cast<const uint8_t*>(device) +
                                                 kGuestViewportOffset);
}

// Nonzero when the guest has scissor testing on. Returns false when the offset
// has not been located (scissor off is the safe default: it draws too much,
// never too little).
inline bool GuestScissorEnable(const GuestDevice* device) {
  if (kGuestScissorEnableOffset == 0 || device == nullptr) {
    return false;
  }
  return reinterpret_cast<const rex::be<uint32_t>*>(reinterpret_cast<const uint8_t*>(device) +
                                                    kGuestScissorEnableOffset)->get() != 0;
}

struct GuestViewport {
  rex::be<uint32_t> x;
  rex::be<uint32_t> y;
  rex::be<uint32_t> width;
  rex::be<uint32_t> height;
  rex::be<float> minZ;
  rex::be<float> maxZ;
};

struct GuestRect {
  rex::be<int32_t> left;
  rex::be<int32_t> top;
  rex::be<int32_t> right;
  rex::be<int32_t> bottom;
};

struct GuestPoint {
  rex::be<int32_t> x;
  rex::be<int32_t> y;
};

// D3DLOCKED_RECT.
struct GuestLockedRect {
  rex::be<int32_t> pitch;
  rex::be<uint32_t> bits;
};

// D3DSURFACE_DESC.
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

enum GuestRenderState : uint32_t {
  D3DRS_ZENABLE = 40,
  D3DRS_ZFUNC = 44,
  D3DRS_ZWRITEENABLE = 48,
  D3DRS_FILLMODE = 52,
  D3DRS_CULLMODE = 56,
  D3DRS_ALPHABLENDENABLE = 60,
  D3DRS_SEPARATEALPHABLENDENABLE = 64,
  D3DRS_BLENDFACTOR = 68,
  D3DRS_SRCBLEND = 72,
  D3DRS_DESTBLEND = 76,
  D3DRS_BLENDOP = 80,
  D3DRS_SRCBLENDALPHA = 84,
  D3DRS_DESTBLENDALPHA = 88,
  D3DRS_BLENDOPALPHA = 92,
  D3DRS_ALPHATESTENABLE = 96,
  D3DRS_ALPHAREF = 100,
  D3DRS_ALPHAFUNC = 104,
  D3DRS_STENCILENABLE = 108,
  D3DRS_TWOSIDEDSTENCILMODE = 112,
  D3DRS_STENCILFAIL = 116,
  D3DRS_STENCILZFAIL = 120,
  D3DRS_STENCILPASS = 124,
  D3DRS_STENCILFUNC = 128,
  D3DRS_STENCILREF = 132,
  D3DRS_STENCILMASK = 136,
  D3DRS_STENCILWRITEMASK = 140,
  D3DRS_CCWSTENCILFAIL = 144,
  D3DRS_CCWSTENCILZFAIL = 148,
  D3DRS_CCWSTENCILPASS = 152,
  D3DRS_CCWSTENCILFUNC = 156,
  D3DRS_CCWSTENCILREF = 160,
  D3DRS_CCWSTENCILMASK = 164,
  D3DRS_CCWSTENCILWRITEMASK = 168,
  D3DRS_CLIPPLANEENABLE = 172,
  D3DRS_SCISSORTESTENABLE = 200,
  D3DRS_SLOPESCALEDEPTHBIAS = 204,
  D3DRS_DEPTHBIAS = 208,
  D3DRS_COLORWRITEENABLE = 212,
  D3DRS_VIEWPORTENABLE = 304,
};

// Verified against FM4 (Task 4 step 6): D3DDevice_SetRenderState_CullMode
// (0x823505F8) is
//   ModeControl = (ModeControl & ~7) | (Value & 7); m_Pending.m_Mask[2] |= 0x40;
// so the value drops straight into PA_SU_SC_MODE_CNTL bits 0..2 with no
// remapping table: bit0 CULL_FRONT, bit1 CULL_BACK, bit2 FACE (winding).
// FM2's D3DDevice_SetRenderState_CullMode (0x8236EA60) packs identically into
// its 0x28C8, so the enum carries over unchanged. To invert: cullFront =
// value & 1, cullBack = value & 2, frontFaceCcw = (value & 4) != 0.
enum GuestCullMode : uint32_t {
  D3DCULL_NONE = 0,
  D3DCULL_CW = 2,
  D3DCULL_NONE_2 = 4,
  D3DCULL_CCW = 6,
};

enum GuestBlendMode : uint32_t {
  D3DBLEND_ZERO = 0,
  D3DBLEND_ONE = 1,
  D3DBLEND_SRCCOLOR = 4,
  D3DBLEND_INVSRCCOLOR = 5,
  D3DBLEND_SRCALPHA = 6,
  D3DBLEND_INVSRCALPHA = 7,
  D3DBLEND_DESTCOLOR = 8,
  D3DBLEND_INVDESTCOLOR = 9,
  D3DBLEND_DESTALPHA = 10,
  D3DBLEND_INVDESTALPHA = 11,
};

enum GuestBlendOp : uint32_t {
  D3DBLENDOP_ADD = 0,
  D3DBLENDOP_SUBTRACT = 1,
  D3DBLENDOP_MIN = 2,
  D3DBLENDOP_MAX = 3,
  D3DBLENDOP_REVSUBTRACT = 4,
};

enum GuestCmpFunc : uint32_t {
  D3DCMP_NEVER = 0,
  D3DCMP_LESS = 1,
  D3DCMP_EQUAL = 2,
  D3DCMP_LESSEQUAL = 3,
  D3DCMP_GREATER = 4,
  D3DCMP_NOTEQUAL = 5,
  D3DCMP_GREATEREQUAL = 6,
  D3DCMP_ALWAYS = 7,
};

enum GuestPrimitiveType : uint32_t {
  D3DPT_POINTLIST = 1,
  D3DPT_LINELIST = 2,
  D3DPT_LINESTRIP = 3,
  D3DPT_TRIANGLELIST = 4,
  D3DPT_TRIANGLEFAN = 5,
  D3DPT_TRIANGLESTRIP = 6,
  D3DPT_RECTLIST = 8,  // Xbox: 3 verts define an axis-aligned rect
  D3DPT_QUADLIST = 13,
};

enum GuestDeclType : uint32_t {
  D3DDECLTYPE_FLOAT1 = 0x2C83A4,
  D3DDECLTYPE_FLOAT2 = 0x2C23A5,
  D3DDECLTYPE_FLOAT3 = 0x2A23B9,
  D3DDECLTYPE_FLOAT4 = 0x1A23A6,
  D3DDECLTYPE_D3DCOLOR = 0x182886,
  D3DDECLTYPE_UBYTE4 = 0x1A2286,
  D3DDECLTYPE_UBYTE4_2 = 0x1A2386,
  D3DDECLTYPE_SHORT2 = 0x2C2359,
  D3DDECLTYPE_SHORT4 = 0x1A235A,
  D3DDECLTYPE_UBYTE4N = 0x1A2086,
  D3DDECLTYPE_UBYTE4N_2 = 0x1A2186,
  D3DDECLTYPE_SHORT2N = 0x2C2159,
  D3DDECLTYPE_SHORT4N = 0x1A215A,
  D3DDECLTYPE_USHORT2N = 0x2C2059,
  D3DDECLTYPE_USHORT4N = 0x1A205A,
  D3DDECLTYPE_UINT1 = 0x2C82A1,
  D3DDECLTYPE_UDEC3 = 0x2A2287,
  D3DDECLTYPE_DEC3N = 0x2A2187,
  D3DDECLTYPE_DEC3N_2 = 0x2A2190,
  D3DDECLTYPE_DEC3N_3 = 0x2A2390,
  D3DDECLTYPE_FLOAT16_2 = 0x2C235F,
  D3DDECLTYPE_FLOAT16_4 = 0x1A2360,
  D3DDECLTYPE_UNUSED = 0xFFFFFFFF,
};

enum GuestDeclUsage : uint32_t {
  D3DDECLUSAGE_POSITION = 0,
  D3DDECLUSAGE_BLENDWEIGHT = 1,
  D3DDECLUSAGE_BLENDINDICES = 2,
  D3DDECLUSAGE_NORMAL = 3,
  D3DDECLUSAGE_PSIZE = 4,
  D3DDECLUSAGE_TEXCOORD = 5,
  D3DDECLUSAGE_TANGENT = 6,
  D3DDECLUSAGE_BINORMAL = 7,
  D3DDECLUSAGE_TESSFACTOR = 8,
  D3DDECLUSAGE_POSITIONT = 9,
  D3DDECLUSAGE_COLOR = 10,
  D3DDECLUSAGE_FOG = 11,
  D3DDECLUSAGE_DEPTH = 12,
  D3DDECLUSAGE_SAMPLE = 13,
};

enum GuestTextureFilterType : uint32_t {
  D3DTEXF_POINT = 0,
  D3DTEXF_LINEAR = 1,
  D3DTEXF_NONE = 2,
};

enum GuestTextureAddress : uint32_t {
  D3DTADDRESS_WRAP = 0,
  D3DTADDRESS_MIRROR = 1,
  D3DTADDRESS_CLAMP = 2,
  D3DTADDRESS_MIRRORONCE = 3,
  D3DTADDRESS_BORDER = 6,
};

constexpr uint32_t D3DCLEAR_TARGET = 0x1;
constexpr uint32_t D3DCLEAR_ZBUFFER = 0x10;
constexpr uint32_t D3DCLEAR_STENCIL = 0x20;

// The single active GuestDevice instance (FM2 only ever creates one D3D9
// device). Set by whichever hook translates the guest's device-create call.
inline GuestDevice* g_activeGuestDevice = nullptr;
inline void SetActiveGuestDevice(GuestDevice* device) { g_activeGuestDevice = device; }
inline GuestDevice* GetActiveGuestDevice() { return g_activeGuestDevice; }

}  // namespace fm4::render
