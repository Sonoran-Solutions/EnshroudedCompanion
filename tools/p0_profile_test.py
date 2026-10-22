#!/usr/bin/env python3
"""Synthetic-PE regression tests for the static evidence collector."""
import struct
import unittest
import tempfile
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from p0_profile_inspect import PE, inspect_scheduler_relations, decode_snapshot


def fixture():
    data=bytearray(0x600)
    data[:2]=b'MZ';struct.pack_into('<I',data,60,0x80)
    data[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HHI',data,0x84,0x8664,2,123)
    struct.pack_into('<H',data,0x94,0xf0)
    opt=0x98
    struct.pack_into('<H',data,opt,0x20b)
    struct.pack_into('<Q',data,opt+24,0x140000000)
    struct.pack_into('<I',data,opt+56,0x3000)
    struct.pack_into('<II',data,opt+112+24,0x2000,12)
    for pos,name,rva,raw,flags in [(0x188,b'.text',0x1000,0x200,0x20000000),(0x1b0,b'.rdata',0x2000,0x400,0)]:
        data[pos:pos+len(name)]=name
        struct.pack_into('<IIII',data,pos+8,0x200,rva,0x200,raw)
        struct.pack_into('<I',data,pos+36,flags)
    data[0x200:0x208]=b'ABABABCD'
    data[0x210:0x213]=b'\x48\x8d\x05'
    struct.pack_into('<i',data,0x213,0x2040-(0x1010+7))
    data[0x230]=0xe8
    struct.pack_into('<i',data,0x231,0x1010-(0x1030+5))
    struct.pack_into('<III',data,0x400,0x1000,0x1008,0x2010)
    data[0x420:0x425]=b'test\0'
    values=[0x140002020,5,0x140001000]+[0]*12
    values[7:9]=[0x1400020c0,1]
    struct.pack_into('<15Q',data,0x440,*values)
    struct.pack_into('<QQ',data,0x4c0,0x1400020e0,6)
    data[0x4e0:0x4e6]=b'Health'
    return bytes(data)


class StaticTests(unittest.TestCase):
    def test_overlapping_and_wildcard(self):
        pe=PE(fixture())
        self.assertEqual(pe.matches('41 42 41 42'),[0x1000,0x1002])
        self.assertEqual(pe.matches('41 ?? 43 44'),[0x1004])
        self.assertEqual(pe.matches('48 65 61 6c 74 68'),[])  # Non-executable data.

    def test_descriptor_requires_unwind_and_dependencies(self):
        entries=PE(fixture()).descriptors()
        self.assertEqual(len(entries),1)
        self.assertEqual(entries[0]['name'],'test')
        self.assertEqual(entries[0]['callback_rva'],0x1000)
        self.assertFalse(entries[0]['callable'])
        broken=bytearray(fixture());struct.pack_into('<Q',broken,0x450,0x140001001)
        self.assertEqual(PE(bytes(broken)).descriptors(),[])

    def test_reference_candidates_are_not_authority(self):
        pe=PE(fixture())
        self.assertEqual(pe.rip_reference_candidates(0x2040),[0x1010])
        self.assertEqual(pe.relative_call_candidates(0x1010),[0x1030])
        self.assertEqual(pe.relative_call_candidates(0x1000),[])

    def test_decoder_uses_private_snapshot_then_removes_it(self):
        payload=fixture()
        observed=[]
        def decode(command, **options):
            snapshot=Path(command[-1])
            self.assertEqual(snapshot.read_bytes(),payload)
            self.assertEqual(snapshot.stat().st_mode & 0o777,0o400)
            self.assertEqual(options['timeout'],60)
            observed.append(snapshot)
            return SimpleNamespace(stdout='  140001000: ret\n')
        with patch('p0_profile_inspect.subprocess.run',side_effect=decode):
            self.assertEqual(decode_snapshot(payload,[(0x1000,0x1001)]),{0x1000:'ret'})
        self.assertFalse(observed[0].exists())

    def test_scheduler_map_refuses_other_binary(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'synthetic.exe';path.write_bytes(fixture())
            with self.assertRaises(ValueError):inspect_scheduler_relations(path)

    def test_invalid_descriptor_pointer_rejected(self):
        broken=bytearray(fixture())
        struct.pack_into('<Q',broken,0x440+7*8,0x140004000)
        self.assertEqual(PE(bytes(broken)).descriptors(),[])

    def test_invalid_header_offset_rejected(self):
        broken=bytearray(fixture());struct.pack_into('<I',broken,60,0xffffff00)
        with self.assertRaises(ValueError):PE(bytes(broken))

    def test_truncated_file_rejected(self):
        with self.assertRaises(ValueError):PE(fixture()[:-1])


if __name__=='__main__':unittest.main()
