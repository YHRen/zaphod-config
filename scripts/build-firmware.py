#!/usr/bin/env python3
"""Build one build.yaml target from an already populated, isolated west workspace."""
import argparse
import hashlib
import importlib.util
import json
import os
import subprocess
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--target", default="zaphod-trackpoint-display-replacement")
    parser.add_argument("--diagnostic", action="store_true")
    parser.add_argument("--check-shield-conflict", action="store_true",
                        help="Only verify that both shield orders are rejected")
    args = parser.parse_args()
    workspace = args.workspace.resolve()
    targets = {item.get("artifact-name", item["board"]): item
               for item in yaml.safe_load((ROOT / "build.yaml").read_text())["include"]}
    if args.target not in targets:
        parser.error(f"Unknown target; choose from {', '.join(targets)}")
    entry = targets[args.target]
    spec = importlib.util.spec_from_file_location("prepare", ROOT / "scripts/prepare-ps2-driver.py")
    prepare = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prepare)
    patch = prepare.prepare(workspace / "kb_zmk_ps2_mouse_trackpoint_driver")
    if args.check_shield_conflict:
        check_shield_conflict(workspace)
        return
    name = args.target + ("-diagnostic" if args.diagnostic else "")
    build = workspace / "build" / name
    cmd = ["west", "build", "-p", "always", "-s", str(workspace / "zmk/app"),
           "-d", str(build), "-b", entry["board"]]
    # Zaphod already declares its CDC ACM console. The upstream snippet assumes
    # a zephyr_udc0 label this legacy board does not provide and adds a second port.
    snippet = entry.get("snippet")
    if args.diagnostic and entry["board"] != "zaphod":
        snippet = "zmk-usb-logging"
    if snippet:
        cmd += ["-S", snippet]
    cmd += ["--", f"-DZMK_CONFIG={ROOT / 'config'}", f"-DZMK_EXTRA_MODULES={ROOT}",
            f"-DZephyr_DIR={workspace / 'zephyr/share/zephyr-package/cmake'}"]
    if entry.get("shield"):
        cmd += [f"-DSHIELD={entry['shield']}"]
    if args.diagnostic:
        cmd += ["-DCONFIG_ASSERT=y", "-DCONFIG_ZMK_USB_LOGGING=y",
                "-DCONFIG_ZMK_LOG_LEVEL_INF=y", "-DCONFIG_PS2_LOG_LEVEL_INF=y"]
    env = os.environ.copy()
    env["ZEPHYR_BASE"] = str(workspace / "zephyr")
    logs = workspace / "logs"
    logs.mkdir(exist_ok=True)
    log = logs / (name + ".log")
    print(f"Building {name}; log: {log}", flush=True)
    with log.open("w") as output:
        result = subprocess.run(cmd, cwd=workspace, env=env, stdout=output,
                                stderr=subprocess.STDOUT)
    if result.returncode:
        print("\n".join(log.read_text().splitlines()[-70:]))
        raise SystemExit(result.returncode)
    lines = log.read_text().splitlines()
    for index, line in enumerate(lines):
        if line.startswith("Memory region"):
            print("\n".join(lines[index:index+4]))
    firmware = build / "zephyr/zmk.uf2"
    if not firmware.is_file():
        raise SystemExit(f"Missing firmware: {firmware}")
    verify = ["python3", str(ROOT / "scripts/verify-firmware.py"),
              "--build", str(build), "--target", args.target]
    if args.diagnostic:
        verify += ["--diagnostic"]
    subprocess.run(verify, check=True)
    artifacts = workspace / "artifacts" / name
    artifacts.mkdir(parents=True, exist_ok=True)
    import shutil
    shutil.copy2(firmware, artifacts / (name + ".uf2"))
    shutil.copy2(log, artifacts / "build.log")
    for file in [".config", "zephyr.dts", "zmk.map"]:
        shutil.copy2(build / "zephyr" / file, artifacts / file)
    revisions = {}
    for project in ["zmk", "zephyr", "kb_zmk_ps2_mouse_trackpoint_driver"]:
        path = workspace / project
        revisions[project] = subprocess.check_output(
            ["git", "-c", f"safe.directory={path}", "-C", str(path), "rev-parse", "HEAD"],
            text=True).strip()
    source_hashes = {}
    for folder in ["boards", "config", "patches", "scripts"]:
        for source in sorted((ROOT / folder).rglob("*")):
            if source.is_file() and "__pycache__" not in source.parts:
                source_hashes[str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    source_hashes["build.yaml"] = hashlib.sha256((ROOT / "build.yaml").read_bytes()).hexdigest()
    record = {"configuration_source_sha256": source_hashes, "target": args.target, "diagnostic": args.diagnostic, "revisions": revisions,
              "ps2_patch_sha256": patch["sha256"], "ps2_source_hashes": patch["files"],
              "uf2_sha256": hashlib.sha256(firmware.read_bytes()).hexdigest()}
    (artifacts / "provenance.json").write_text(json.dumps(record, indent=2) + "\n")
    print(f"Verified firmware and provenance: {artifacts}")


def check_shield_conflict(workspace):
    shields = ["zaphod_trackpoint", "zaphod_trackpoint_display_replacement"]
    env = os.environ.copy()
    env["ZEPHYR_BASE"] = str(workspace / "zephyr")
    logs = workspace / "logs"
    logs.mkdir(exist_ok=True)
    for index, order in enumerate([shields, list(reversed(shields))]):
        cmd = ["west", "build", "-p", "always", "-s", str(workspace / "zmk/app"),
               "-d", str(workspace / "build" / f"rejected-shield-combination-{index}"),
               "-b", "zaphod", "--", f"-DZMK_CONFIG={ROOT / 'config'}",
               f"-DZMK_EXTRA_MODULES={ROOT}",
               f"-DZephyr_DIR={workspace / 'zephyr/share/zephyr-package/cmake'}",
               "-DSHIELD=" + " ".join(order)]
        result = subprocess.run(cmd, cwd=workspace, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (logs / f"rejected-shield-combination-{index}.log").write_text(result.stdout)
        if not result.returncode or "Select only one Zaphod TrackPoint shield" not in result.stdout:
            print(result.stdout)
            raise SystemExit("Dual-shield build was not rejected by the expected guard")
        print("Correctly rejected: " + " + ".join(order))


if __name__ == "__main__":
    main()
