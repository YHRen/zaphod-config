#!/usr/bin/env python3
"""Compile and exercise the actual patched driver against a host API contract model."""
import argparse
import os
import platform
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('module', type=Path)
args = parser.parse_args()
source = args.module.resolve() / 'src/drivers/ps2/ps2_uart.c'
with tempfile.TemporaryDirectory(prefix='zaphod-ps2-test-') as directory:
    for open_drain in [0, 1]:
        executable = Path(directory) / f'driver-{open_drain}'
        cmd = [os.environ.get('CC', 'cc'), '-std=gnu11', '-O1', '-g',
               '-Werror=implicit-function-declaration', '-ffunction-sections', '-fdata-sections',
               '-Wl,-dead_strip' if platform.system() == 'Darwin' else '-Wl,--gc-sections',
               f'-DPS2_UART_SOURCE="{source}"', f'-DCONFIG_PS2_UART_OPEN_DRAIN={open_drain}',
               '-I', str(root / 'tests/ps2_uart/mocks'), str(root / 'tests/ps2_uart/test_driver.c'),
               '-o', str(executable)]
        subprocess.run(cmd, check=True)
        subprocess.run([str(executable)], check=True)
