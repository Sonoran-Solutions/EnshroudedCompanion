import hashlib
from pathlib import Path
import struct
import tempfile
import unittest
from tools.p0_fingerprint import collect


class FingerprintTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.manifest = root / 'manifest'
        self.binary = root / 'binary'
        self.manifest.write_text('"appid" "1203620" "buildid" "123" "LastOwner" "private"')
        self.data = bytearray(70)
        self.data[:2] = b'MZ'
        struct.pack_into('<I', self.data, 60, 64)
        self.data[64:] = b'PE\0\0\x64\x86'
        self.binary.write_bytes(self.data)

    def test_identity_without_private_fields_or_compatibility_claim(self):
        result = collect(self.manifest, self.binary)
        self.assertEqual(result['executable']['sha256'], hashlib.sha256(self.data).hexdigest())
        self.assertEqual(result['runtime_verdict'], 'NOT_RUN')
        self.assertEqual(result['steam'], {'appid': '1203620', 'buildid': '123'})
        self.assertNotIn('private', str(result))
        self.assertNotIn(self.temp.name, str(result))

    def test_invalid_binary_refused(self):
        self.binary.write_bytes(b'not a game')
        with self.assertRaises(ValueError):
            collect(self.manifest, self.binary)

    def test_ambiguous_or_wrong_manifest_refused(self):
        for text in ('"appid" "1" "buildid" "123"',
                     '"appid" "1203620" "buildid" "123" "buildid" "456"'):
            self.manifest.write_text(text)
            with self.assertRaises(ValueError):
                collect(self.manifest, self.binary)


if __name__ == '__main__':
    unittest.main()
