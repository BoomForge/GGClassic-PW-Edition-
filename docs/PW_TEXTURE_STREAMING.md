# Texture streaming and bounded memory management

## M1 — resource ownership audit and live diagnostics (26 November 2026)

Source implementation through 53f794f7. No texture residency behaviour has been changed. The measured Windows baseline and current full Windows compilation are still outstanding.

### Actual architecture
- Shared/Image/CImageC.cpp owns a process-wide m_List (image ID to tagImgData). Mesh and UI callers can retrieve the raw resource and shader-resource-view pointers; removing an image ID alone cannot be assumed safe for every material or render binding.
- Common-Images.cpp has separate high-level filename banks for images and textures. Some lookups are case-sensitive, one is case-insensitive, and the DDS/PNG fallback can resolve distinct requested strings to one physical file. Existing cache keys also do not consistently distinguish loader mode, quality and texture divide. Do not globally collapse image IDs until usage/compatibility is understood.
- The image SDK's FindInternalImage scans names across the registry; LoadImageCoreInternal also consults the high-level texture bank. Embedded DBO texture callers can therefore share entries. Character Creator has a deliberate ID exclusion.
- DDS fast path creates the full supplied mip chain using DirectX::CreateDDSTextureFromFile. Reduced-quality path currently decodes, sometimes decompresses, resizes and recompresses before upload; PNG and unusual DDS paths have separate fallback behavior. This is load-time size reduction, not streaming.
- An existing preload worker uses g_image_list and g_image_outputv and builds GPU textures via PreloadThreadSafeImage. It is batch-prefetch, not a budgeted residency scheduler; audit transfer/cancellation and COM ownership before reuse. In particular, compare the pointer transfer across image_preload_files_finish, image_thread_function and image_preload_files_reset.
- RemoveImage releases lpTextureView in DX11 whereas ImageDestructorD3D releases both lpTextureView and lpTexture. Audit actual AddRef/Release paths, mesh/shader consumers and transferred preload resources before making any ownership change. A leak is plausible but not proven by source asymmetry alone.
- game_freelevel tears down entities, physics, weapons, grass, projectiles and other resources. Standalone then deletes entities and lightmaps, clears lightmap textures, and conditionally clears entitybank textures when standalonefreememorybetweenlevels is enabled. Editor Test Level intentionally retains editor assets; its cleanup cannot simply purge the registry.

### Enable and collect
In PW Edition menu choose **Capture Texture Memory (CSV)**, then load a test map and enter/exit Test Level. Standalone: set GGCPW_PROFILE_TEXTURES=1 in the environment before launch. Capture off by default. Output: %TEMP%\\GGCPW-texturetrace-<pid>.csv. Capture includes immediate editor toggle, level_init_begin, level_ready, gameplay (at most once per five seconds), level_cleanup_begin and level_cleanup_end. The existing load diagnostics CSV can be captured alongside it for timing.

- Estimated registered texture bytes: actual DX11 resource dimensions, mip levels, format slice pitch (DirectXTex ComputePitch), array faces, volume depth, samples, deduplicated by resource pointer. Separately counts staging bytes and aliases. Unknown entries are counted, not fabricated. This is logical allocation size, not exact physical VRAM, driver padding, or live residency. Observed peak is sampled, not an allocation-time high-water mark.
- DXGI adapter3 node zero local/non-local process usage and budget are measured on the actual renderer adapter when available, with validity flags. Local memory may be shared on UMA systems. It includes resources outside the registry.
- Windows working set, private committed bytes, peak working set; process-valid flag. Does not include per-category CPU staging or unregistered preload allocations as texture estimates.
- The diagnostic samples the registry on the game thread, never calls the cached-access getters, never deletes or rebinds resources. It does not scan the worker's mutable queue. No budget enforcement, unloading or streaming is claimed.

### Benchmark and acceptance
Use same Windows GPU, resolution, quality and identical assets. Capture empty map, small populated map, texture-heavy map, three consecutive Test Level runs, two standalone level transitions, five minutes of steady movement. Compare level_ready, cleanup_end, steady samples, sampled peak, actual DXGI usage/budget and process private bytes. Retained editor assets are not automatically leaks; compare deltas across repeated identical runs. Record unavailable DXGI telemetry separately. Test mixed DDS/PNG, sky/cube, terrain, grass, lightmaps, HUD, asset preview, weapons, Character Creator and exported games. Attach traces and frame-time/load timing. No baseline numbers invented.

### Remaining implementation order
1. Verify M1 full Windows build and collect actual traces; classify registry versus non-registry growth and prove resource ownership at deletion, preload transfer and renderer bindings.
2. M2: fix verified ownership/cache defects; add canonical resolved path + decode parameters and reference-safe lifecycle, protect UI/terrain/grass/lightmaps/render targets and editor assets.
3. M3: configurable soft budgets and deterministic residency planner with tests, pinned resources and eviction/reload policy; do not evict active raw pointers.
4. M4: bounded background file I/O/decode, DDS mip-tail/low-res fallback, render-thread upload/swap queue with cancellation, generation IDs and safe rebinding. Keep full-load fallback and PNG handling.
5. M5: visibility/distance-driven mip promotion, hysteresis, prefetch, eviction, device reset and measured RAM/VRAM/load-time/frame-time comparisons. Full editor/standalone runtime acceptance before default enablement.

Native third-person source remains intact; lifecycle M1/M2 builds and actual licensed gameplay acceptance remain independent gates. No release/master merge.
