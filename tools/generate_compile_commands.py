#!/usr/bin/env python3
"""Generate compile_commands.json from a qmake-produced Makefile.

The Qt SDK bundled with Toon Boom Harmony does not ship qmake's
compile_commands_json feature, so this reconstructs a compilation
database by dry-running `make` (-n, without executing anything) and
parsing the compiler invocations it would run.

Usage: generate_compile_commands.py --build-dir <dir> --pro-file <file> --output <file>
"""
import argparse
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

SOURCE_EXTS = {".c", ".cc", ".cpp", ".cxx", ".mm", ".m"}


def find_compile_commands(build_dir: Path, pro_file: Path):
    # -B forces every compile recipe to be printed regardless of whether
    # the .o files are already up to date. The Makefile qmake generates
    # also has a rule to regenerate itself from the .pro file; -B would
    # force that too, actually re-running qmake (which fails unless
    # HARMONY_SDK_ROOT/TB_ROOT are set). -o marks those files as already
    # up to date so that rule is skipped.
    pro_rel = os.path.relpath(pro_file, build_dir)
    result = subprocess.run(
        ["make", "-n", "-B", "-o", "Makefile", "-o", pro_rel],
        cwd=build_dir,
        capture_output=True,
        text=True,
        check=True,
    )

    entries = []
    for line in result.stdout.splitlines():
        line = line.strip()
        if " -c " not in line:
            continue
        try:
            args = shlex.split(line)
        except ValueError:
            continue
        if "-c" not in args:
            continue

        source = None
        for arg in reversed(args):
            if arg.startswith("-"):
                continue
            if Path(arg).suffix in SOURCE_EXTS:
                source = arg
                break
        if source is None:
            continue

        source_path = (build_dir / source).resolve()
        if not source_path.exists():
            continue

        entries.append(
            {
                "directory": str(build_dir),
                "arguments": args,
                "file": str(source_path),
            }
        )
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--pro-file", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    pro_file = args.pro_file.resolve()
    if not (build_dir / "Makefile").exists():
        sys.exit(f"error: no Makefile in {build_dir} (run qmake there first)")

    entries = find_compile_commands(build_dir, pro_file)
    if not entries:
        sys.exit("error: no compile commands found in `make -Bn` output")

    args.output.write_text(json.dumps(entries, indent=2) + "\n")
    print(f"wrote {len(entries)} entries to {args.output}")


if __name__ == "__main__":
    main()
