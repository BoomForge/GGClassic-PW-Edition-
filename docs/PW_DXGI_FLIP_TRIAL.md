# Experimental DirectX 11 flip-model presentation

Status: **source implemented, off by default**. The full Windows x64 compiler job validates this code; real in-editor/game GPU timing and regression tests must still be performed.

## Why

The inherited main DX11 chain in `CGfxC.cpp` used one buffer and `DXGI_SWAP_EFFECT_DISCARD`. Ivan Ertlov's 14 November 2026 investigation into the Robert D. Anderson runtime highlighted 20+ ms waits in `Present()` on some frames, and suggested modern DXGI flip presentation. See Microsoft: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model and https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect.

Caveat: time spent in `Present()` can be GPU completion, driver scheduling or vsync waits, rather than the presentation mechanism itself. There is **no measured PW Edition FPS gain yet**.

## Source change

In `GameGuru Core/Dark Basic Public Shared/Dark Basic Pro SDK/Shared/Setup/CGfxC.cpp`:

- Only when the variable `GGCPW_DXGI_FLIP=1` is present at device creation, the main chain is windowed and vsync is initially enabled, request three buffers, `DXGI_SWAP_EFFECT_FLIP_DISCARD` and the already single-sample DX11 settings.
- If flip swap-chain creation fails, release the partial device and retry the legacy single-buffer blt model.
- After a **successful real** `Present()` on main chain 0, restore `m_pRenderTargetView` and `m_pDepthStencilView` to the immediate context. No restore occurs on `DXGI_PRESENT_TEST` or secondary swap chains.
- Skip `SetFullscreenState` for the windowed flip chain; legacy fullscreen remains unchanged.
- Legacy behavior is the default and stays available simply by leaving the environment variable unset. A new process is required to change modes.

These are deliberate guards rather than a global swap-effect edit. No general multisampled back-buffer support was added. Windows 10 or later is required for `FLIP_DISCARD`; unsupported systems fall back.

## How to compare

Open a CMD in your **test** installation folder (not an important project). For legacy behavior:

    set GGCPW_DXGI_FLIP=
    Guru-MapEditor.exe

For the opt-in test, close the editor, then:

    set GGCPW_DXGI_FLIP=1
    Guru-MapEditor.exe

Repeat the same scene, resolution and VSync settings under both modes. Record CPU/GPU frame time, 1% lows, PresentMon statistics and frame pacing, not only averaged FPS. Test Test Level, standalone, Alt+Tab, desktop borderless/windowed, resize, minimize/restore, shaders, capture/overlay software, UI widgets, external monitors and repeated level changes. Do not declare a performance gain until results are repeatable. **Leave this disabled for normal use for now.**
