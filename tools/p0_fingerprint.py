#!/usr/bin/env python3
"""Read-only Steam manifest and executable identity collector; never loads game code."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def collect(manifest: Path, executable: Path) -> dict:
    # Publish only allowlisted manifest fields; never account IDs, paths or logs.
    source = manifest.read_text(encoding='utf-8')
    fields = {}
    for key in ('appid', 'buildid'):
        values = re.findall(r'"' + key + r'"\s+"([0-9]+)"', source)
        if len(values) != 1:
            raise ValueError(f'expected exactly one numeric {key}')
        fields[key] = values[0]
    if fields['appid'] not in ('1203620', '2278520'):
        raise ValueError('not an Enshrouded client/server manifest')
    digest = hashlib.sha256()
    with executable.open('rb') as stream:
        header = stream.read(64)
        if len(header) != 64 or header[:2] != b'MZ':
            raise ValueError('not a PE executable')
        pe_offset = struct.unpack_from('<I', header, 60)[0]
        stream.seek(pe_offset)
        signature = stream.read(6)
        if len(signature) != 6 or signature[:4] != b'PE\0\0':
            raise ValueError('invalid PE signature')
        machine = struct.unpack_from('<H', signature, 4)[0]
        if machine != 0x8664:
            raise ValueError('expected x86-64 PE')
        stream.seek(0)
        size = 0
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            size += len(chunk)
            digest.update(chunk)
    return {'schema': 'p0-file-identity-v1', 'steam': fields,
            'executable': {'bytes': size, 'sha256': digest.hexdigest(), 'pe_machine': 'AMD64'},
            'runtime_verdict': 'NOT_RUN', 'compatibility': 'NOT_VERIFIED',
            'limitations': ['Manifest and binary association is supplied by the operator.',
                            'No game revision, symbol availability, load or compatibility is proven.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--executable', required=True, type=Path)
    args = parser.parse_args()
    try:
        result = collect(args.manifest, args.executable)
    except (OSError, ValueError) as exc:
        # Avoid leaking local paths from filesystem exception messages.
        parser.exit(2, f'Identity capture refused: {type(exc).__name__}\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
