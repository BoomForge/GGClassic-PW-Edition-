# PW Edition Lua Studio - first native implementation

**Status:** Source committed; full-engine compilation, in-editor interaction and file IO are **not yet verified**.

Open via GameGuru's **Edit > Lua Script Editor...**. This creates a modeless native RichEdit 4.1 window in the existing engine process. No external editor/EXE or downloaded extension is necessary.

Features:
- Editable Lua text with lexical syntax colours: Lua keywords, strings, comments, numbers, commands found in installed scripts.
- Ctrl+Space completes the first command matching the token before the caret. Typing suggests matching functions in the right-hand list. Search the library manually and double-click a function to insert its name.
- The command library parses **real signatures** from Files/scriptbank/global.lua and pw_transition_state.lua. The index does not include all native runtime commands or full semantic documentation. It avoids inventing unsupported APIs.
- Open/New/Save/Save As, Ctrl+O, Ctrl+S, Ctrl+Shift+S, modified-file prompts.
- UTF-8 only: invalid legacy encodings are rejected on open rather than silently converted. Existing scripts are backed up with a .pw-backup timestamp suffix before replacement. No auto-execute or hot reload.
- Files over 1 MB are refused; lexical highlighting is skipped for buffers over 250 KB.

**Limitations:** The tool is modeless but not an ImGui docking pane. Long-bracket strings with equals signs and full Lua semantic diagnostics are not yet implemented; autocomplete is prefix-based, not AI/LSP-based. Script commands are listed from installed Lua functions, not an exhaustively curated native API reference.

**Deferred Windows checks:** Build VS2022 Release x64; open and move/focus the editor; verify UTF-8 save/reopen, all syntax colours, Ctrl+Space, list insertion, unsaved changes, scriptbank discovery under Steam, and that the engine stays open after closing this tool window.

## EFPSE Model Bridge investigation

The archived Model Bridge README confirms FBX/GLTF/GLB loading and animation clip playback through Three.js, and **baking skeletal/morph poses into MD3 vertex frames** for Easy FPS Editor. EFPSE Studio's connected C# integration launches an external Model Bridge executable and reviews its MD3/PNG output. Its conversion source has not been located in the connected EFPSE GitHub repository.

The concept is valuable for FBX/GLB preprocessing, multi-material texture mapping and animation bake previews, but an MD3 vertex animation is **not** a GameGuru skeleton or a Mixamo-to-GameGuru bone retarget. Do not transplant the MD3 exporter as if it preserves rigging. Next: recover actual Model Bridge source ZIP, evaluate Three.js loaders and animation sampling code under licence, then design a non-destructive skeletal FBX import/retarget path, optionally providing a baked fallback.
