from pathlib import Path
from PIL import Image
import hashlib
import json

root = Path(__file__).resolve().parents[1]
relative = Path('SimpleGame/Assets/PixelArt/Characters/hero-general-roll-v1.png')
source = root / relative
meta = json.loads(source.with_suffix('.json').read_text(encoding='utf-8'))
image = Image.open(source)
assert image.mode == 'RGBA' and image.size == (meta['width'], meta['height'])
assert hashlib.sha256(source.read_bytes()).hexdigest() == meta['sha256']
assert meta['columns'] == 6 and meta['rows'] == 8 and len(meta['frames']) == 8
assert len(set(meta['directions'])) == 8 and len(meta['scales']) == 8
alpha = image.getchannel('A')
assert alpha.getextrema() == (0, 255)
digests = set()
for row, frames in enumerate(meta['frames']):
    assert len(frames) == 6 and 0 < meta['scales'][row] < 2
    for column, frame in enumerate(frames):
        x, y, w, h = (frame[key] for key in ('x', 'y', 'width', 'height'))
        assert x >= column * image.width // 6 and x + w <= (column + 1) * image.width // 6
        assert y >= row * image.height // 8 and y + h <= (row + 1) * image.height // 8
        assert frame['anchorY'] == y + h and w > 40 and h > 40
        crop = image.crop((x, y, x + w, y + h))
        assert crop.getchannel('A').getbbox()
        digests.add(hashlib.sha256(crop.tobytes()).hexdigest())
assert len(digests) == 48
runtime = root / 'build/msbuild/x64/Debug/Assets/PixelArt/Characters'
for file in (source, source.with_suffix('.json')):
    copied = runtime / file.name
    assert copied.exists() and hashlib.sha256(copied.read_bytes()).digest() == hashlib.sha256(file.read_bytes()).digest()
renderer = (root / 'SimpleGame/WorldRenderer.cpp').read_text(encoding='utf-8-sig')
assert 'hero-general-roll-v1.png' in renderer and renderer.count('m_GeneralRollTexture') >= 5
print('PASS: 48 unique RGBA roll poses, source bounds/anchors, 8 directions, hashes and runtime PNG/JSON copy')
