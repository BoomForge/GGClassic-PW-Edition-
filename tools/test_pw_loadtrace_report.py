"""Offline tests for the PW Edition load profiler CSV report."""
import tempfile
import unittest
from pathlib import Path

from analyse_pw_loadtrace import aggregate, read_trace, report


class PWLoadTraceReportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "trace.csv"

    def test_aggregation_and_sorting(self):
        self.path.write_text(
            "tick_ms,pid,stage,duration_ms\n"
            "1,25,game.prepare.ai_obstacles,100.000\n"
            "2,25,editor.preview_init.terrain_height_samples,500.000\n"
            "3,25,game.prepare.ai_obstacles,120.000\n"
            "4,25,editor.preview_init.terrain_height_samples,900.000\n",
            encoding="utf8",
        )
        rows = read_trace(self.path)
        results = aggregate(rows)
        self.assertEqual(results[0]["stage"], "editor.preview_init.terrain_height_samples")
        self.assertEqual(results[0]["count"], 2)
        self.assertEqual(results[0]["median_ms"], 700.0)
        self.assertEqual(results[1]["mean_ms"], 110.0)
        output = report([self.path])
        self.assertIn("Parent scopes contain child scopes", output)
        self.assertIn("terrain_height_samples", output)

    def test_begin_and_slow_markers_are_not_timed_as_completed_phases(self):
        self.path.write_text(
            "tick_ms,pid,stage,duration_ms\n"
            "1,25,begin.editor.preview_init.total,0.000\n"
            "2,25,slow.editor.preview_init.total,60500.000\n"
            "3,25,editor.preview_init.total,60500.000\n",
            encoding="utf8",
        )
        rows = read_trace(self.path)
        self.assertEqual(len(rows), 3)
        stages = aggregate(rows)
        self.assertEqual(len(stages), 1)
        self.assertEqual(stages[0]["stage"], "editor.preview_init.total")
        self.assertEqual(stages[0]["median_ms"], 60500.0)

    def test_reject_invalid_duration(self):
        self.path.write_text(
            "tick_ms,pid,stage,duration_ms\n"
            "1,25,game.entities.bank,-5\n", encoding="utf8"
        )
        with self.assertRaisesRegex(ValueError, "negative"):
            read_trace(self.path)

    def test_empty_trace(self):
        self.path.write_text("tick_ms,pid,stage,duration_ms\n", encoding="utf8")
        self.assertEqual(report([self.path]), "No trace samples found.")


if __name__ == "__main__":
    unittest.main()
