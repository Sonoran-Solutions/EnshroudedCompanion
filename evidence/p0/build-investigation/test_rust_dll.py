#!/usr/bin/env python3
"""Load actual Rust DLL in a synthetic host; never supply a game directory."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser(description=__doc__)
for name in ('cxx', 'wine', 'wineserver', 'dll'):
    p.add_argument('--' + name, required=True, type=Path)
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix='p0-rust-load-') as directory:
    root = Path(directory)
    prefix = root / 'prefix'
    prefix.mkdir()
    exe = root / 'probe.exe'
    subprocess.run([str(a.cxx), '-std=c++20', '-municode', '-static',
                    str(Path(__file__).with_name('rust_dll_probe.cpp')), '-o', str(exe)],
                   check=True, timeout=30)
    env = dict(os.environ, WINEPREFIX=str(prefix), WINEDEBUG='-all',
               WINEDLLOVERRIDES='winemenubuilder.exe=d')
    try:
        result = subprocess.run([str(a.wine), str(exe),
                                 'Z:' + str(a.dll.resolve()).replace('/', '\\')],
                                env=env, text=True, capture_output=True, timeout=45)
        print(result.stdout.strip())
        print('exit=' + str(result.returncode))
        if result.returncode:
            raise SystemExit(result.returncode)
    finally:
        subprocess.run([str(a.wineserver), '-k'], env=env, timeout=10, capture_output=True)
        subprocess.run([str(a.wineserver), '-w'], env=env, timeout=10, check=True, capture_output=True)
