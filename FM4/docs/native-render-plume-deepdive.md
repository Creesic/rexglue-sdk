# Native rendering for Forza on ReXGlue

Deep dive into how UnleashedRecomp and re:Blue integrate Plume, what FM4's D3D layer looks like in IDA, and how a native rendering option would fit the ReXGlue SDK. Written 2026-09-02.

Sources: `C:\Users\Tera\Documents\GitHub\UnleashedRecomp`, `C:\Users\Tera\Documents\GitHub\reblue`, this repo, and the IDA databases ida38 (Unleashed), ida39 (Blue Dragon), ida40 (Forza 4). Line references are to the files as checked out on that date.

---

## 0. Summary

ReXGlue today renders through `rexgpu-xenos`, a Xenia-derived GPU emulator: the game's D3D library builds PM4 packets into a ring buffer, and the plugin's command processor consumes them, translates Xenos microcode at runtime, and emulates EDRAM. UnleashedRecomp and re:Blue do the opposite: they replace the game's `D3DDevice_*` entry points with host code that drives Plume directly, with shaders precompiled offline by XenosRecomp. The ring buffer, EDRAM and every `Vd*` kernel export are stubs.

re:Blue is the more important reference: it is built on ReXGlue 0.10.0 itself, compiles a Plume renderer straight into the title (no GPU plugin, no render thread), hooks the D3D entry points symbolically through `config/functions.toml`, and reads per-draw state from the Xenos register shadows inside the guest `D3DDevice`. Its `src/gpu/` tree is the closest existing "native option on ReXGlue" and the right code to lift from.

The recommendation is a **hybrid renderer in the reblue style**, packaged as an SDK library plus an FM4 title profile. Four facts make this tractable:

- FM4's IDB already names about 60 `D3DDevice_*` entry points, 180 `D3D::` internals, and carries the XDK `D3DDevice` struct with its register shadows and constant files typed. Unleashed and reblue had to reverse all of that by hand.
- The XDK `D3DDevice` public layout is identical across Unleashed, Blue Dragon and FM4, so the device-table trick, the dirty-flag decoding and the register-shadow reads transfer as-is.
- ReXGlue already provides weak-alias function overrides, named guest functions in config, runtime hooking by address, a guest system heap, `RuntimeConfig::graphics` injection from the title app, and the Xenia texture untiling helpers reblue already uses.
- reblue's XenosRecomp fork, prelinker, PSO tooling, and Plume fork are all reusable with an `FM4_RECOMP` variant of the shared-constants block.

Two things FM4 does that neither reference game does, and which the design must cover from day one: it plays back pre-recorded `D3DCommandBuffer` objects from its engine render passes, and it uses predicated tiling (`D3DDevice_BeginTiling`) through its `TileBuffer` class.

---

## 1. What ReXGlue already has

### 1.1 Graphics is a runtime plugin

`fm4.toml` selects `gpu_plugin = "xenos"`. `src/ui/rex_app.cpp:338` loads `rexgpu-<name>.dll` via `rex::system::LoadGpuPlugin`, then calls `SetupPresentation(&app_context())` and `SetupGuestGpu(...)`. The ABI is small (`include/rex/system/gpu_plugin.h`):

```cpp
extern "C" uint32_t rex_gpu_abi_version();                    // kGpuPluginAbiVersion = 1
extern "C" IGraphicsSystem* rex_gpu_create(uint32_t abi, const GpuCreateInfo*);  // backend "d3d12"|"vulkan"|"any"
```

`IGraphicsSystem` (`include/rex/system/interfaces/graphics.h`) is what a native plugin implements:

| method | xenos plugin | native plugin |
|---|---|---|
| `SetupPresentation(WindowedAppContext*)` | creates `GraphicsProvider` + `Presenter` | create Plume interface/device/swapchain on `window->GetNativeWindowHandle()` (`include/rex/ui/window.h:291`) |
| `SetupGuestGpu(FunctionDispatcher*, KernelState*)` | starts command processor, maps GPU MMIO | install guest hooks, allocate fake device |
| `InitializeRingBuffer`, `EnableReadPointerWriteBack`, `SetInterruptCallback` | real | no-ops (never reached once `Direct3D_CreateDevice` is replaced) |
| `presenter()` | drives the ImGui overlays (`rex_app.cpp:383`) | may return `nullptr`: "detached mode", overlays off (`rex_app.cpp:415`) |

The CMake side is `rexglue_setup_target(fm4 GPU_PLUGINS xenos)` (`FM4/CMakeLists.txt:29`); `cmake/rexglue_helpers.cmake:145-166` resolves each name to a `rexgpu-<name>` target and stages the DLL beside the executable. Adding `native` to that list is the whole integration on the title side.

### 1.2 Kernel `Vd*` exports route to the plugin

`src/kernel/xboxkrnl/xboxkrnl_video.cpp` implements `VdInitializeRingBuffer`, `VdEnableRingBufferRPtrWriteBack`, `VdSetGraphicsInterruptCallback`, `VdSwap`, `VdQueryVideoMode`, `VdGetCurrentDisplayInformation` and stubs the rest. The ring-buffer and interrupt ones forward to `IGraphicsSystem` and warn if no plugin is loaded (`xboxkrnl_video.cpp:83`). In the native design these are only called from inside the D3D library, which the hooks bypass, so the default no-op virtuals are enough. `VdQueryVideoMode` still matters: the game calls it directly (FM4 `sub_8239FDC8`, `sub_826F29D0`).

### 1.3 Three ways to replace guest code

1. **Link-time weak override.** Every generated function is emitted as `DEFINE_REX_FUNC(sub_X)`, which on clang is `__attribute__((alias("__imp__sub_X"))) __attribute__((weak)) extern "C" REX_FUNC(sub_X)` (`FM4/generated/default/fm4_pch.h:75-78`). A strong `extern "C" REX_FUNC(sub_X)` anywhere in the title target wins, and `__imp__sub_X` keeps the original body for wrapping. FM4 already does this for `sub_826E8358` (`D3D::CBlocker::Check`) in `FM4/src/fm4_midasm_hooks.cpp:66`. Both FM4 presets compile with clang (`FM4/CMakePresets.json:61`), so the alias works.
2. **Runtime hook by address.** `FunctionDispatcher::SetFunction(uint32_t guest_address, PPCFunc*)` (`include/rex/system/function_dispatcher.h:47,85`). The native plugin receives the dispatcher in `SetupGuestGpu`, so an SDK-level plugin can install hooks from a title-supplied address table without any link tricks.
3. **Mid-asm hooks** via `[[midasm_hook]]` in `fm4_config.toml` (`src/codegen/config.cpp:311`), for splicing inside a function.

`REX_HOOK(sub_X, host_fn)` (`include/rex/hook.h:42`) gives the same auto-marshalled signature style as Unleashed's `GUEST_FUNCTION_HOOK`: plain C++ parameters, guest pointers translated through `HostToGuestFunction`.

### 1.4 Guest-visible allocation

`Memory::SystemHeapAlloc(size, alignment, flags)` (`src/system/xmemory.cpp:693`) is what the kernel uses for guest-visible objects (thread TLS, modules, kernel globals). It is the equivalent of Unleashed's `g_userHeap.AllocPhysical<T>()` for placing fake D3D objects where recompiled code can hold pointers to them.

### 1.5 Reusable graphics code in the SDK

- `src/graphics/pipeline/shader/`: Xenia's `DxbcShaderTranslator` and `SpirvShaderTranslator`, driven by `ShaderTranslator::TranslateAnalyzedShader`. These translate Xenos microcode to DXBC or SPIR-V at runtime. They are an alternative to XenosRecomp for the native path (section 5.4).
- `include/rex/graphics/pipeline/texture/{info,util,conversion}.h` plus the `texture_load_*` compute shaders under `src/graphics/shaders/`: tiled-texture untiling and format conversion for every Xenos texture format. Unleashed never needed this because Sonic Unleashed ships DDS; FM4 ships tiled 360 textures.
- `include/rex/graphics/xenos.h` and `register_table.inc`: the register bitfield definitions needed to decode the device's `GPU_*PACKET` shadows.
- `src/ui/presenter.cpp` with `RefreshGuestOutput(width, height, refresher)`: the D3D12 presenter hands the refresher a UAV-capable `ID3D12Resource` (`include/rex/ui/d3d12/d3d12_presenter.h:66`). Keeping the overlays alive would mean copying Plume's final frame into that resource across devices (shared NT handle); see section 5.7.

