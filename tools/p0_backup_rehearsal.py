#!/usr/bin/env python3
"""Private, copy-only save backup and restore rehearsal; never restore in place."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile
from datetime import datetime, timezone


def fingerprint(root):
    result = {}
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            raise RuntimeError('save tree contains a symbolic link; manual review needed')
        if path.is_file():
            before = path.stat()
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            after = path.stat()
            if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
                raise RuntimeError('save changed during read')
            result[str(path.relative_to(root))] = {'bytes': after.st_size, 'sha256': digest}
    return result


def require_game_closed():
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit():
            continue
        try:
            args = (proc/'cmdline').read_bytes().split(b'\0')
            comm = (proc/'comm').read_text().strip().lower()
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            continue
        if comm.startswith('enshrouded') or any(arg.replace(b'\\', b'/').rsplit(b'/', 1)[-1].lower() in (b'enshrouded.exe', b'enshrouded_server.exe') for arg in args):
            raise RuntimeError('game/server process present; close it before backup')


def main():
    os.umask(0o077)
    require_game_closed()
    steam = Path.home()/'.local/share/Steam'
    roots = sorted((steam/'userdata').glob('*/1203620'))
    saved = steam/'steamapps/compatdata/1203620/pfx/drive_c/users/steamuser/Saved Games'
    if saved.is_dir():
        roots.extend(p for p in saved.iterdir() if p.name.lower() == 'enshrouded' and p.is_dir())
    if not roots:
        raise RuntimeError('no expected save directory found')
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    private = Path.home()/'.local/share/EnshroudedCompanion/p0-backups'/stamp
    private.mkdir(parents=True, exist_ok=False)
    private_manifest = []
    for index, source in enumerate(roots):
        expected = fingerprint(source)
        backup = private/f'source-{index}'
        shutil.copytree(source, backup)
        if fingerprint(source) != expected or fingerprint(backup) != expected:
            raise RuntimeError('backup consistency check failed; retained for review')
        with tempfile.TemporaryDirectory(prefix='restore-rehearsal-', dir=private) as temporary:
            restored = Path(temporary)/'restored'
            shutil.copytree(backup, restored)
            if fingerprint(restored) != expected:
                raise RuntimeError('restore rehearsal mismatch')
        private_manifest.append({'source': str(source), 'backup': str(backup), 'files': expected})
    require_game_closed()
    for item in private_manifest:
        if fingerprint(Path(item['source'])) != item['files']:
            raise RuntimeError('source changed by end of backup')
    (private/'manifest-private.json').write_text(json.dumps(private_manifest, indent=2)+'\n')
    print(json.dumps({'schema': 'p0-copy-only-backup-rehearsal-v1', 'utc': stamp,
        'source_directories': len(roots), 'files': sum(len(x['files']) for x in private_manifest),
        'bytes': sum(f['bytes'] for x in private_manifest for f in x['files'].values()),
        'backup_hash_verified': True, 'restore_to_separate_directory_hash_verified': True,
        'active_save_restore': 'NOT_RUN', 'in_game_load_of_restored_save': 'NOT_RUN',
        'shared_world_reset': 'NOT_RUN', 'source_files_written': False,
        'private_backup_location': '~/.local/share/EnshroudedCompanion/p0-backups/'+stamp,
        'limitations': ['Steam Cloud behavior and game-level recovery are not verified by a byte-copy rehearsal.']}, indent=2))


if __name__ == '__main__':
    main()
