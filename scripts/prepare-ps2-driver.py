#!/usr/bin/env python3
"""Apply the reviewed patch to an isolated checkout of the pinned PS/2 module."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def prepare(module):
    module = Path(module).resolve()
    patches = Path(__file__).resolve().parents[1] / "patches"
    spec = json.loads((patches / "ps2-uart-reliability.json").read_text())
    patch = patches / spec["patch"]
    if hashlib.sha256(patch.read_bytes()).hexdigest() != spec["sha256"]:
        raise SystemExit("PS/2 patch hash differs from the reviewed manifest")
    git = ["git", "-c", f"safe.directory={module}", "-c", "core.fsmonitor=false", "-C", str(module)]
    revision = subprocess.check_output(git + ["rev-parse", "HEAD"], text=True).strip()
    if revision != spec["revision"]:
        raise SystemExit(f"Unsupported PS/2 revision: {revision}")

    def matches(state):
        return all(hashlib.sha256((module / name).read_bytes()).hexdigest() == hashes[state]
                   for name, hashes in spec["files"].items())

    if matches("after"):
        print("PS/2 patch already applied and verified")
        return spec
    if not matches("before"):
        raise SystemExit("PS/2 source has unexpected edits; use a clean isolated checkout")
    subprocess.run(git + ["apply", "--check", str(patch)], check=True)
    subprocess.run(git + ["apply", str(patch)], check=True)
    if not matches("after"):
        raise SystemExit("PS/2 patched source hash verification failed")
    print("PS/2 patch applied and verified")
    return spec


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("module", type=Path, help="Writable isolated PS/2 module checkout")
    prepare(parser.parse_args().module)