---

## 2. How UnleashedRecomp integrates Plume

Everything lives in `UnleashedRecomp/gpu/video.cpp` (7,882 lines) and `video.h`. Plume and XenosRecomp are submodules pinned at plume `11926860` and XenosRecomp `990d03b2`; both are uninitialised in this checkout, so the Plume API below is reconstructed from call sites.

### 2.1 Architecture

```
recompiled PPC code
  -> GUEST_FUNCTION_HOOK trampoline (kernel/function.h:351), args unpacked from r3..r10 / f1..f13
  -> host function on the GUEST thread (reads GuestDevice dirty flags, packs a RenderCommand)
  -> moodycamel::BlockingConcurrentQueue<RenderCommand> (video.cpp:1006)
  -> render thread (video.cpp:5249), the only thread touching Plume command lists
  -> FlushRenderStateForRenderThread(): hash PipelineState -> RenderPipeline -> plume::RenderCommandList
```

Two state machines: the guest-side one is the real 360 `D3DDevice` dirty-flag bitfields inside the fake device; the host-side one is `g_pipelineState` + `g_dirtyStates`, render-thread-local. The queue is the only bridge.

### 2.2 Plume surface used

Backends: D3D12 and Vulkan only. macOS goes Vulkan over MoltenVK. Selection (`video.cpp:1663-1805`): Wine forces Vulkan; a vector of factory functions is tried in preference order; the whole creation is wrapped in SEH and on a driver crash the process re-execs itself with `--graphics-api-retry` and the order swapped; D3D12 on old AMD drivers or any Intel is redirected to Vulkan.

Objects: `RenderInterface`, `RenderDevice`, one `RenderCommandQueue` plus a separate copy queue with its own fence for blocking uploads, `NUM_FRAMES = 2` command lists/fences/query pools, `RenderSwapChain` in `B8G8R8A8_UNORM`, per-frame acquire/render semaphores.

Binding model is bindless with four descriptor sets bound once per frame (`video.cpp:1888-1976`): sets 0, 1, 2 are the same 65,536-entry texture heap viewed as 2D, 3D and cube; set 3 is a 1,024-entry sampler heap. Slots 0, 1, 2 of the texture heap are permanent null textures with an all-zero component mapping. Constants go through three root CBVs on D3D12 (`b0`/`b1`/`b2` in `space4`) or a 24-byte push constant of three buffer device addresses on Vulkan, hidden behind `SetRootDescriptor` (`video.cpp:2887-2895`).

### 2.3 Which guest functions are hooked

Forty-two `GUEST_FUNCTION_HOOK`s and fifteen stubs (`video.cpp:7798-7883`). The hooked set is exactly the D3D9-X object API:

| group | entry points |
|---|---|
| device | `Direct3D_CreateDevice`, `D3DDevice_Present`, `GetBackBuffer` |
| resources | `CreateTexture`, `CreateVertexBuffer`, `CreateIndexBuffer`, `CreateSurface`, `CreateVertexDeclaration`, `CreateVertexShader`, `CreatePixelShader`, resource destructor, texture/VB/IB `Lock`/`Unlock`, `GetDesc` variants, `GetVertexDeclaration`, `HashVertexDeclaration` |
| state | `SetRenderTarget`, `SetDepthStencilSurface`, `SetViewport`, `SetScissorRect`, `SetTexture`, `SetVertexDeclaration`, `SetVertexShader`, `SetPixelShader`, `SetStreamSource`, `SetIndices` |
| draw | `DrawVertices`, `DrawIndexedVertices`, `DrawVerticesUP`, `Clear`, `Resolve` (as StretchRect) |
| game code | `D3DXFillTexture`, `MakePictureData`, `SetResolution`, `ScreenShaderInit` |

Absent on purpose: `SetRenderState`, `SetSamplerState`, `SetVertexShaderConstantF`, `SetPixelShaderConstantF`. Those are handled by the device-table trick and dirty-flag polling below. All `Vd*` kernel exports are stubs (`kernel/imports.cpp:794-939`); the ring buffer and EDRAM are never emulated.

### 2.4 The fake device and the function-table trick

`GuestDevice` (`video.h:34-66`) mirrors the real XDK `D3DDevice`; the offsets are load-bearing:

| offset | field | FM4 IDB name (identical layout) |
|---|---|---|
| 0x0000 | `dirtyFlags[8]` (u64, big-endian) | `m_Pending.m_Mask[5]` + `m_Predicated_PendingMask2` + `m_pRing*` |
| 0x0040 | `setRenderStateFunctions[0x65]` (guest fn ptrs, index = state/4) | `m_SetRenderStateCall[101]` |
| 0x01D4 | `setSamplerStateFunctions[0x14]` | `m_SetSamplerStateCall[20]` |
| 0x0480 | `samplerStates[0x20]` (24-byte raw Xenos fetch constants) | `m_Constants` first union (0x300 bytes) |
| 0x0780 | `vertexShaderFloatConstants[0x400]` (256 float4) | `m_Constants.Alu` |
| 0x1780 | `pixelShaderFloatConstants[0x400]` | `m_Constants.Alu` + 0x1000 |
| 0x2780 / 0x2790 | VS / PS bool constants | `m_Constants` third union (0x2300 + 0x480) |
| 0x2E2C | `vertexDeclaration` | past the 0x2B00 public struct: internal `CDevice` |
| 0x3168 | viewport (6 floats) | internal `CDevice` |
| 0x5E00 | total | FM4 allocates 0x6080 (`Direct3D_CreateDevice`, ida40 0x826E8AA0) |

The XDK inlines `D3DDevice_SetRenderState` as an indirect call through `m_SetRenderStateCall[State/4]`. `CreateDevice` (`video.cpp:2120-2138`) exploits that: it appends host trampolines to the end of the guest function table (`g_memory.InsertFunction`) and points 17 render-state slots at them; every other slot gets a no-op. Sampler-state slots are pointed back at the *original* guest functions, which write raw Xenos sampler registers into the device and set `dirtyFlags[3]`; the host decodes those bits later.

### 2.5 Guest object model

No COM. Each D3D object is a C++ struct allocated from the guest heap with `{ uint32 pad; be<uint32> refCount; ResourceType type; }` at the front so the game's own inlined refcounting works (`video.h:81-117`, CAS on the byte-swapped value). Subtypes carry Plume handles: `GuestBaseTexture` (`RenderTexture`, view, bindless descriptor index, tracked layout), `GuestTexture` (staging memory for Lock, shader-resolve framebuffer), `GuestBuffer` (`RenderBuffer` plus a big-endian staging copy), `GuestSurface` (framebuffer map, sample count, pending resolve destinations), `GuestVertexDeclaration` (hash, Plume input elements, `swappedTexcoords`, instance stream), `GuestShader` (cache entry, per-spec-constant linked variants). Destruction is deferred two frames (`video.cpp:670-737`).

### 2.6 Render state model

`PipelineState` (`video.cpp:122-150`, packed) is the PSO key: shader and declaration *pointers* (safe because both are interned), blend/depth/cull/topology, per-stream strides, RT and DS format, sample count, alpha-to-coverage, and a `specConstants` bitmask. `SanitizePipelineState` (`video.cpp:4011-4055`) canonicalises states that cannot affect output before hashing; without it the PSO count explodes. Lookup is `XXH3_64bits` over the struct into `g_pipelines`, render-thread only; async compiler threads hand results back via an `AddPipeline` command.

Host dirty flags (`DirtyStates`, `video.cpp:195-235`) are set only through `SetDirtyValue`, so redundant sets cost nothing. `FlushRenderStateForRenderThread` (`video.cpp:4457-4551`) is the single point where state becomes GPU commands.

Guest dirty flags are decoded on the guest thread before each draw (`FlushRenderStateForMainThread`, `video.cpp:4297-4360`): `dirtyFlags[0]` and `[1]` cover VS/PS float constants at one bit per four registers, with `countl_zero`/`countr_zero` giving the tight range to copy; `dirtyFlags[3]` bits 32..47 are samplers 0..15; `dirtyFlags[4]` bit 56 flags the bool constants. Samplers are decoded from the raw register words (`video.cpp:4367-4413`) and interned by hash of the `RenderSamplerDesc`.

