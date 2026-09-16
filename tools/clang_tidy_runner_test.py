#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright (c) M. Boerger, the MBO Works authors
# SPDX-License-Identifier: Apache-2.0

import io
import os
import signal
import time
import pathlib
import stat
import subprocess
import sys
import tempfile
import textwrap
import unittest
from unittest import mock

from tools import clang_tidy_runner


class ClangTidyRunnerTest(unittest.TestCase):
    def test_default_reserves_one_cpu_and_explicit_jobs_win(self):
        arguments = [
            "--clang-tidy", "clang-tidy",
            "--compile-database", ".",
            "--output", "diagnostics.txt",
            "--test-disabled-checks=",
        ]
        for cpus, expected in ((18, 2), (2, 1), (1, 1), (None, 1)):
            with self.subTest(cpus=cpus), mock.patch.object(
                clang_tidy_runner.os, "cpu_count", return_value=cpus
            ):
                self.assertEqual(clang_tidy_runner.parse_args(arguments).jobs, expected)
                self.assertEqual(
                    clang_tidy_runner.parse_args(arguments + ["--jobs", "8"]).jobs, 8
                )

    def test_progress_line(self):
        result = clang_tidy_runner.Result(
            clang_tidy_runner.Task("mbo/file/glob.cc"), 0, "", 4.25
        )
        self.assertEqual(
            clang_tidy_runner.progress_line(42, 187, result),
            "[ 42/187  22.5%] PASS mbo/file/glob.cc (4.2s)",
        )

    def test_parallel_run_reports_completion_and_failure(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            executable = root / "fake-clang-tidy"
            executable.write_text(
                textwrap.dedent(
                    """\
                    #!/usr/bin/env python3
                    import pathlib
                    import sys
                    import time

                    path = pathlib.Path(sys.argv[-1]).name
                    time.sleep(0.05 if path == "slow.cc" else 0.01)
                    if path == "bad.cc":
                        print("bad.cc:1:1: error: finding")
                        raise SystemExit(1)
                    print(f"{path}: routine successful output")
                    """
                ),
                encoding="utf-8",
            )
            executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            output = root / "diagnostics.txt"
            stream = io.StringIO()
            status = clang_tidy_runner.run_all(
                [
                    clang_tidy_runner.Task("slow.cc"),
                    clang_tidy_runner.Task("good.cc"),
                    clang_tidy_runner.Task("bad.cc"),
                ],
                str(executable),
                ".",
                2,
                str(output),
                stream,
            )

            rendered = stream.getvalue()
            self.assertEqual(status, 1)
            self.assertIn("clang-tidy: 3 translation unit(s), 2 worker(s)", rendered)
            self.assertIn("[1/3  33.3%] PASS good.cc", rendered)
            self.assertIn("PASS slow.cc", rendered)
            self.assertIn("[3/3 100.0%]", rendered)
            self.assertIn("FAIL bad.cc", rendered)
            self.assertIn("clang-tidy: 2 passed, 1 failed", rendered)
            self.assertIn("bad.cc:1:1: error: finding", output.read_text(encoding="utf-8"))
            self.assertNotIn("routine successful output", rendered)
            self.assertIn("routine successful output", output.read_text(encoding="utf-8"))

    def test_registry_terminates_active_children(self):
        registry = clang_tidy_runner.ProcessRegistry()
        process = registry.start([sys.executable, "-c", "import time; time.sleep(30)"])
        self.assertIsNotNone(process)

        registry.terminate_all()

        self.assertIsNotNone(process.poll())
        process.communicate()
        self.assertIsNone(registry.start(["must-not-launch-after-shutdown"]))

    def test_sigterm_stops_running_and_queued_workers(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            executable = root / "fake-clang-tidy"
            executable.write_text(
                "#!/usr/bin/env python3\n"
                "import os, pathlib, sys, time\n"
                "pathlib.Path(sys.argv[-1]).write_text(str(os.getpid()))\n"
                "time.sleep(30)\n"
            )
            executable.chmod(0o755)
            started = [root / f"started-{number}" for number in range(5)]
            command = [
                sys.executable, clang_tidy_runner.__file__,
                "--clang-tidy", str(executable), "--compile-database", ".",
                "--output", str(root / "output"), "--jobs", "2",
                "--test-disabled-checks=",
            ]
            for path in started:
                command.extend(["--source", str(path)])
            process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            try:
                deadline = time.monotonic() + 5
                while sum(path.exists() for path in started) < 2 and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertEqual(sum(path.exists() for path in started), 2)
                process.send_signal(signal.SIGTERM)
                output, _ = process.communicate(timeout=5)
                self.assertEqual(process.returncode, 130, output)
                self.assertIn("worker pool terminated", output)
                self.assertEqual(sum(path.exists() for path in started), 2)
                for path in started[:2]:
                    with self.assertRaises(ProcessLookupError):
                        os.kill(int(path.read_text()), 0)
            finally:
                if process.poll() is None:
                    process.kill()
                process.communicate()

    def test_invalid_worker_counts_fail_without_starting_a_tool(self):
        for value in ("0", "-1", "nonsense"):
            result = subprocess.run([
                sys.executable, clang_tidy_runner.__file__, "--clang-tidy", "must-not-run",
                "--compile-database", ".", "--output", "/dev/null",
                "--test-disabled-checks=", "--jobs", value,
            ], capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)

    def test_precommit_uses_one_coordinator(self):
        configuration = (pathlib.Path(__file__).parents[1] / ".pre-commit-config.yaml").read_text()
        hook = configuration.split("      - id: clang-tidy\n", 1)[1].split("      - id:", 1)[0]
        self.assertIn("require_serial: true", hook)

    def test_extra_arguments_are_forwarded(self):
        args = clang_tidy_runner.parse_args(
            [
                "--clang-tidy=clang-tidy",
                "--compile-database=.",
                "--output=diagnostics.txt",
                "--test-disabled-checks=-example",
                "--extra-arg-before=-isystem/libcxx",
            ]
        )
        self.assertEqual(args.extra_arg_before, ["-isystem/libcxx"])


if __name__ == "__main__":
    unittest.main()
