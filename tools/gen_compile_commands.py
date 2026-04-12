#!/usr/bin/env python3
"""
gen_compile_commands.py - Generate compile_commands.json from Makefile

Runs `make -n` (dry-run) to capture the actual compiler commands that would
be executed, then parses them to extract compilation entries for C/C++ files.

This approach is robust because:
- All Makefile variables are fully expanded by make itself
- Conditionals, includes, $(shell ...) are all resolved
- Works with any Makefile structure (recursive, non-recursive, generated)

Usage:
    python3 gen_compile_commands.py [options] [make_target]

    # Simple - run in project directory
    cd /path/to/project
    python3 /path/to/gen_compile_commands.py

    # Specify Makefile directory
    python3 gen_compile_commands.py -C /path/to/project

    # Specify output location
    python3 gen_compile_commands.py -o /path/to/compile_commands.json

    # Pass extra flags to make
    python3 gen_compile_commands.py -- RELEASE=1 CC=gcc-12

    # Specify a target
    python3 gen_compile_commands.py all
"""

import argparse
import json
import os
import re
import subprocess
import sys


# Compiler names we recognize
COMPILERS = {
    "gcc", "g++", "cc", "c++",
    "clang", "clang++",
    "arm-none-eabi-gcc", "arm-none-eabi-g++",
}

# Patterns that match compiler invocations
# Matches: gcc, g++, cc, /usr/bin/g++, arm-none-eabi-g++, etc.
COMPILER_RE = re.compile(
    r'(?:^|\s)(\S*(?:gcc|g\+\+|cc|c\+\+|clang|clang\+\+)(?:-\d+)?)\s'
)

# Source file extensions
C_EXTENSIONS = {".c"}
CPP_EXTENSIONS = {".cpp", ".cc", ".cxx", ".C", ".c++"}
ALL_EXTENSIONS = C_EXTENSIONS | CPP_EXTENSIONS

# Flags we want to keep (compilation-relevant)
KEEP_FLAG_PREFIXES = (
    "-I", "-D", "-U",           # includes and defines
    "-std=",                     # language standard
    "-f", "-W", "-w",           # compiler flags, warnings
    "-m", "-march", "-mtune",   # architecture
    "-O",                        # optimization (affects preprocessor)
    "-g",                        # debug info
    "-isystem", "-iquote",      # include variants
    "-include",                  # force include
    "-pthread",                  # threading
)

# Flags to skip entirely (with their argument if any)
SKIP_FLAGS_WITH_ARG = {"-o", "-MF", "-MQ", "-MT", "-MJ"}
SKIP_FLAGS = {"-c", "-MD", "-MP", "-MMD"}


def find_compiler_lines(make_output: str) -> list[str]:
    """Extract lines that look like compiler invocations."""
    lines = []
    for line in make_output.splitlines():
        line = line.strip()
        if not line:
            continue
        # Skip make messages like "make[1]: Entering directory..."
        if line.startswith("make[") or line.startswith("make:"):
            continue
        # Check if line contains a compiler invocation
        if COMPILER_RE.search(line):
            # Verify it compiles a source file (has -c or a source file)
            has_source = any(
                token.endswith(tuple(ALL_EXTENSIONS))
                for token in line.split()
            )
            if has_source:
                lines.append(line)
    return lines


