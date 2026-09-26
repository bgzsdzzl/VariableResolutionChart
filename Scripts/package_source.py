#!/usr/bin/env python3
"""Package the plugin into a distributable source zip.

The list of files and directories to include is read from `.release`
at the repository root. One entry per line, relative to the repo root.
Directories are packed recursively. Lines starting with '#' and blank
lines are ignored.

Reads VersionName from the .uplugin and writes
dist/VariableResolutionChart-v<VersionName>-source.zip.

No Binaries, no Intermediate, no Saved, no .git, no .github, no Scripts.
"""

import json
import sys
import zipfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PLUGIN_NAME = "VariableResolutionChart"
UPLUGIN = REPO_ROOT / f"{PLUGIN_NAME}.uplugin"
MANIFEST = REPO_ROOT / ".release"
DIST_DIR = REPO_ROOT / "dist"

# Skips that apply even inside included folders.
EXCLUDE_DIRS = {".git", "Binaries", "Intermediate", "Saved", ".vs", ".idea", "__pycache__"}
EXCLUDE_FILES = {".DS_Store", "Thumbs.db"}
EXCLUDE_SUFFIXES = {".pyc", ".pyo"}


def read_version():
    with UPLUGIN.open("r", encoding="utf-8") as f:
        data = json.load(f)
    return data.get("VersionName", "0.0.0")


def read_manifest():
    if not MANIFEST.exists():
        print(f"error: {MANIFEST.name} not found at repo root", file=sys.stderr)
        sys.exit(1)

    entries = []
    for lineno, raw in enumerate(MANIFEST.read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entries.append(line)

    if not entries:
        print("error: .release is empty", file=sys.stderr)
        sys.exit(1)

    return entries


def collect_files(entries):
    files = []
    for entry in entries:
        p = REPO_ROOT / entry
        if not p.exists():
            print(f"warning: '{entry}' not found, skipping", file=sys.stderr)
            continue
        if p.is_file():
            files.append(p)
            continue
        for child in p.rglob("*"):
            if not child.is_file():
                continue
            if any(part in EXCLUDE_DIRS for part in child.parts):
                continue
            if child.name in EXCLUDE_FILES:
                continue
            if child.suffix in EXCLUDE_SUFFIXES:
                continue
            files.append(child)
    return files


def main():
    version = read_version()
    zip_name = f"{PLUGIN_NAME}-v{version}-source.zip"
    DIST_DIR.mkdir(exist_ok=True)
    zip_path = DIST_DIR / zip_name

    if zip_path.exists():
        zip_path.unlink()

    entries = read_manifest()
    files = collect_files(entries)
    if not files:
        print("error: no files to package", file=sys.stderr)
        return 1

    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for f in sorted(files):
            rel = f.relative_to(REPO_ROOT)
            arcname = Path(PLUGIN_NAME) / rel
            zf.write(f, arcname)

    size_kb = zip_path.stat().st_size / 1024
    print(f"Wrote {zip_path.relative_to(REPO_ROOT)} ({len(files)} files, {size_kb:.1f} KB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