Per draw, three constant blocks are uploaded (`video.cpp:4520-4536`): the whole 4 KB VS block and 3.5 KB PS block (byte-swapped in the copy), and a 276-byte `SharedConstants` block holding 16 texture-2D indices, 16 texture-3D indices, 16 cube indices, 16 sampler indices, packed booleans, `swappedTexcoords`, half-pixel offset and alpha threshold. One `SetTexture` writes all three index arrays, so a shader picks whichever dimension it declared and there is no pipeline variant per texture type.

### 2.7 Shaders

Shaders are identified by `XXH3_64bits` over the guest bytecode (`video.cpp:5057-5096`), looked up by binary search in a sorted `ShaderCacheEntry[]` generated by XenosRecomp into `shader/shader_cache.cpp` (`UnleashedRecompLib/shader/shader_cache.h`: hash, DXIL offset/size, SPIR-V offset/size, `specConstantsMask`). Both blobs are Zstd-compressed as a whole and the SPIR-V is additionally SMOL-V encoded. Only the chosen backend's blob is decompressed at startup. **A cache miss silently yields a shader-less `GuestShader`; there is no runtime translator.** The cache must be complete.

Specialisation: Vulkan uses real spec constants. D3D12 has none, so cached DXIL is compiled as `lib_6_3` with an unresolved `g_SpecConstants()` and linked at runtime by DXC against a tiny generated library (`video.cpp:3894-4006`), memoised per shader and mask. Observed masks: reverse-Z (viewport min > max, `video.cpp:3746`), R11G11B10 normals, alpha test.

Constant registers map 1:1 to `packoffset(cN)`; bools live in `SharedConstants.booleans`. The half-pixel offset is applied in the vertex shader from `SharedConstants` (`1/width`, `-1/height`, in clip space), recomputed whenever the framebuffer changes (`video.cpp:3653-3657`).

Vertex declarations (`video.cpp:4825-5008`) use a fixed semantic-to-location table shared with XenosRecomp, pad missing attributes with dummy elements on never-bound stream 15, declare FLOAT3 normals as `R32G32B32_UINT` so the shader byte-swaps them, flag DEC3N normals through a spec constant, and record 16-bit texcoord usage in `swappedTexcoords` because vertex buffers are byte-swapped in 32-bit units.

### 2.8 Resources

- Textures: no tiling or swizzle decode anywhere. The game's assets are DDS parsed with ddspp and uploaded linearly; `LockTextureRect` hands the guest a linear staging surface at a 256-byte pitch. Channel order is fixed with view component mappings, not data conversion. Depth formats collapse to `D32_FLOAT`; stencil is dropped.
- Buffers: byte-swapped on the CPU during unlock (`UnlockBuffer<T>`, `video.cpp:2225-2271`), vertex buffers always as `uint32`, index buffers at their real width, straight into a GPU-upload heap when ReBAR is available.
- Render targets: MSAA level is the user setting, not the game's request (`video.cpp:3193`). Framebuffers are cached per surface pair. Barriers are tracked by hand and flushed in one call.
- `Resolve` is deferred (`video.cpp:3265-3299`): the destination texture is *associated* with the surface, and for non-MSAA surfaces the sampler slot is simply pointed at the render target's own descriptor. A real copy or MSAA resolve only happens when the surface is about to be rendered into again, via hardware resolve or fullscreen `resolve_msaa_*`/`copy_*` shaders.
- Instancing: the 360's `POSITION1` instance-index stream is rebound as an `R32_UINT` index buffer and drawn with `drawIndexedInstanced`.
- Quads and (on D3D12) triangle fans are converted with generated index buffers.

### 2.9 Threading and presentation

One render thread drains the queue in bulk (32 at a time). `Present` runs on the guest thread, enqueues pending resolves, ImGui, and `ExecuteCommandList`, then blocks on a single futex until the render thread has submitted; it then presents, rotates frames, waits the fence of frame N-2, resets the per-frame allocators, and applies an optional FPS cap. `WaitOnSwapChain` (present-wait) is called at the top of the game's update loop for low latency. A thread-ownership guard routes buffer uploads from loading threads through the blocking copy queue.

The back buffer is a `GuestSurface` moved into guest memory at `CreateDevice`; each frame its `texture` pointer is re-pointed at the swapchain image, an intermediary render target, or a 1x1 fallback. Aspect ratio is a letterboxed sub-rect of the window written into the guest back-buffer size; internal resolution comes from hooking the game's own resolution setter; a final fullscreen pass applies the Xbox gamma curve and letterbox.

### 2.10 What is generic and what is Sonic-specific

Generic and worth porting: backend selection with crash-retry, the bindless layout, `SharedConstants`, the guest object model with deferred destruction, the device function-table trick, dirty-flag decoding, `PipelineState` plus `SanitizePipelineState`, the command queue and Present rendezvous, the byte-swapping upload allocator, declaration interning and location table, deferred Resolve with surface aliasing, MSAA shader-resolve fallback, reverse-Z detection, the format conversion tables, and the shader cache format with DXC linking.

Sonic-specific and not portable: every `sub_XXXXXXXX` and literal address, the `GuestDevice` padding beyond the public struct, shader-hash and texture-hash hacks, the async PSO precompiler that walks Hedgehog Engine databases, all post-process and shadow-map fixes, and the `patches/video_patches.cpp` file. Two generic problems were solved with game knowledge: depth surfaces are assumed transient at Present, and the AMD triangle-strip workaround is applied at asset load.

Known gaps for FM4 called out by the reference: no texture detiling, no stencil, only one render target per pipeline (`desc.renderTargetCount` is 0 or 1, `video.cpp:4076-4085`).

---

## 3. How re:Blue integrates Plume (on ReXGlue)

Repo `reblue` v1.0.0. Host SDK is ReXGlue 0.10.0 (`reblue_manifest.toml:2`, `generated/rexglue.cmake:18`). `CMakeLists.txt:184` calls `rexglue_setup_target(${target})` with **no** `GPU_PLUGINS`; the SDK's Xenia GPU layer is not loaded. Two executables come out of one configure: `reblue` (D3D12) and `reblue_vk` (Vulkan, MoltenVK on macOS); the backend is a compile-time constant in `src/gpu/backend.h`, which `#error`s if included from a common translation unit so that backend-specific code is a compile error rather than a link error.

### 3.1 Layout

About 60 files, 15,500 lines under `src/gpu/`:

| area | files | role |
|---|---|---|
| hooks | `gpu/hooks/{device,state,resource,draw,shader,output,tweaks,debug_overlay_basis}.cpp` | the D3D9 API surface, `REX_HOOK` / `REX_HOOK_RAW` / midasm |
| guest ABI | `gpu/d3d.h` | byte-exact XDK struct layouts recovered by SDK ordinal, every offset `static_assert`ed |
| device/frame | `device*.cpp`, `frame_ring.cpp`, `present.cpp`, `graveyard.cpp`, `deferred_destroy.h` | Plume device, 2-slot frame ring, fence-deferred destruction |
| draw | `draw.cpp`, `draw_bindings.cpp`, `draw_framebuffer.cpp`, `resolve.cpp` | state flush, framebuffer cache, EDRAM resolve emulation |
| resources | `host_heap.*`, `host_resource_heap.*`, `surface_pool.*`, `native_texture_mirror.*`, `texture_upload.*`, `physical_buffers.*`, `byte_swap.h`, `format.*`, `vertex_declaration.*`, `sampler_cache.*`, `bindless*.{cpp,h}`, `constant_buffers.*` | |
| pipeline | `pipeline/pipeline_state.h`, `pipeline_cache.*`, `pso_precache.*`, `pso_recorder.*`, `pso_predictor.*`, `pipeline/cache/*.h` | PSO key, cache, three-layer warm-up |
| shaders | `shaders/shader_cache.h`, `linked_shader_cache.h`, `guest_shaders.cpp`, `shader_linker.*`, `dxc_link.*`, `shaders/hlsl/*` | cache loader, DXC linking, 22 host HLSL files |

### 3.2 Guest hooking

