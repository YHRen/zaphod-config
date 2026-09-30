#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Deliberately separate from the original XIAO/Zaphod build cache.
build_root="${ZAPHOD_REPLACEMENT_BUILD_ROOT:-${HOME}/.cache/zaphod-zmk-display-replacement}"
image="${ZMK_BUILD_IMAGE:-zmkfirmware/zmk-build-arm:stable}"
target="${1:-zaphod-trackpoint-display-replacement}"
diagnostic="${2:-}"
if [[ -n "${diagnostic}" && "${diagnostic}" != "--diagnostic" ]]; then
    echo "Usage: $0 [build.yaml artifact name] [--diagnostic]" >&2
    exit 2
fi
if ! docker info >/dev/null 2>&1; then
    echo "Error: Docker must be installed and running." >&2
    exit 1
fi
mkdir -p "${build_root}"
if [[ "$(cd -- "${build_root}" && pwd -P)" == "${HOME}/.cache/zaphod-zmk" ]]; then
    echo "Refusing to patch the original shared ZMK cache; choose a separate build root." >&2
    exit 1
fi
docker run --rm \
    -v "${repo_root}:/repo:ro" -v "${build_root}:/work" -w /work \
    -e "BUILD_TARGET=${target}" -e "DIAGNOSTIC=${diagnostic}" \
    -e "SKIP_UPDATE=${ZAPHOD_REPLACEMENT_SKIP_UPDATE:-0}" \
    "${image}" bash -lc '
        set -euo pipefail
        mkdir -p /work/config
        cp -R /repo/config/. /work/config/
        if [ ! -d /work/.west ]; then west init -l /work/config; fi
        if [ "${SKIP_UPDATE}" != "1" ]; then west update --fetch-opt=--filter=tree:0; fi
        west zephyr-export
        args=(--workspace /work --target "${BUILD_TARGET}")
        if [ -n "${DIAGNOSTIC}" ]; then args+=(--diagnostic); fi
        python3 /repo/scripts/build-firmware.py "${args[@]}"
    '
echo "Artifacts: ${build_root}/artifacts/${target}${diagnostic:+-diagnostic}"