def parse_compile_line(line: str, directory: str) -> dict | None:
    """Parse a compiler invocation line into a compile_commands entry."""
    tokens = tokenize_command(line)
    if not tokens:
        return None

    compiler = None
    source_file = None
    flags = []

    i = 0
    while i < len(tokens):
        token = tokens[i]

        # Detect compiler
        if compiler is None:
            basename = os.path.basename(token)
            # Strip version suffix like gcc-12
            base_no_ver = re.sub(r'-\d+$', '', basename)
            if base_no_ver in COMPILERS or basename in COMPILERS:
                compiler = token
                i += 1
                continue

        # Skip flags that take an argument
        if token in SKIP_FLAGS_WITH_ARG:
            i += 2  # skip flag + its argument
            continue

        # Skip standalone flags
        if token in SKIP_FLAGS:
            i += 1
            continue

        # Source file
        if not token.startswith("-"):
            _, ext = os.path.splitext(token)
            if ext in ALL_EXTENSIONS:
                source_file = token
                i += 1
                continue

        # Keep relevant flags
        if token.startswith(KEEP_FLAG_PREFIXES):
            flags.append(token)
            # Some flags take a separate argument: -I /path, -include file
            if token in ("-I", "-D", "-U", "-isystem", "-iquote", "-include"):
                if i + 1 < len(tokens):
                    flags.append(tokens[i + 1])
                    i += 2
                    continue

        i += 1

    if not compiler or not source_file:
        return None

    # Resolve source file path
    if not os.path.isabs(source_file):
        source_file = os.path.normpath(os.path.join(directory, source_file))

    # Resolve -I paths to absolute
    resolved_flags = []
    i = 0
    while i < len(flags):
        flag = flags[i]
        if flag == "-I" and i + 1 < len(flags):
            path = flags[i + 1]
            if not os.path.isabs(path):
                path = os.path.normpath(os.path.join(directory, path))
            resolved_flags.append("-I")
            resolved_flags.append(path)
            i += 2
        elif flag.startswith("-I") and len(flag) > 2:
            path = flag[2:]
            if not os.path.isabs(path):
                path = os.path.normpath(os.path.join(directory, path))
            resolved_flags.append("-I" + path)
            i += 1
        else:
            resolved_flags.append(flag)
            i += 1

    command = " ".join([compiler] + resolved_flags + ["-c", source_file])

    return {
        "directory": directory,
        "command": command,
        "file": source_file,
    }


def tokenize_command(line: str) -> list[str]:
    """Split a command line respecting quotes."""
    tokens = []
    current = []
    in_quote = None

    for char in line:
        if in_quote:
            if char == in_quote:
                in_quote = None
            else:
                current.append(char)
        elif char in ('"', "'"):
            in_quote = char
        elif char in (' ', '\t'):
            if current:
                tokens.append(''.join(current))
                current = []
        elif char == '\\':
            continue  # skip escape char in dry-run output
        else:
            current.append(char)

    if current:
        tokens.append(''.join(current))

    return tokens


def run_make_dry_run(directory: str, target: str = "",
                     extra_args: list[str] = None) -> str:
    """Run make -nB (dry-run, unconditional) and capture output."""
    cmd = ["make", "-nB", "--no-print-directory"]
    if target:
        cmd.append(target)
    if extra_args:
        cmd.extend(extra_args)

    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True,
            cwd=directory, timeout=60
        )
        return result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        print("ERROR: make -n timed out after 60s", file=sys.stderr)
        return ""
    except FileNotFoundError:
        print("ERROR: 'make' not found", file=sys.stderr)
        return ""


def generate(directory: str, target: str = "",
             extra_args: list[str] = None,
             output: str = None) -> list[dict]:
    """Generate compile_commands.json entries."""
    directory = os.path.abspath(directory)

    print(f"Running make -n in {directory}...", file=sys.stderr)
    make_output = run_make_dry_run(directory, target, extra_args)

    if not make_output:
        print("ERROR: make -n produced no output", file=sys.stderr)
        return []

    compiler_lines = find_compiler_lines(make_output)
    print(f"Found {len(compiler_lines)} compiler invocations", file=sys.stderr)

    entries = []
    seen_files = set()

    for line in compiler_lines:
        entry = parse_compile_line(line, directory)
        if entry and entry["file"] not in seen_files:
            entries.append(entry)
            seen_files.add(entry["file"])

    print(f"Generated {len(entries)} compile_commands entries", file=sys.stderr)

    # Write output
    output_path = output or os.path.join(directory, "compile_commands.json")
    with open(output_path, "w") as f:
        json.dump(entries, f, indent=2)
    print(f"Wrote {output_path}", file=sys.stderr)

    return entries


def main():
    parser = argparse.ArgumentParser(
        description="Generate compile_commands.json from Makefile (dry-run)"
    )
    parser.add_argument(
        "-C", "--directory", default=".",
        help="Directory containing the Makefile (default: current dir)"
    )
    parser.add_argument(
        "-o", "--output",
        help="Output path (default: <directory>/compile_commands.json)"
    )
    parser.add_argument(
        "target", nargs="?", default="",
        help="Make target to dry-run (default: default target)"
    )
    parser.add_argument(
        "extra_args", nargs="*",
        help="Extra arguments passed to make (e.g. RELEASE=1 CC=gcc-12)"
    )

    args = parser.parse_args()

    entries = generate(
        directory=args.directory,
        target=args.target,
        extra_args=args.extra_args,
        output=args.output,
    )

    if not entries:
        print("WARNING: No compile commands generated", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
