"""Offline, synthetic Steam overlay packaging and installation tests."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from install_pw_steam_overlay import allowed, apply_overlay, verify_package
from make_pw_steam_overlay import make_overlay, COMPANIONS

class OverlayTests(unittest.TestCase):
    def setUp(self):
        t = tempfile.TemporaryDirectory()
        self.addCleanup(t.cleanup)
        self.root = Path(t.name)
        self.build = self.root / "build"
        self.build.mkdir()
        (self.build / "Guru-MapEditor.exe").write_bytes(b"new-engine")
        for path in COMPANIONS:
            target = self.build / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b"companion")
        self.steam = self.root / "GameGuru Steam"
        self.steam.mkdir()
        (self.steam / "Guru-MapEditor.exe").write_bytes(b"old-engine")
        (self.steam / "world.fpm").write_bytes(b"personal-world")

    def test_preview_and_backup_install(self):
        archive, checksum = make_overlay(self.build, "0.1.0", self.root / "dist")
        self.assertEqual(hashlib.sha256(archive.read_bytes()).hexdigest(),
                         checksum.read_text().strip())
        files = verify_package(archive)
        backup = self.root / "backup"
        apply_overlay(files, self.steam, backup)
        self.assertFalse(backup.exists())
        self.assertEqual((self.steam / "Guru-MapEditor.exe").read_bytes(), b"old-engine")
        apply_overlay(files, self.steam, backup, True)
        self.assertEqual((self.steam / "Guru-MapEditor.exe").read_bytes(), b"new-engine")
        self.assertEqual((backup / "Guru-MapEditor.exe").read_bytes(), b"old-engine")
        self.assertEqual((self.steam / "world.fpm").read_bytes(), b"personal-world")

    def test_integrity_and_directory_guards(self):
        archive, _ = make_overlay(self.build, "0.1.0", self.root / "dist")
        archive.write_bytes(archive.read_bytes() + b"corrupted")
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            verify_package(archive)
        for name in ("../GameGuru.exe", "world.fpm", "Files/entitybank/foo.fpe",
                     "Files\\scriptbank\\global.lua", "Files/terrainbank/a.dds"):
            self.assertFalse(allowed(name))
        with self.assertRaises(ValueError):
            apply_overlay({"Guru-MapEditor.exe": b"x"}, self.steam, self.steam / "backup", True)

    def test_fails_closed_without_shader_and_optional_ide(self):
        (self.build / COMPANIONS[0]).unlink()
        with self.assertRaises(FileNotFoundError):
            make_overlay(self.build, "0.1.1", self.root / "dist")
        with self.assertRaises(FileNotFoundError):
            make_overlay(self.build, "0.1.1", self.root / "dist", True)

if __name__ == "__main__":
    unittest.main()
