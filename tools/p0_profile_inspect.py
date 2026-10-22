#!/usr/bin/env python3
"""Read-only PE/profile investigation. Emits metadata, never proprietary code bytes.

A signature match or original descriptor is a candidate, not native-call authority.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


class PE:
    def __init__(self, data):
        self.data = data
        if data[:2] != b'MZ':
            raise ValueError('missing DOS header')
        pe = self.unpack('<I', 60)[0]
        if data[pe:pe+4] != b'PE\0\0':
            raise ValueError('missing PE header')
        machine, count, self.timestamp = self.unpack('<HHI', pe+4)
        optional_size = self.unpack('<H', pe+20)[0]
        opt = pe+24
        if machine != 0x8664 or self.unpack('<H', opt)[0] != 0x20b:
            raise ValueError('expected AMD64 PE32+')
        self.base = self.unpack('<Q', opt+24)[0]
        self.image_size = self.unpack('<I', opt+56)[0]
        self.sections = []
        for index in range(count):
            pos = opt+optional_size+index*40
            name = data[pos:pos+8].rstrip(b'\0').decode('ascii')
            virtual_size, rva, size, raw = self.unpack('<IIII', pos+8)
            flags = self.unpack('<I', pos+36)[0]
            if raw+size > len(data):
                raise ValueError('section outside file')
            self.sections.append(dict(name=name, rva=rva, raw=raw, size=size,
                                      virtual_size=virtual_size, executable=bool(flags & 0x20000000)))
        directory, size = self.unpack('<II', opt+112+3*8)
        if size % 12:
            raise ValueError('truncated exception directory')
        offset = self.offset(directory, size)
        self.functions = {}
        for pos in range(offset, offset+size, 12):
            begin, end, unwind = self.unpack('<III', pos)
            if not begin < end <= self.image_size:
                raise ValueError('invalid function extent')
            self.functions[begin] = dict(end=end, unwind=unwind)

    def unpack(self, fmt, offset):
        if offset < 0 or offset+struct.calcsize(fmt) > len(self.data):
            raise ValueError('read outside file')
        return struct.unpack_from(fmt, self.data, offset)

    def offset(self, rva, size=1):
        for section in self.sections:
            delta = rva-section['rva']
            if 0 <= delta and delta+size <= section['size']:
                return section['raw']+delta
        raise ValueError('RVA outside file-backed sections')

    def string_span(self, pointer, count):
        if not 0 < count <= 256:
            raise ValueError('invalid string span')
        offset = self.offset(pointer-self.base, count)
        value = self.data[offset:offset+count].rstrip(b'\0').decode('ascii')
        if not value or not all(32 <= ord(char) < 127 for char in value):
            raise ValueError('invalid descriptor name')
        return value

    def matches(self, pattern):
        tokens = pattern.split()
        if not tokens:
            raise ValueError('empty pattern')
        expression = b''.join(b'.' if token in ('?', '??') else
                              re.escape(bytes([int(token, 16)])) for token in tokens)
        compiled = re.compile(b'(?='+expression+b')', re.DOTALL)
        result = []
        for section in self.sections:
            if section['executable']:
                payload = self.data[section['raw']:section['raw']+section['size']]
                result.extend(section['rva']+match.start() for match in compiled.finditer(payload))
        return result

    def rip_reference_candidates(self, target):
        # Byte-level candidate discovery only. A decoder/manual instruction
        # boundary review must establish that a hit is a real instruction.
        pattern = re.compile(b'[\\x48\\x4c][\\x8b\\x8d][\\x05\\x0d\\x15\\x1d\\x25\\x2d\\x35\\x3d]....', re.DOTALL)
        result = []
        for section in self.sections:
            if not section['executable']:
                continue
            payload = self.data[section['raw']:section['raw']+section['size']]
            for match in pattern.finditer(payload):
                rva = section['rva']+match.start()
                displacement = struct.unpack_from('<i', payload, match.start()+3)[0]
                if rva+7+displacement == target:
                    result.append(rva)
        return result

    def relative_call_candidates(self, target):
        result = []
        for section in self.sections:
            if not section['executable']:
                continue
            payload = self.data[section['raw']:section['raw']+section['size']]
            for match in re.finditer(b'\\xe8', payload[:-4]):
                rva = section['rva']+match.start()
                displacement = struct.unpack_from('<i', payload, match.start()+1)[0]
                if rva+5+displacement == target:
                    result.append(rva)
        return result

    def descriptors(self):
        result = []
        for section in self.sections:
            if section['executable']:
                continue
            # Original descriptors contain a string span, callback pointer and
            # six dependency spans. Require an actual unwind entry callback.
            first = section['raw']+(-section['rva'] % 8)
            for pos in range(first, section['raw']+section['size']-120+1, 8):
                values = self.unpack('<15Q', pos)
                callback = values[2]-self.base
                if callback not in self.functions:
                    continue
                try:
                    name = self.string_span(values[0], values[1])
                    if not re.fullmatch('[a-z][a-z0-9_]{2,95}', name):
                        continue
                    dependencies = []
                    for index in range(3, 14, 2):
                        pointer, count = values[index:index+2]
                        if count > 256 or bool(pointer) != bool(count):
                            raise ValueError('invalid dependency span')
                        names = []
                        for item in range(count):
                            offset = self.offset(pointer-self.base+item*16, 16)
                            names.append(self.string_span(*self.unpack('<QQ', offset)))
                        dependencies.append(dict(descriptor_offset=index*8, names=names))
                    if not any(slot['names'] for slot in dependencies):
                        continue
                    result.append(dict(name=name, callback_rva=callback,
                                       descriptor_rva=section['rva']+pos-section['raw'],
                                       dependency_slots=dependencies, callable=False,
                                       semantic_status='STATIC_CANDIDATE'))
                except (ValueError, UnicodeDecodeError):
                    continue
        return result


def inspect(executable, historical_profile):
    initial = executable.stat()
    data = executable.read_bytes()
    final = executable.stat()
    if (initial.st_ino, initial.st_size, initial.st_mtime_ns) != (final.st_ino, final.st_size, final.st_mtime_ns):
        raise ValueError('binary changed while reading')
    pe = PE(data)
    profile_bytes = historical_profile.read_bytes()
    profile = json.loads(profile_bytes)
    hooks = []
    for name, entry in profile['hooks'].items():
        signature = pe.matches(entry['signature'])
        original = pe.matches(' '.join(f'{value:02x}' for value in entry['original']))
        hooks.append(dict(name=name, full_signature_count=len(signature),
                          full_signature_rvas=signature, historical_overwrite_count=len(original),
                          historical_overwrite_first_rvas=original[:8],
                          verdict='NOT_VALIDATED'))
    descriptors = pe.descriptors()
    selected = {'actor_simulation_start', 'actor_simulation_end', 'player_control',
                'actor_spawn_entity', 'actor_locomotion', 'client_cursor_entity'}
    references = []
    for entry in descriptors:
        if entry['name'] not in {'actor_simulation_start', 'actor_simulation_end'}:
            continue
        for getter in pe.rip_reference_candidates(entry['descriptor_rva']):
            references.append(dict(name=entry['name'], descriptor_rva=entry['descriptor_rva'],
                                   rip_reference_candidate_rva=getter,
                                   direct_call_candidate_rvas=pe.relative_call_candidates(getter),
                                   authority='BYTE_LEVEL_CANDIDATE_REQUIRES_DECODE'))
    return dict(schema='p0-static-execution-investigation-v1',
                executable=dict(sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                                pe_timestamp=pe.timestamp, image_size=pe.image_size),
                historical_profile=dict(id=profile['id'], sha256=hashlib.sha256(profile_bytes).hexdigest()),
                executable_sections=[{key: section[key] for key in ('name', 'rva', 'size')}
                                     for section in pe.sections if section['executable']],
                hook_scans=hooks, named_descriptor_count=len(descriptors), descriptor_reference_candidates=references,
                descriptor_candidates=[entry for entry in descriptors if entry['name'] in selected],
                execution_profile_verdict='HOLD', live_runtime='NOT_RUN',
                limitations=['Static file scan only; no runtime mapping/thread/lifetime validation.',
                             'Unwind entries identify code extents, not ABI or safe patch boundaries.',
                             'Descriptor names/dependencies do not authorize arbitrary callback execution.',
                             'Historical overwrite snippets are diagnostics, never replacement signatures.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--historical-profile', type=Path, required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(inspect(args.executable, args.historical_profile), indent=2))
    except (OSError, ValueError, KeyError, struct.error) as error:
        parser.exit(2, f'Static inspection refused: {type(error).__name__}\n')


if __name__ == '__main__':
    main()
