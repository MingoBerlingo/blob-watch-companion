"""End-to-end contract: real LVGL interactions, deterministic pixels, and circle mask."""
import binascii
import hashlib
import json
from pathlib import Path
import struct
import shutil
import zlib
import subprocess
import sys
import tempfile

def export_png(source):
    data = source.read_bytes()
    offset = struct.unpack_from('<I', data, 10)[0]
    rows = []
    for y in range(239, -1, -1):
        row = data[offset + y * 960:offset + (y + 1) * 960]
        rows.append(b'\0' + b''.join(bytes((row[x + 2], row[x + 1], row[x])) for x in range(0, 960, 4)))
    def chunk(kind, payload):
        body = kind + payload
        return struct.pack('>I', len(payload)) + body + struct.pack('>I', binascii.crc32(body) & 0xffffffff)
    source.with_suffix('.png').write_bytes(b'\x89PNG\r\n\x1a\n' +
        chunk(b'IHDR', struct.pack('>IIBBBBB', 240, 240, 8, 2, 0, 0, 0)) +
        chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b''))

executable = Path(sys.argv[1]).resolve()
output = Path(sys.argv[2]).resolve()
output.mkdir(parents=True, exist_ok=True)

def run(destination):
    subprocess.run([str(executable), '--test', '--output', str(destination)], check=True)
    report = json.loads((destination / 'report.json').read_text())
    assert report['passed'] >= 32, report
    assert report['failed'] == 0, report
    for name in ('blob', 'timer-setup', 'brightness', 'brightness-low', 'brightness-high', 'timer-running', 'timer-paused', 'timer-expired'):
        data = (destination / (name + '.bmp')).read_bytes()
        assert data[:2] == b'BM', name
        assert struct.unpack_from('<ii', data, 18) == (240, 240), name
        offset = struct.unpack_from('<I', data, 10)[0]
        assert struct.unpack_from('<H', data, 28)[0] == 32, name
        pixels = data[offset:]
        # A 32-bit SDL BMP encodes the round bezel in the exported pixels too.
        assert pixels[:3] == bytes((24, 24, 24)), name
        assert len(set(pixels)) > 8, name
    data = (destination / 'color-check.bmp').read_bytes()
    offset = struct.unpack_from('<I', data, 10)[0]
    index = offset + ((239 - 120) * 240 + 120) * 4
    assert data[index:index + 3] == bytes((255, 0, 123)), 'RGB565 byte order/color conversion'
    return report

run(output)
with tempfile.TemporaryDirectory() as directory:
    repeat = Path(directory)
    run(repeat)
    for path in output.glob('*.bmp'):
        assert path.read_bytes() == (repeat / path.name).read_bytes(), path.name
print('Simulator integration and deterministic screenshot checks passed')

# Intentional UI changes require an explicit baseline update after image inspection.
baseline_path = Path(__file__).with_name('visual-baselines.json')
hashes = {}
for path in sorted(output.glob('*.bmp')):
    data = path.read_bytes()
    offset = struct.unpack_from('<I', data, 10)[0]
    export_png(path)
    hashes[path.name] = hashlib.sha256(data[offset:offset + 240 * 240 * 4]).hexdigest()
if len(sys.argv) > 3 and sys.argv[3] == '--update-baselines':
    baseline_path.write_text(json.dumps(hashes, indent=2) + '\n')
    references = baseline_path.with_name('baselines')
    references.mkdir(exist_ok=True)
    for path in output.glob('*.png'):
        shutil.copyfile(path, references / path.name)
else:
    expected = json.loads(baseline_path.read_text())
    assert hashes == expected, 'Visual regression: inspect screenshots before updating visual-baselines.json'
print('Visual baselines match')