Functions are named in `config/functions.toml` (`0x8246AC38 = { name = "D3DDevice_Swap", size = 0x620 }`) so hooks read `REX_HOOK(D3DDevice_CreateTexture, ...)`. `REX_HOOK_RAW` is used whenever the original must also run, because a typed import re-roots the guest stack. `src/core/hooks.h` adds only `BD_NOOP`/`BD_NOOP_RETURN` (silent stubs).

Hooked: `Direct3D_CreateDevice` (allocates `SystemHeapAlloc(0x5000, 0x100)`, zeroes it, copies the two dispatch tables from the XEX's static templates at `0x82751D68`/`0x827521F8`, seeds blend registers and viewport), `D3DDevice_Reset` (mirrors only the presentation-parameter writes), `Clear`, `Swap`, `SetViewport`, `SetRenderTarget` (index 0 only, MRT not modelled), `SetDepthStencilSurface`, `SetScissorRect`, `SetVertexShader`, `SetPixelShader`, `SetVertexDeclaration`, `SetTexture`, `SetStreamSource`, `SetIndices`, `CreateSurface`, `CreateTexture`, `CreateVertexBuffer`, `CreateIndexBuffer`, VB/IB `Lock`/`Unlock`, `D3DSurface_LockRect`, `D3DResource_Unlock/Release/AddRef/Destroy/GetType`, `D3DTexture_GetSurfaceLevel`, `DrawVertices`, `DrawVerticesUP`, `DrawIndexedVertices` (all five arguments must be marshalled or `IndexCount` is lost in r7), `BeginVertices`/`EndVertices` (an unhooked `BeginVertices` spins forever in `RingBufferFlush`), `Resolve` (12 arguments), `BeginTiling`/`EndTiling`, `CreateVertexDeclaration`, `CreateVertexShader`, `CreatePixelShader`, `D3DQuery_Issue/GetData`, plus Blue Dragon's own `bdSetRenderState`, `bdAllocRenderBuffer`, `bdPhysical*BufferCreate`, `hcg*ShaderCreateByHlsl`.

Stubbed: `SetRingBufferParameters`, `RingBufferAlloc/Flush/SubmitBatch/WaitForSpace/PollReady`, `SetShaderGPRAllocation`, `BlockUntilIdle`, `D3D__ResetAllState`, `D3D__SetSurfaceInfo`, `BeginZPass`/`EndZPass`, `SetScreenExtentQueryMode`, `SetGammaRamp`, `XGSurfaceSize`, `ReleaseThreadOwnership`. **The PM4 ring is never built or parsed.**

Deliberately not hooked: `SetSamplerState_*` (its recompiled body writes the fetch-constant table at device+0x400, which the host reads directly) and the constant setters (wrapped only to mark dirty; three engine functions that write the constant shadows directly are wrapped the same way).

### 3.3 Guest object model

`HostHeap` reserves one block in the guest physical window at `0xA0000000` and runs o1heap over it; `HostResourceHeap::Alloc<T>` places the host C++ object at a guest VA with the XDK header as its first bytes, then registers the VA. `FromGuest<T>` reverse-maps and rejects engine sentinels. `D3DResource_Release` decrements the in-struct big-endian refcount and at zero queues destruction into the frame slot's graveyard, drained after that slot's fence. Shaders and vertex declarations are never freed because the PSO cache keys on their pointers.

`d3d.h` is the single most valuable artefact: `D3DResource` (24 bytes), `D3DTexture` (52), `D3DSurface` (48, `SizeBits` encodes width/height), `D3DVertexBuffer`/`D3DIndexBuffer` (32, `FetchLo`/`FetchHi` with two encodings depending on whether `XGOffsetResourceAddress` has run), and a 0x5000-byte `D3DDevice` with the public prefix at the same offsets FM4 has (`m_Mask[5]`, dispatch tables, `fetchConstants[32]` at +0x400, VS/PS float constants at +0x700/+0x1700, bools at +0x2700, `rbBlendControl0` at +0x28B8, `rbColorControl` at +0x2D3C) followed by Blue Dragon-private slots (`pixelShader` +0x3080, `vertexShader` +0x3084, `indexBuffer` +0x4D38, resolution block +0x4DFC). Note Blue Dragon's public prefix is shifted 0x80 bytes down from FM4's because its dirty-mask array is 5 entries plus ring pointers packed differently; the members and their order are the same.

### 3.4 Render state: the hybrid model

This is the main architectural lesson. Per-draw state comes from three sources (`draw.cpp:33-117`):

1. Hooked setters for structural state (shaders, declaration, RT/DS, viewport, streams, indices, textures), written through `SetDirtyValue`.
2. **The Xenos register shadows inside the guest device**: `rbBlendControl0` and `rbColorControl` are decoded bit-field by bit-field into `srcBlend/blendOp/destBlend/srcBlendAlpha/blendOpAlpha/destBlendAlpha` and `alphaBlendEnable`, using `rex::graphics::xenos::{BlendFactor,BlendOp,CompareFunction,StencilOp}`. Rationale: the SDK folds `AlphaBlendEnable` and friends across many inlined call sites that a hook cannot see.
3. Blue Dragon's own engine-level render-state cache at `0x82DBE1A8` for depth, cull, fill, stencil and colour-write, because BD routes those through one `bdSetRenderState` chokepoint.

Sampler state is decoded from the 24-byte fetch constants at draw time (`sampler_cache.cpp:112-144`, via `xe_gpu_texture_fetch_t`), with a per-slot `memcmp` short-circuit.

`PipelineState` is a packed 156-byte struct (with full stencil state, fill mode and front face added relative to Unleashed), hashed with XXH3 after `SanitizePipelineState`; `GetOrCreatePipeline` unlocks around the build so precache workers compile in parallel. `BeginCommandList` force-dirties everything because IA, pipeline and root bindings do not survive `begin()`.

### 3.5 Shaders

Same `ShaderCacheEntry` format as Unleashed, generated by a heavily modified XenosRecomp fork (`zolaware/reblue-XenosRecomp`, 13 commits: structured control flow instead of a `switch(pc)` state machine, per-usage swap masks, 256-bit bool file, bilinear shadow compares, shared vertex-location table, `REBLUE_RECOMP` shared-constant layout). Offline, XenosRecomp scans `assets/**/*.{vso,pso,xex}` byte-by-byte for the `ShaderContainer` signature (`flags & 0xFFFFFF00 == 0x102A1100`) and hashes `virtualSize + physicalSize` bytes; the runtime key is computed identically. Cache miss: a warning, then the draw is dropped (no crash).

`reblue_prelink` (`tools/prelink_shader_cache.cpp`) enumerates every subset of each entry's `specConstantsMask` at build time and DXC-links the `lib_6_3` DXIL against a generated `g_SpecConstants()` for both `vs_6_0` and `ps_6_0` (exactly one succeeds, which identifies the stage), so D3D12 never links at runtime except on skew. Only two spec bits exist for BD: R11G11B10 normals and alpha test.

`SharedConstants` grew to 352 bytes: the four 16-entry index arrays, an 8-dword bool file, `swappedTexcoords`, half-pixel offset, alpha threshold, then per-usage swap masks for normals, binormals, tangents, blend weights and positions, `sintTexcoords`, a shadow PCF scale and a blit half-pixel offset. The vertex-location table is a shared `REBLUE_VERTEX_INPUT_LOCATIONS(X)` macro consumed by both the recompiler and the input-layout builder, with a `static_assert` cross-check; missing semantics get synthetic elements on slot 15.

VS/PS float constants are uploaded verbatim from device+0x700/+0x1700 with a fused byte-swap and NaN flush (Xenos `0*NaN = 0`, strict IEEE on the host does not).

### 3.6 Resources

- Two texture paths. `CreateTexture` textures are linear scratch surfaces filled through `LockRect`/`Unlock` (256-byte pitch, plain copy, no swap). Engine-allocated textures that carry a real Xenos fetch constant go through `native_texture_mirror.cpp`: `xe_gpu_texture_fetch_t` decode, `rex::graphics::FormatInfo`, `GetTiledOffset2D/3D`, `GetGuestTextureLayout`, `CopySwapBlock` per block, packed mip tails, cube and volume builders. This is the untiling FM4 will need and it is already written against the ReXGlue headers.
- Buffers: `ByteSwapElements` swaps 16-bit elements per element and everything else per dword (a `bswap32` over 16-bit indices gives out-of-range indices). Real scene geometry never goes through `CreateVertexBuffer`; reblue registers engine-owned physical blocks and makes each mesh a non-owning offset view into one block buffer. `GeometryHeapType` splits heaps by rewrite frequency because both single-heap choices hit a vendor floor on discrete AMD.
- Formats: `D24FS8/D24S8 -> D32_FLOAT_S8_UINT` (stencil kept), the 7e3 HDR EDRAM format stays float, unknown formats are refused rather than guessed.
- Render targets come from a fence-gated `SurfacePool`; framebuffers cached per (RT, DS) pair.
- EDRAM resolve: source is the bound surface only if it has been drawn since it was issued, else the last drawn surface for that slot; same-size resolves are aliased (destination sampler slot redirected to the source) and only materialised when either side is about to be overwritten; the exponent bias in the resolve flags becomes a scale in the copy shader; a composite-chain heuristic seeds fresh full-screen targets with the previous pass. `BeginTiling` becomes a clear (tile rects ignored) and `EndTiling` a resolve.
- MSAA and SSAA are boot-latched and mutually exclusive; everything shader-resolves; SSAA scales the scene surfaces at two midasm sites and the engine's own resolve downsamples.

### 3.7 Threading and presentation

**No render thread and no command queue.** Recording happens inline on whichever guest thread calls a hook, under one `std::mutex`. `kNumFrames = 2`; `Present` acquires, records the present pass (aspect-fit viewport, gamma-correction triangle, screenshot, overlay hook), submits with the acquire/render semaphores and the slot fence, presents, then waits the fence of the slot about to be *reused* and drains its graveyards. `frame_present_committed` guarantees one present per engine frame with the first `Clear` as the frame boundary. Shutdown is two-stage with a bounded, UI-pumping try-lock because the render thread may be parked waiting on the UI thread.

Frame pacing sleeps between presents even under vsync; a fixed 30 Hz logic accumulator (`engine/frame_clock.cpp`) plus camera interpolation (`engine/frame_interp.cpp`, ~30 midasm sites) provide unlocked FPS. The guest never sees the swapchain: a `GuestTexture` placeholder at the BD-believed back-buffer size stands in as implicit RT0, and the size the engine is told to render at is latched on first use.

Overlays: `gpu/imgui_overlay_drawer.*` draws the SDK's ImGui overlays inside the Plume present pass, which is how reblue keeps them without an SDK presenter.

### 3.8 Differences from Unleashed, and what is Blue Dragon-specific

| axis | Unleashed | re:Blue |
|---|---|---|
| threading | render thread + 29-type command union | inline recording under one mutex |
| guest structs | host invention with padding | byte-exact XDK layouts, asserted |
| hook naming | raw `sub_` addresses | symbolic via `functions.toml` |
| render state | `SetRenderState` chokepoint trampolines | register shadows + engine cache at draw time |
| samplers | hooked setters write host struct | fetch constants decoded at draw time |
| backend | runtime retry | compile-time, two executables |
| spec constants | 5 bits, runtime DXC link | 2 bits, prelinked at build |
| resolve | StretchRect command | lazy aliasing, exponent bias, chain seeding |
| PSO warm-up | precompile pass | compiled-in cache + load-time predictor + worker pool |

Generic and liftable: `backend.h`, `bindless_allocator.h`, `byte_swap.h`, `deferred_destroy.h`, `host_heap.*`, `host_resource_heap.*`, `frame_ring.cpp`, `graveyard.cpp`, `device_lost.cpp`, `dred.*`, `pipeline_state.h`, `pipeline_cache.cpp` (minus one PS-hash hack), `pso_precache.*`, `sampler_cache.*`, the untiling core of `native_texture_mirror.cpp`, `texture_upload.cpp`, the converters in `format.cpp`, `surface_pool.*`, `output.*`.

Blue Dragon-specific: every address in `config/`, the private `D3DDevice` tail offsets, `BdRenderStateCache`, the physical-buffer feeders, the `bdAllocRenderBuffer` mirror trigger and cube-atlas heuristic, the HLSL blit substitution, shader-hash hacks, `hooks/tweaks.cpp` and `hooks/output.cpp`, the composite-chain thresholds, `pso_predictor.cpp`, and the 2D design-canvas fit.

---

## 4. FM4's D3D layer as seen in IDA (ida40)

### 4.1 What the IDB already knows

The FM4 database carries the XDK's own symbols for the statically linked D3D library, which is why hooking is far cheaper than it was for Unleashed:

- About 60 public entry points named `D3DDevice_*`, `D3DResource_*`, `D3DTexture_*`, `D3DSurface_*`, `D3DVertexBuffer_*`, `D3DIndexBuffer_*`, `D3DQuery_*`, `D3DCommandBuffer_*`, plus the `XG*` texture-layout helpers (`XGTileTextureLevel`, `XGUntileTextureLevel`, `XGAddress2DTiledOffset`, `XGEndianSwapMemory`, ...).
- About 180 mangled internals: `D3D::SetPending_{Shaders,FetchConstants,AluConstants,RenderStates,Predicated,Split}`, `D3D::PM4Draw`, `D3D::PM4SetTextureFetchConstant`, `D3D::SetTextureHeader`, `D3D::SetSurfaceHeader`, `D3D::AllocateEdramMemory`, `D3D::CDevice::QueueIndirectBuffers`, `D3D::TrainEDRAM`, and so on.
- Typed structs: `D3DDevice` (0x2B00 bytes, the public prefix), `D3DConstants`, `D3DTAGCOLLECTION`, `GPU_CONTROLPACKET`, `GPU_DESTINATIONPACKET`, `GPU_PROGRAMPACKET`, `D3DBaseTexture` (52 bytes, `GPUTEXTURE_FETCH_CONSTANT` at 0x1C), `D3DVertexBuffer` (32, `GPUVERTEX_FETCH_CONSTANT` at 0x18), `D3DIndexBuffer` (32, `Address` at 0x18, `Size` at 0x1C), `D3DSurface` (48), `D3DVertexShader`/`D3DPixelShader`/`D3DVertexDeclaration` (24, header only).

Named during this session: `Direct3D_CreateDevice` (0x826E8AA0), `D3DDevice_Swap` (0x8237FA28), `D3DDevice_RunCommandBuffer` (0x82310818), `D3DDevice_BeginTiling` (0x82374660), `D3DDevice_DrawIndexedVertices` (0x82311080), `D3DDevice_DrawIndexedVerticesUP` (0x822D42A8). Uncertain identifications carry a `?.` prefix: `?.Gfx_RunCommandBufferOrRecord` (0x82310FC8, Forza engine wrapper), `?.TileBuffer_SwapImpl` (0x822AF9D8), `?.D3DDevice_BeginTiling_Wrapper` (0x823813B0), `?.D3D_InitializeRingBuffer` (0x826E4E28), `?.D3D_DestroyDevice` (0x826F48B8), `?.D3DDevice_DrawVertices_Core` (0x8233AC60).

### 4.2 Device layout

`D3DDevice` members with offsets (ida40 `type_inspect`):

| offset | member | use for a native renderer |
|---|---|---|
| 0x0000 | `m_Pending.m_Mask[5]` (u64) | dirty masks; bit layout matches Unleashed's `dirtyFlags[0..4]` |
| 0x0028 | `m_Predicated_PendingMask2` | tiling |
| 0x0030 | `m_pRing`, `m_pRingLimit`, `m_pRingGuarantee` | ring write cursor; unused once hooked |
| 0x0040 | `m_SetRenderStateCall[101]` | the function table Unleashed hijacks |
| 0x01D4 | `m_SetSamplerStateCall[20]` | sampler function table |
| 0x0224 | `m_GetRenderStateCall[101]`, `m_GetSamplerStateCall[20]` | leave to original code |
| 0x0480 | `m_Constants` (9,120 bytes): fetch/sampler constants, ALU float constants, bool/loop constants | read at draw time |
| 0x2820 | `m_ClipPlanes[6][4]` | |
| 0x2880 | `m_DestinationPacket`: `SurfaceInfo`, `Color0..3Info`, `DepthInfo`, `ScreenScissorTL/BR` | render-target formats and EDRAM bases |
| 0x28C0 | `m_WindowPacket` | window offset/scissor |
| 0x28CC | `m_ValuesPacket` | blend constant, alpha ref, depth bias, stencil ref |
| 0x2920 | `m_ProgramPacket`: `ProgramControl`, `ContextMisc`, `InterpolatorControl` | shader binding |
| 0x2934 | `m_ControlPacket`: `DepthControl`, `BlendControl0..3`, `ColorControl`, `HiControl`, `ClipControl`, `ModeControl`, `VteControl`, `EdramModeControl` | the fixed-function pipeline state |
| 0x2964 | `m_TessellatorPacket`, `m_MiscPacket`, `m_PointPacket` | |
| 0x2A70 | `m_MaxAnisotropy[26]`, `m_ZFilter[26]` | sampler clamps |
| 0x2B00 | end of public struct; `CDevice` continues to 0x6080 | ring, fences, tiling rects, XPS |

The internal region is what FM4's existing hot-path override reads: `kD3DRingPtrOffset = 0x2B10`, `kD3DCurrentTokenOffset = 0x2B08`, `kD3DFlagsOffset = 0x2B3D` (`FM4/src/fm4_midasm_hooks.cpp:30-34`).

`D3DDevice_SetRenderState_ZFunc` (0x822BB2A8) shows the pattern every render-state setter follows:

```c
pDevice->m_ControlPacket.DepthControl.dword = (16 * Value) & 0x70 | pDevice->m_ControlPacket.DepthControl.dword & 0xFFFFFF8F;
pDevice->m_Pending.m_Mask[2] |= 0x800;
pDevice->m_Pending.m_Mask[2] |= 0x20000;
```

It writes the Xenos `RB_DEPTHCONTROL` shadow and sets pending bits. Nothing goes to the ring until a draw. The same holds for `SetTexture` (writes the 24-byte fetch constant into `m_Constants` and sets `m_Pending.m_Mask[3]`) and `SetStreamSource` (writes the vertex fetch constant, stride table, and pending bits). This is the key design lever in section 5: the device *is* a register file, and a native renderer can read it at draw time instead of intercepting every setter.

### 4.3 Draw and submit path

`D3DDevice_DrawIndexedVertices(device, prim, base, start, count)` and `D3DDevice_RunCommandBuffer(device, cb, predication)` both call the `SetPending_*` family, which turns pending masks into PM4 `SET_CONSTANT` packets, then `PM4Draw` or, for command buffers, a chain of PM4 `INDIRECT_BUFFER` packets (`0xC0013F00` headers) built from the buffer's `SegmentCall` list, with `D3D::CDevice::QueueIndirectBuffers` for the split case.

Forza's engine wraps draws: `?.Gfx_RunCommandBufferOrRecord` (0x82310FC8) checks whether the engine is currently recording (`gfx+0x1EE4` non-null) and either nests the call into the recording growable command buffer (`sub_82872DE0`, `sub_82875458` -> `CGroupGrowableCommandBuffer::Create`) or calls `D3DDevice_RunCommandBuffer` on `*(gfx+0x14)`. The callers (`sub_822E3068`, `sub_82317600`, `sub_82310368`, `sub_823104D0`, `sub_823115B8`, `sub_828F1780`, `sub_828F1830`) are engine render-pass functions reached through vtables in `.rdata` (0x82007280, 0x8200728C). `D3DDevice_CreateCommandBuffer` is also called from the Kinect head-tracking code (`STEXEMPLAR::HeadDetector::CreateCommandBuffer`, `ST::ColorDepthAndRegistration::Process`), which can be stubbed.

Present: `TileBuffer::Swap(D3DTexture*)` (game) -> `?.TileBuffer_SwapImpl` (resets GPR allocation, then) -> `D3DDevice_Swap` (0x8237FA28), which calls `VdSwap`, `VdPersistDisplay`, `sub_82382A70` and `D3D::CBlocker::Check`. Device creation: `TileBuffer::CreateDevice(Direct3D*, D3DDevice*&, D3DPRESENT_PARAMETERS&, int, bool)` -> `Direct3D_CreateDevice` (allocates 0x6080 bytes via `D3D::MemAllocAligned`, `sub_826F4778` fills the function tables, `sub_826F4D00` -> `?.D3D_InitializeRingBuffer` -> `VdInitializeRingBuffer`/`VdEnableRingBufferRPtrWriteBack`).

Predicated tiling: `D3DDevice_BeginTiling(device, flags, count, rects, clearColor, clearZ, clearStencil)` stores up to N tile rects at `device+0x332C`, computes the union, optionally clears, records `PIXMetaRecordTilingInfo`, and enters tiling mode via `sub_82382A70(device, 1)`. Its wrapper `?.D3DDevice_BeginTiling_Wrapper` is called from four engine render-pass functions (`sub_822D78F0`, `sub_822D8130`, `sub_8243B2D0`, `sub_8243B5C0`), again vtable-dispatched. The `fm4.toml` note that the RTV path gives a black 3D scene while ROV works is consistent with tiled EDRAM use during gameplay.

Resource creation: `D3DDevice_CreateTexture` (0x826DF128) allocates the 52-byte header with `sub_823E4FE0` (the D3D memory allocator), fills it with `D3D::SetTextureHeader`, then allocates the texel memory (and the mip-tail block if any) with the same allocator and stores the physical addresses in fetch-constant words 1 and 5. The game also builds headers itself through `XGSetTextureHeader`/`XGSetVertexBufferHeader` on memory it owns, so a native renderer must handle resources whose backing memory it never saw allocated.

### 4.4 Differences from the Unleashed reference

| topic | Unleashed (2008 XDK, Hedgehog) | FM4 (2011 XDK, Turn 10) |
|---|---|---|
| public `D3DDevice` layout | reversed by hand, 0x5E00 total | typed in IDB, identical public prefix, 0x6080 total |
| render-state setters | mostly through the function table | table plus ~25 named direct `D3DDevice_SetRenderState_*` functions the compiler resolved statically |
| command buffers | not used | `D3DCommandBuffer_*` recording and `RunCommandBuffer` playback in engine passes; growable command buffers |
| tiling | not used | `BeginTiling` from engine passes |
| textures | DDS, linear | 360 tiled formats via `XG*` |
| multiple render targets | one | `m_DestinationPacket` has `Color0..3Info`; check usage |
| stencil | dropped | `SetRenderState_StencilEnable/StencilFail/StencilZFail/CCWStencilZFail/TwoSidedStencilMode` all present |
| queries | none | `D3DDevice_CreateQueryTiled`, `D3DQuery_GetData` (occlusion, tiled) |
| fences | trivial | `InsertFence`/`BlockOnFence`/`IsFencePending`, `D3D::CBlocker::Check` already natively overridden in FM4 |
| shaders | `shader.ar` archive plus XEX | to be located: likely inside the game's `.zip`/`.bin` asset bundles under `FM4/assets/Media` |

---

## 5. Design: a native renderer for Forza on ReXGlue

### 5.1 Packaging

Follow reblue, not the plugin ABI. The plugin ABI (`rex_gpu_create`) is a C boundary designed for a self-contained emulator; the native renderer needs the title's hooks to call into it constantly, so it belongs in the same link unit as the title.

- SDK side: a static library `rexnative` (`src/graphics_native/`, target `rex::gpu-native-lib`) holding the generic code lifted from reblue's `src/gpu/` (everything in the "generic and liftable" list in section 3.8), an `IGraphicsSystem` implementation whose `SetupPresentation` creates the Plume device and swapchain on the SDK window and whose `SetupGuestGpu` installs the title's hook table, and a `TitleProfile` struct the title fills in: private `D3DDevice` offsets beyond 0x2B00, the guest format enum table, the texture-format allow-list, and callbacks for engine-specific resource discovery.
- Title side: `FM4/src/gpu/` with the FM4 hooks (`REX_HOOK(D3DDevice_CreateTexture, ...)` against names added to `fm4_config.toml`), the FM4 profile, and the generated shader cache. `Fm4App::OnPreSetup` sets `config.graphics = std::make_unique<rex::native::GraphicsSystem>(profile)` when the `gpu_plugin` cvar is `native`, leaving `xenos` as the default so both paths stay selectable from `fm4.toml`.
- Plume: vendor the zolaware fork (SDL3 and fill-mode patches) under `thirdparty/plume`. It links volk, which this repo already has (`thirdparty/volk`, currently untracked).

### 5.2 Hook surface for FM4

Name these in `fm4_config.toml` `[functions]` and hook them; the addresses are all in ida40 today:

| group | functions |
|---|---|
| device | `Direct3D_CreateDevice` 0x826E8AA0, `D3DDevice_Release` 0x822D0C98, `D3DDevice_Swap` 0x8237FA28, `D3DDevice_Clear` 0x8236D840, `D3DDevice_AcquireThreadOwnership` 0x826E8600, `D3DDevice_BlockUntilIdle` 0x826E4D20, `D3DDevice_InsertFence` 0x822C5260, `D3DDevice_BlockOnFence` 0x822B0858, `D3DDevice_IsFencePending` 0x823102D0, `D3DDevice_InsertCallback` 0x822C2250, `D3DDevice_SetShaderGPRAllocation` 0x822BA060, `D3DDevice_SetScalerParameters` 0x826F3190 |
| resources | `CreateTexture` 0x826DF128, `CreateSurface` 0x826DF248, `CreateVertexBuffer` 0x826E7348, `CreateIndexBuffer` 0x826E7410, `CreateVertexShader` 0x826E7088, `CreatePixelShader` 0x826E6EA0, `CreateVertexDeclaration` 0x826E6B48, `CreateQueryTiled` 0x826EB2A8, `D3DResource_Release` 0x82386FB8, `AddRef` 0x82392748, `GetType` 0x822E5CB0, `BlockUntilNotBusy` 0x826E4768, `D3DVertexBuffer_Lock` 0x82386BF8, `D3DIndexBuffer_Lock` 0x826E8218, `D3DSurface_LockRect` 0x826DF418, `D3DSurface_GetDesc` 0x826DF380, `D3DTexture_GetSurfaceLevel` 0x826DEEC0, `D3DBaseTexture_LockTail` 0x826DEDD8, `D3DQuery_GetData` 0x8239B2C0, `D3DQuery_Release` 0x826E9B68 |
| state | `SetRenderTarget` 0x823563B8, `SetDepthStencilSurface` 0x822D9968, `SetViewport` 0x822FEAA8, `SetViewportF` 0x823925C0, `SetScissorRect` 0x822FF050, `SetTexture` 0x8233A9A8, `SetStreamSource` 0x823445A0, `SetIndices` 0x8236E3B8, `SetVertexShader` 0x82351A00, `SetPixelShader` 0x82351BD0, `SetVertexFetchConstant` 0x826E0D28, `FlushHiZStencil` 0x82381318 (no-op) |
| draw | `DrawIndexedVertices` 0x82311080, `DrawIndexedVerticesUP` 0x822D42A8, `BeginVertices` 0x8234D278, `?.DrawVertices_Core` 0x8233AC60 (confirm the public `DrawVertices`/`DrawVerticesUP` wrappers first), `Resolve` 0x822E2120, `BeginTiling` 0x82374660 and its `EndTiling` (locate via `sub_82382A70` callers), `RunCommandBuffer` 0x82310818 and the `D3DCommandBuffer_*` family |
| stubs | `D3DDevice_SetScreenExtentQueryMode`, `D3D__InvalidateAllGpuCaches`, `D3DDevice_NuiMetaData`, the XPS/low-priority command-buffer entry points, and the Kinect `STEXEMPLAR`/`ST` command-buffer users |

Do not hook the ~25 named `D3DDevice_SetRenderState_*` and `SetSamplerState_*` functions, the constant setters, or the dispatch-table variants. Let them run: they only write the register shadows and constant files in the guest device and set pending bits. The renderer reads that state at draw time (5.3). This is what makes inlined and table-dispatched setters equivalent, and it is exactly how reblue handles blend state.

Keep the existing `CBlocker::Check` override (`fm4_midasm_hooks.cpp`) but make it return "not blocked" when the native renderer owns the device: it reads the ring pointer and the current token, neither of which advance without the PM4 path. Fences become trivially complete, as in both references.

### 5.3 State model: hybrid, register-shadow driven

At every draw (`FlushRenderState`), read the guest `D3DDevice` directly:

| shadow | decode with | feeds |
|---|---|---|
| `m_ControlPacket.DepthControl` | `xenos::RB_DEPTHCONTROL` | z enable/write/func, stencil enable/func/ops (both faces) |
| `m_ControlPacket.BlendControl0..3` | `RB_BLENDCONTROL` | per-RT blend factors and ops |
| `m_ControlPacket.ColorControl` | `RB_COLORCONTROL` | alpha test enable/func, blend master |
| `m_ControlPacket.ModeControl` | `PA_SU_SC_MODE_CNTL` | cull, fill, poly offset enable |
| `m_ValuesPacket` | `RB_ALPHA_REF`, `RB_BLEND_RED..ALPHA`, `RB_STENCILREFMASK`, `PA_SU_POLY_OFFSET_*`, `RB_COLOR_MASK` | alpha ref, blend constant, stencil ref/mask, depth bias, colour write |
| `m_DestinationPacket` | `RB_SURFACE_INFO`, `RB_COLOR_INFO`, `RB_DEPTH_INFO` | RT/DS formats and MSAA, cross-checked against the hooked `SetRenderTarget` |
| `m_ProgramPacket` | `SQ_PROGRAM_CNTL`, `SQ_CONTEXT_MISC` | GPR split, param gen, sample-count hints |
| `m_Constants` 0x480..0x600 | `xe_gpu_texture_fetch_t` | texture fetch constants for samplers 0..15 at 24-byte stride (`SetTexture` writes `device + 0x480 + 24*sampler`) |
| `m_Constants` 0x6F0..0x780 | `xe_gpu_vertex_fetch_t` | vertex stream N at `0x778 - 8*N` (`SetStreamSource` writes `Fetch[26].dword[2*(17-N)]`) |
| `m_Constants` 0x780 / 0x1780 | byte-swap + NaN flush | VS / PS float constants |
| `m_Constants` 0x2780 / 0x2790 | | VS / PS bool constants |
| `m_Pending.m_Mask[0..4]` | Unleashed's bit layout | tight dirty ranges for constant uploads, sampler dirty bits |

The register definitions are already in `include/rex/graphics/registers.h` and `register_table.inc`. Unlike Blue Dragon, FM4 has no engine-level render-state cache to read, and it does not need one: the XDK shadows carry everything the fixed-function pipeline needs.

### 5.4 Shaders

Use reblue's XenosRecomp fork with an `FM4_RECOMP` shared-constants block and an FM4 vertex-location table. The Xenia translators in the SDK are not a shortcut: they produce DXBC against Xenia's binding model (shared memory buffer, its own constant layout), which would mean re-implementing the xenos plugin's data path in Plume.

Finding the bytecode is the first unknown. XenosRecomp scans raw files for the container signature, so it will find shaders embedded in the three XEXs but not shaders inside compressed Turn 10 bundles under `FM4/assets/Media`. Add a capture mode to the native `CreateVertexShader`/`CreatePixelShader` hooks (and to a `REX_HOOK_RAW` wrapper on the xenos path, which works today) that dumps each container to `FM4/shaders/<hash>.{vso,pso}`; feed that directory to XenosRecomp. A cache miss at runtime should warn and drop the draw, as reblue does, so the capture set converges over a few play sessions.

Keep the `lib_6_3` + prelink scheme for D3D12 and real spec constants for Vulkan. FM4 will need at least the alpha-test, R11G11B10-normal and reverse-Z bits (check FM4's viewport usage for min > max), and probably new per-usage swap masks for `BLENDINDICES`/`BLENDWEIGHT` and the extra `TEXCOORD` sets a racing renderer declares.

### 5.5 Resources

- Textures: `CreateTexture` hands out headers and texel memory the game fills through `LockRect`/`LockTail` or by DMA into memory it allocated itself; both end with a Xenos fetch constant in the header. Use reblue's `native_texture_mirror.cpp` path (decode the fetch constant, untile with the SDK helpers, upload) as the primary path, keyed on the header VA, and mirror on first `SetTexture` or on the guest's `Unlock`. The `XGSetTextureHeader` family is called by the game on memory the hooks never saw allocated; a VA-keyed registry with lazy adoption (reblue's `AdoptPhysicalBuffer` idea) covers that.
- Buffers: `CreateVertexBuffer`/`CreateIndexBuffer` in FM4 store physical addresses in `FetchLo`/`FetchHi` exactly like Blue Dragon's post-`XGOffset` encoding; byte-swap per element width on `Unlock`, and adopt buffers seen only through `SetStreamSource`/`SetIndices`.
- Render targets: `CreateSurface` -> surface pool. `m_DestinationPacket.Color1..3Info` decides whether FM4 uses MRT; if it does, `PipelineState` needs `renderTargetFormat[4]` and Plume's framebuffer creation takes multiple colour attachments (Unleashed and reblue both stop at one).
- Stencil: keep `D32_FLOAT_S8_UINT` as reblue does; FM4 has the full stencil setter family.
- Resolve and tiling: lift reblue's `resolve.cpp` wholesale. For `BeginTiling`, record the rects and flags, honour the clear, and render the frame once (predicated tiling issues each draw once; the GPU replays per tile). `EndTiling` resolves the whole surface. Expect the composite-chain heuristics to need retuning for FM4's post chain.

### 5.6 Command buffers

FM4's engine records draws into `D3DCommandBuffer` objects and replays them with `RunCommandBuffer`; nested recording goes through `CGroupGrowableCommandBuffer`. Emulate at the hook layer:

1. Locate `D3DDevice_BeginCommandBuffer`/`EndCommandBuffer` (unnamed; `sub_8237CC08` and `sub_822D78F0` are candidates since they call `CreateCommandBuffer` and the segmented run path).
2. While a command buffer is being recorded, every hooked state/draw call appends a host `RecordedCommand` (with the current register-shadow snapshot for draws) instead of executing.
3. `RunCommandBuffer` replays the host list through the normal flush path with the predication flag ignored.
4. `D3DCommandBuffer_SetTexture/SetVertexBuffer/SetIndexBuffer/SetVertexShader/SetPixelShader/SetSurfaces/SetViewport/SetClipRect` are dynamic fixups; store the recorded binding by fixup handle and let these overwrite it before replay. `BeginDynamicFixups`, `Deconstruct`, `BeginReconstruction`, `GetClone`/`CreateClone` need the same treatment.

Measure before building: a counting `REX_HOOK_RAW` on `RunCommandBuffer` and `DrawIndexedVertices` under the xenos path tells how much of a frame goes through command buffers.

### 5.7 Threading and presentation

Take reblue's inline-recording design (one mutex, 2-slot frame ring, per-slot graveyards). FM4's `Fm4GpuHangCheck` and the GPU wait predicate become no-ops under the native path.

Presentation: Plume owns the swapchain on `GetNativeWindowHandle()`. The guest back buffer is a placeholder `GuestTexture`; `D3DDevice_Swap(device, frontBuffer, params)` presents the front-buffer texture the game passes. Aspect-fit and gamma come from reblue's `present.cpp` and `output.cpp`. To keep the F2/F3/F4 overlays, port reblue's `imgui_overlay_drawer` into the present pass rather than trying to share Plume's frame with the SDK presenter across devices.

### 5.8 Build

- `thirdparty/plume` (zolaware fork) and `thirdparty/XenosRecomp` (reblue fork) as submodules.
- `cmake/shader_cache.cmake` and `cmake/shaders.cmake` from reblue: `reblue_shader_cache()` -> `rex_native_shader_cache()`, host HLSL via `dxc -T ... -HV 2021 -all-resources-bound` with `-spirv -fvk-use-dx-layout` (`-fvk-invert-y` on vertex shaders) or DXIL, emitted as `-Fh` headers; the prelinker runs at build time on D3D12 targets; `dxcompiler.dll`/`dxil.dll` staged beside the executable (`thirdparty/dxc` already exists here).
- The generated cache and recompiled guest code go in OBJECT libraries, never STATIC, or the `REX_HOOK` registration symbols are dropped.

---

## 6. Risks and unknowns

Measured on 2026-09-02 with the milestone-1 trace (`fm4_d3d_trace`, xenos path, title screen only, per 300 presented frames): `drawIdx` 9052 to 9160, `runCB` 0, `beginTiling` 600, `rtIdxNon0` 2700, `resolveFlags` 0xFC000300, `createTex` steady, 150 distinct shader containers captured. So at the title screen there is no command-buffer playback yet, predicated tiling runs twice per frame, and render-target indices other than 0 are set nine times per frame (MRT is in use). A race has not been traced yet.

Also learned while bringing up milestone 1: the library's secondary command arena is a circular allocator whose only GPU dependencies are `CDevice::BlockOnSecondaryPosition` and `CBlocker::Check`; with those returning immediately the real ring can be left in place (no host sink is needed, and a host sink is actively harmful because `RingBufferDeviceAllocate` carves data blocks at `m_pRingLimit + 68` bounded by the arena's own end pointer).

1. **Shader availability.** Whether XenosRecomp can see FM4's shaders offline depends on the bundle format under `FM4/assets/Media`. The capture-mode fallback in 5.4 removes the dependency but needs play coverage.
2. **Command-buffer share of the frame.** If most draws are recorded and replayed, 5.6 is on the critical path for first pixels, not a later phase. Measure first.
3. **Tiling semantics.** Rendering once at full size ignores `D3DTILING_FIRST_TILE_INHERITS_DEPTH_BUFFER` and any engine logic that depends on per-tile resolve order. reblue ignores tile rects and ships; FM4 is a heavier tiling user.
4. **MRT and the 7e3 HDR format.** Decide from `Color1..3Info` usage and `RB_COLOR_INFO` formats during the trace. Both references stop at one colour target.
5. **XPS and low-priority command buffers.** FM4's D3D has the procedural-synthesis worker machinery (`D3D::DispatchWorker`, `ExecuteLowPriCommandBuffer`, `CXpsGlobal`). If the game uses XPS, those CPU workers write PM4 directly and need their own stubs or emulation.
6. **Occlusion queries.** `CreateQueryTiled`/`D3DQuery_GetData` exist; reblue answers them with a counting pixel shader. Sun visibility and lens flares in FM4 may depend on real counts.
7. **Float-argument marshalling.** The Xenon ABI reserves a GPR slot per float argument, so `Clear`, `BeginTiling` and `Resolve` hooks need placeholder parameters or the later arguments land in the wrong registers. Silent and confusing when wrong.
8. **Resources created outside the hooks.** Any FM4 code path that builds headers with `XGSet*Header` on its own memory bypasses `Create*`; the VA registry with lazy adoption must be complete or those meshes render black.
9. **`D3DDevice` private tail.** Offsets past 0x2B00 (ring, current token, flags, tile rects at 0x332C, XPS state) are FM4-specific and are only partially known; the public prefix is not enough for `Swap`, `BeginTiling` and the blocker.

---

## 7. Phased plan

1. **Trace under the xenos path (no renderer changes).** Add counting `REX_HOOK_RAW` wrappers for `RunCommandBuffer`, `DrawIndexedVertices`, `BeginTiling`, `SetRenderTarget` (log `Color1..3Info`), `Resolve` (log flags and formats), `CreateTexture` (log formats), and shader creation with bytecode capture to disk. One race, one menu session. This settles risks 1, 2, 4 and sizes 5.6.
2. **Lift reblue's generic GPU code into the SDK** as `rexnative` with a `TitleProfile`, keeping reblue building against it as the regression test.
3. **FM4 profile and hooks:** names in `fm4_config.toml`, fake device with the FM4 tail offsets, resource registry, present path, register-shadow state flush. Goal: the 2D menus and the loading screen render through Plume.
4. **Shader cache** from the captured set through the XenosRecomp fork with `FM4_RECOMP`, prelinked; cache-miss logging feeds back into the capture set.
5. **Command buffers and tiling** per 5.6 and 5.5.
6. **In-race quality:** resolve/composite tuning, MSAA/SSAA, occlusion queries, overlays, PSO warm-up.

Names assigned in ida40 during this session are listed in section 4.1; `?.`-prefixed names are unconfirmed and should be verified before hooks depend on them.
