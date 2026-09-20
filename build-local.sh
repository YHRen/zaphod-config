#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_root="${ZAPHOD_BUILD_ROOT:-${HOME}/.cache/zaphod-zmk}"
image="${ZMK_BUILD_IMAGE:-zmkfirmware/zmk-build-arm:stable}"

if ! command -v docker >/dev/null 2>&1; then
    echo "Error: Docker is not installed." >&2
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    echo "Error: Docker is not running." >&2
    exit 1
fi

mkdir -p "${build_root}"

docker run --rm \
    -v "${repo_root}:/repo:ro" \
    -v "${build_root}:/work" \
    -w /work \
    "${image}" \
    bash -lc '
        set -euo pipefail

        mkdir -p /work/config
        cp -R /repo/config/. /work/config/

        if [ ! -d /work/.west ]; then
            west init -l /work/config
        fi

        west update --fetch-opt=--filter=tree:0
        west zephyr-export
        west build -p always \
            -s /work/zmk/app \
            -d /work/build/zaphod \
            -b zaphod \
            -- \
            -DZMK_CONFIG=/work/config \
            -DZMK_EXTRA_MODULES=/repo
    '

firmware="${build_root}/build/zaphod/zephyr/zmk.uf2"
output="${ZAPHOD_FIRMWARE_OUTPUT:-${HOME}/Downloads/zaphod-zmk.uf2}"

if [[ ! -f "${firmware}" ]]; then
    echo "Error: build completed without producing ${firmware}." >&2
    exit 1
fi

mkdir -p "$(dirname -- "${output}")"
cp "${firmware}" "${output}"

echo
echo "Firmware ready: ${output}"
