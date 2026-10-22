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
import subprocess
import tempfile


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


def decode_snapshot(data, ranges):
    """Decode only a private snapshot of the exact bytes already hashed."""
    decoded = {}
    with tempfile.TemporaryDirectory(prefix='p0-profile-decode-') as directory:
        snapshot = Path(directory)/'image.exe'
        snapshot.write_bytes(data)
        snapshot.chmod(0o400)
        for begin, end in ranges:
            completed = subprocess.run([
                'objdump', '-d', '--no-show-raw-insn', '-M', 'intel',
                f'--start-address={0x140000000+begin:#x}',
                f'--stop-address={0x140000000+end:#x}', str(snapshot)
            ], check=True, capture_output=True, text=True, timeout=60)
            for line in completed.stdout.splitlines():
                match = re.fullmatch(r'\s*([0-9a-f]+):\s+(.+)', line)
                if match:
                    decoded[int(match[1],16)-0x140000000] = ' '.join(match[2].split())
    return decoded


def inspect_scheduler_relations(executable):
    """Verify exact-build static relations, never execution/lifetime authority."""
    data = executable.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != 'd64966e23b1a458ee6f147bcbceac0b577fa7e39a212f371287d8cc119817882':
        raise ValueError('scheduler relation map belongs to a different binary')
    # Each range starts at an independently inspected real function boundary.
    # No second read of the changing installed path is used by objdump.
    decoded = decode_snapshot(data, [
        (0x9a1310,0x9e2d07), (0xa1fd40,0xa20170),
        (0xe3bad0,0xe3be84), (0xe48b60,0xe49fa0),
        (0xe4a200,0xe4a5c6), (0x8e5c00,0x8e5fa0),
        (0x2701e0,0x2701f2),
    ])
    # Expected operand relations are analyst descriptions, not byte signatures
    # or callable ABI declarations. Any mismatch refuses this evidence map.
    expected = [
        ('caller_9a2d30_stack_0',0x9a2d30,'rex push rbp'),
        ('caller_9a2d30_stack_2',0x9a2d32,'push rbx'),
        ('caller_9a2d30_stack_3',0x9a2d33,'push rdi'),
        ('caller_9a2d30_stack_4',0x9a2d34,'push r12'),
        ('caller_9a2d30_stack_6',0x9a2d36,'push r13'),
        ('caller_9a2d30_stack_16',0x9a2d40,'sub rsp,0x890'),
        ('caller_9a3400_stack_0',0x9a3400,'rex push rbp'),
        ('caller_9a3400_stack_2',0x9a3402,'push rbx'),
        ('caller_9a3400_stack_3',0x9a3403,'push rdi'),
        ('caller_9a3400_stack_4',0x9a3404,'push r12'),
        ('caller_9a3400_stack_6',0x9a3406,'push r13'),
        ('caller_9a3400_stack_16',0x9a3410,'sub rsp,0x890'),
        ('alternate_iterator_frame_store',0x9a3318,'mov QWORD PTR [rsp+0x30],rax'),
        ('alternate_iterator_argument',0x9a331d,'lea rcx,[rsp+0x30]'),
        ('registered_thread_context',0xe3bae4,'mov rbx,QWORD PTR [rcx+0x78]'),
        ('thread_frame_context',0xe3bcdb,'mov rcx,rbx'),
        ('thread_frame_loop_call',0xe3bcde,'call 0x140e4a200'),
        ('shared_scene_gate_address',0xe4a215,'lea r12,[rcx+0x538]'),
        ('shared_scene_gate_acquire',0xe4a22f,'call 0x1408ae160'),
        ('scene_entry_context_argument',0xe4a30d,'mov r8,QWORD PTR [rdi]'),
        ('scene_entry_execution_call',0xe4a324,'call 0x140e48b60'),
        ('scene_context_retained',0xe48b89,'mov r14,r8'),
        ('scene_outer_execution_owner',0xe4974b,'mov rcx,QWORD PTR [r14+0x1d0]'),
        ('scene_execution_call',0xe49752,'call 0x140a1fd40'),
        ('alternate_scene_outer_execution_owner',0xe49806,'mov rcx,QWORD PTR [r14+0x1d0]'),
        ('alternate_scene_execution_call',0xe4980d,'call 0x140a1fd40'),
        ('execution_manager_address',0xa1fd77,'lea rcx,[rdi+0xe15dc0]'),
        ('execution_manager_tick_call',0xa1fd7e,'call 0x140a1fe10'),
        ('manager_prepass',0xa2000a,'call 0x1409a1310'),
        ('manager_scheduler_mode',0xa20012,'cmp BYTE PTR [rsi+0x518],r13b'),
        ('manager_scheduler_mode_branch',0xa20019,'je 0x140a20022'),
        ('manager_selected_scheduler',0xa2001b,'call 0x1409a21e0'),
        ('manager_alternate_scheduler',0xa20022,'call 0x1409a3ae0'),
        ('shared_scene_gate_release',0xe4a5b6,'lock dec DWORD PTR [r12]'),
        ('job_handle_wait_redirect',0x8e5c02,'jmp 0x1408e0630'),
        ('queue_pending_count_check',0x8e5e61,'cmp DWORD PTR [rdi+0x1c0],0x0'),
        ('registry_from_manager',0x9a2733,'mov r13,QWORD PTR [r10]'),
        ('registry_system_array',0x9a273a,'add r13,0x745a28'),
        ('registry_system_stride',0x9a271e,'imul rcx,rax,0x1f20'),
        ('registry_selected_record',0x9a274b,'add r13,rcx'),
        ('batch_task_array_source',0x9a27b0,'mov rax,QWORD PTR [r10+0x528]'),
        ('batch_task_array_store',0x9a27bd,'mov QWORD PTR [rcx],rax'),
        ('batch_system_record_store',0x9a27d3,'mov QWORD PTR [rcx+0x10],r13'),
        ('inline_dispatch_call',0x9a2b5f,'call 0x1409a3400'),
        ('worker_task_index',0x9e2cc7,'mov r8d,DWORD PTR [rdi+0x14]'),
        ('worker_dispatch_call',0x9e2cd4,'call 0x1409a3400'),
        ('frame_manager_store',0x9a34ae,'mov QWORD PTR [rsp+0x40],rcx'),
        ('iterator_frame_address',0x9a39eb,'lea rax,[rsp+0x40]'),
        ('iterator_frame_store',0x9a39f4,'mov QWORD PTR [rsp+0x30],rax'),
        ('callback_iterator_argument',0x9a39f9,'lea rcx,[rsp+0x30]'),
        ('callback_record_load',0x9a39fe,'mov rax,QWORD PTR [rdi+0x10]'),
        ('registered_callback_load',0x9a3a02,'mov rdx,QWORD PTR [rax+0x28]'),
        ('registered_callback_indirect_call',0x9a3a06,'call rdx'),
        ('alternate_callback_record_load',0x9a3322,'mov rax,QWORD PTR [rdi+0x10]'),
        ('alternate_registered_callback_load',0x9a3326,'mov rdx,QWORD PTR [rax+0x28]'),
        ('alternate_registered_callback_indirect_call',0x9a332a,'call rdx'),
        ('worker_completion_system_id',0x9e2ce4,'movzx edx,WORD PTR [rdx+0x8]'),
        ('worker_completion_notify',0x9e2ce8,'call 0x1409ca210'),
        ('job_handle_wait',0x9a2c73,'call 0x1408e5c00'),
        ('job_queue_wait',0x9a2c84,'call 0x1408e5e20'),
    ]
    checks = []
    for label, rva, instruction in expected:
        ok = decoded.get(rva) == instruction
        checks.append(dict(relation=label,rva=rva,matched=ok))
    pe = PE(data)
    entry = 0x2701e0
    instruction_starts = sorted(rva for rva in decoded if entry <= rva < entry+18)
    lengths = [end-start for start,end in zip(instruction_starts,instruction_starts[1:]+[entry+18])]
    if not instruction_starts or instruction_starts[0] != entry or lengths != [2,1,8,7]:
        raise ValueError('actor entry instruction boundaries changed')
    record = pe.functions[entry]
    offset = pe.offset(record['unwind'],4)
    header = data[offset:offset+4]
    prefix = data[pe.offset(entry,11):pe.offset(entry,11)+11]
    candidate = dict(name='actor_simulation_start',entry_rva=entry,
                     first_instruction_lengths=lengths,minimum_five_byte_detour_overwrite=11,
                     overwrite_sha256=hashlib.sha256(prefix).hexdigest(),
                     overwrite_executable_match_count=len(pe.matches(' '.join(f'{byte:02x}' for byte in prefix))),
                     unwind_entry_end_rva=record['end'],unwind_info_rva=record['unwind'],
                     unwind_header=dict(version=header[0]&7,flags=header[0]>>3,
                                        prologue_size=header[1],code_count=header[2],
                                        frame_register=header[3]&15,frame_offset_scaled=header[3]>>4),
                     installation_status='NOT_INSTALLABLE',
                     missing=['Registered trampoline unwind metadata','ABI/preservation/relocation fixtures',
                              'Live session ownership, cadence and cleanup evidence'])
    return dict(schema='p0-static-scheduler-relations-v1',executable_sha256=digest,
                actor_entry_candidate=candidate,
                all_selected_relations_match=all(check['matched'] for check in checks),
                decode_input='private_read_only_snapshot_of_hashed_bytes',
                checks=checks, authority='STATIC_OPERAND_RELATIONS_ONLY',
                live_runtime='NOT_RUN',execution_profile_verdict='HOLD',
                limitations=['No code bytes or disassembly exported.',
                             'Selected instructions do not prove every control-flow path.',
                             'Original engine dispatch is not authority to inject arbitrary jobs.',
                             'Runtime owner/session identity, no-op cadence and cleanup remain untested.'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--historical-profile', type=Path)
    parser.add_argument('--scheduler-relations', action='store_true')
    args = parser.parse_args()
    try:
        if not args.scheduler_relations and args.historical_profile is None:
            parser.error('--historical-profile is required for profile scanning')
        result = inspect_scheduler_relations(args.executable) if args.scheduler_relations else inspect(args.executable, args.historical_profile)
        print(json.dumps(result, indent=2))
    except (OSError, ValueError, KeyError, struct.error, subprocess.SubprocessError) as error:
        parser.exit(2, f'Static inspection refused: {type(error).__name__}\n')


if __name__ == '__main__':
    main()
