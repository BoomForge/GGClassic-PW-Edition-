"""Offline tests for the PW Edition release-packaging contract.

Run from the repository root:
    python -m unittest discover -s tools -p "test_make_pw_release.py"
"""
import hashlib
import json
import tempfile
import unittest
import zipfile
from pathlib import Path

from make_pw_release import ASSET, check_name, make_release


class ReleasePackagingTests(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.root = Path(self.dir.name)
        self.build = self.root / "build"
        self.build.mkdir()
        (self.build / "Guru-MapEditor.exe").write_bytes(b"synthetic-editor-for-package-test")

    def test_zip_contains_only_whitelisted_runtime_files(self):
        output = self.root / "dist"
        archive, sidecar = make_release(self.build, "0.1.0", output, [])
        self.assertEqual(archive.name, ASSET)
        expected_sha = hashlib.sha256(archive.read_bytes()).hexdigest()
        self.assertEqual(sidecar.read_text().strip(), expected_sha)
        with zipfile.ZipFile(archive) as zipped:
            paths = set(zipped.namelist())
            self.assertEqual(paths, {
                "pw-release.json", "pw-edition-version.txt",
                "pw-updater.ps1", "Guru-MapEditor.exe"
            })
            manifest = json.loads(zipped.read("pw-release.json"))
            self.assertEqual(manifest["version"], "0.1.0")
            self.assertEqual(zipped.read("pw-edition-version.txt"), b"0.1.0\n")
            for item in manifest["files"]:
                self.assertEqual(
                    hashlib.sha256(zipped.read(item["path"])).hexdigest(),
                    item["sha256"],
                )

    def test_rejects_user_projects_and_directory_traversal(self):
        for name in ("Files/entitybank/mymodel.fpe", "myLevel.fpm",
                     "../Guru-MapEditor.exe", "/Guru-MapEditor.exe",
                     "Files/terrainbank/snow.dds", "settings.ini",
                     "Files/effectbank/reloaded/../something.fx"):
            with self.subTest(path=name):
                with self.assertRaises(ValueError):
                    check_name(name)

    def test_accepts_opted_in_effect_but_not_unrequested_dll(self):
        effect = self.build / "Files" / "effectbank" / "reloaded" / "pw_effect.fx"
        effect.parent.mkdir(parents=True)
        effect.write_text("// synthetic FX")
        (self.build / "testplugin.dll").write_bytes(b"synthetic-dll")
        archive, _ = make_release(
            self.build, "0.1.1", self.root / "out",
            ["Files/effectbank/reloaded/pw_effect.fx"],
        )
        with zipfile.ZipFile(archive) as zipped:
            self.assertIn("Files/effectbank/reloaded/pw_effect.fx", zipped.namelist())
            self.assertNotIn("testplugin.dll", zipped.namelist())

    def test_contextual_runtime_scripts_are_explicitly_allowed(self):
        paths = ["Files/scriptbank/" + name for name in
                 ("global.lua", "pw_transition_state.lua", "gameplayercontrol.lua", "key.lua", "weapon.lua")]
        for name in paths:
            target = self.build / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text("-- synthetic script")
        archive, _ = make_release(self.build, "0.1.2", self.root / "scripts", paths)
        with zipfile.ZipFile(archive) as zipped:
            for name in paths:
                self.assertIn(name, zipped.namelist())
        with self.assertRaises(ValueError):
            check_name("Files/scriptbank/custom_player_project.lua")

    def test_rejects_bad_version_and_missing_executable(self):
        with self.assertRaises(ValueError):
            make_release(self.build, "nightly", self.root / "bad", [])
        (self.build / "Guru-MapEditor.exe").unlink()
        with self.assertRaises(FileNotFoundError):
            make_release(self.build, "0.1.2", self.root / "bad", [])


if __name__ == "__main__":
    unittest.main()
