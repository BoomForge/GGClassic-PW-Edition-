#!/usr/bin/env python3
"""Pixelwolf GGC Model Importer 2: convert GLB 2.0 to GameGuru's existing FBX path.

Runs INSIDE an installed Blender Python environment, not in system Python:
  blender --background --factory-startup --python pw_glb_to_fbx.py -- input.glb output.fbx log.txt

Input GLB remains untouched. FBX and extracted PNG textures live in an isolated
GameGuru importer/temp/pw_glb_* folder. No bundled Blender, assimp, MD3 baking,
generic rig retargeting or claimed lossless PBR/animation conversion.
"""
from __future__ import annotations

import re
import sys
import traceback
from pathlib import Path


def safe_name(value: str, fallback: str) -> str:
    """Keep output texture names local and deterministic, never asset-owned paths."""
    result = re.sub(r"[^a-zA-Z0-9_.-]", "_", value.strip())[:60]
    return result or fallback


def convert(input_file: Path, output_file: Path) -> str:
    import bpy  # Only available inside Blender.

    if input_file.suffix.lower() != ".glb" or not input_file.is_file():
        raise ValueError("Input must be an existing binary glTF 2.0 .glb file.")
    output_file.parent.mkdir(parents=True, exist_ok=True)
    # No user projects, scenes or saved startup add-ons should affect import.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    result = bpy.ops.import_scene.gltf(filepath=str(input_file))
    if "FINISHED" not in result:
        raise RuntimeError("Blender glTF importer did not complete.")
    objects = list(bpy.data.objects)
    meshes = [obj for obj in objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError("GLB contains no importable mesh objects.")

    # GLB images are often embedded/packed. Export them as standalone PNGs
    # before FBX export, so the legacy GameGuru FBX material resolver can find
    # real files by absolute path (not a stale artist's computer path).
    written = []
    failures = []
    images = list(bpy.data.images)
    for index, img in enumerate(images):
        if img.type != "IMAGE" or img.source not in {"FILE", "GENERATED"}:
            continue
        if not img.has_data:
            failures.append(f"Image '{img.name}' has no pixel data.")
            continue
        name = safe_name(img.name, f"image_{index}")
        target = output_file.parent / f"pw_glb_tex_{index:04d}_{name}.png"
        try:
            img.file_format = "PNG"
            img.filepath_raw = str(target)
            img.save()
            if not target.is_file() or target.stat().st_size == 0:
                raise RuntimeError("no output bytes")
            written.append(str(target))
        except Exception as exc:
            failures.append(f"Image '{img.name}' could not be extracted: {exc}")

    # Full-material PBR and per-clip rig animation compatibility still need
    # actual GameGuru testing; the conversion is a real FBX intermediate, not
    # an MD3 vertex-animation bake. Keep armatures and animation channels.
    result = bpy.ops.export_scene.fbx(
        filepath=str(output_file),
        use_selection=False,
        object_types={"MESH", "ARMATURE", "EMPTY"},
        path_mode="ABSOLUTE",
        embed_textures=False,
        add_leaf_bones=False,
        apply_unit_scale=True,
        bake_anim=True,
        bake_anim_use_all_actions=True,
    )
    if "FINISHED" not in result or not output_file.is_file():
        raise RuntimeError("Blender FBX export failed.")
    if output_file.stat().st_size < 64:
        raise RuntimeError("Blender exported an empty FBX.")

    details = [
        f"Input: {input_file}",
        f"FBX: {output_file}",
        f"Mesh objects: {len(meshes)}",
        f"Armatures: {sum(obj.type == 'ARMATURE' for obj in objects)}",
        f"Exported PNG textures: {len(written)}",
    ]
    details.extend(f"WARNING: {problem}" for problem in failures)
    return "\n".join(details)


def main(argv: list[str]) -> int:
    # Standalone CI can check syntax and argument handling without Blender.
    if "--self-test" in argv:
        assert safe_name(r"..\\Avatar face#1", "image") == "..__Avatar_face_1"
        assert safe_name("***", "fallback") == "___"
        print("PW GLB Blender bridge Python argument/sanitizer check: PASS")
        return 0
    if "--" not in argv:
        print("Expected: blender --background --python pw_glb_to_fbx.py -- input.glb output.fbx log.txt",
              file=sys.stderr)
        return 2
    positional = argv[argv.index("--") + 1:]
    if len(positional) != 3:
        print("Expected exactly input GLB, output FBX and diagnostic log.", file=sys.stderr)
        return 2
    source, destination, logfile = [Path(item).resolve() for item in positional]
    try:
        summary = convert(source, destination)
        logfile.write_text("SUCCESS\n" + summary + "\n", encoding="utf-8")
        print(summary)
        return 0
    except Exception:
        log = "ERROR: GLB conversion did not complete.\n" + traceback.format_exc()
        try:
            logfile.parent.mkdir(parents=True, exist_ok=True)
            logfile.write_text(log, encoding="utf-8")
        except OSError:
            pass
        print(log, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
