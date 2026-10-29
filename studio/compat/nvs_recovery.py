#!/usr/bin/env python3
"""Generate a Studio-only copy of pinned Zephyr NVS with one recovery correction.

After a torn write fills the remaining data gap, nvs_startup's comparison loop
can leave rc=1 (non-empty, not an I/O error). Returning that value makes mounting
fail and prevents Bluetooth settings from loading. Negative errors already jump
to end. Clear only the successful scan result; keep real errors and disk format.
Never edit the upstream checkout, and refuse an unrecognized source revision.
"""
from pathlib import Path
import sys

needle = '''\t\tfs->data_wra += fs->flash_parameters->write_block_size;
\t}

\t/* If the ate_wra is pointing to the first ate write location in a'''
replacement = '''\t\tfs->data_wra += fs->flash_parameters->write_block_size;
\t}

\t/* Studio: a completed scan is successful even when the last comparison
\t * found non-empty bytes (rc == 1). Actual read errors jump to end above.
\t */
\trc = 0;

\t/* If the ate_wra is pointing to the first ate write location in a'''

def patched(source: str) -> str:
    if source.count(needle) != 1:
        raise ValueError('NVS recovery source changed; review the compatibility fix before building')
    result = source.replace(needle, replacement, 1)
    mount = 'int nvs_mount(struct nvs_fs *fs)'
    if result.count(mount) != 1:
        raise ValueError('Pinned NVS mount changed; review the error guard')
    result = result.replace(mount, 'static int ks_nvs_mount_standard(struct nvs_fs *fs)', 1)
    return result + '\nstatic int ks_nvs_error;\nint keebart_storage_error(void){return ks_nvs_error;}\nint nvs_mount(struct nvs_fs *fs){ks_nvs_error=ks_nvs_mount_standard(fs);return ks_nvs_error;}\n'

if __name__ == '__main__':
    original, destination = map(Path, sys.argv[1:])
    if original.resolve() == destination.resolve():
        raise ValueError('The upstream source is read-only')
    result = patched(original.read_text())
    if not destination.exists() or destination.read_text() != result:
        destination.write_text(result)
