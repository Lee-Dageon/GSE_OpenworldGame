"""Static asset/anchor checks; does not build or launch the game."""
from pathlib import Path
import re
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
header = (root / 'SimpleGame/PixelAnimalFrames.h').read_text(encoding='utf-8')
pivots = re.findall(r'\{([\d.]+)f, ([\d.]+)f\}', header)
assert len(pivots) == 48
for species_index, species in enumerate(('deer', 'boar', 'sheep')):
    path = root / f'SimpleGame/Assets/PixelArt/Animals/{species}-walk-v1.png'
    with Image.open(path) as image:
        assert image.mode == 'RGBA'
        alpha = image.getchannel('A').point(lambda value: 255 if value >= 128 else 0)
        assert alpha.getextrema() == (0, 255)
        width, height = image.width / 4, image.height / 4
        for direction in range(4):
            poses = []
            for frame in range(4):
                left, top = round(direction * width), round(frame * height)
                cell = alpha.crop((left, top, round((direction + 1) * width), round((frame + 1) * height)))
                box = cell.getbbox()
                assert box and box[2] - box[0] > width * .2 and box[3] - box[1] > height * .3
                px, py = map(float, pivots[species_index * 16 + frame * 4 + direction])
                assert abs(px * width - (left + (box[0] + box[2]) / 2 - direction * width)) < .01
                assert abs(py * height - (top + box[3] - frame * height)) < .01
                poses.append(cell.resize((128, 128)))
            assert all(ImageChops.difference(poses[frame], poses[(frame + 1) % 4]).getbbox() for frame in range(4))
    print(f'PASS: {species}: RGBA, 16 populated cells, measured anchors, distinct adjacent silhouettes')
source = (root / 'SimpleGame/WorldRenderer.cpp').read_text(encoding='utf-8')
assert 'DrawPixelAnimal(p, 1,' in source
assert 'DrawPixelAnimal(animal.position,' in source
print('PASS: ambient animals and combat boars use pixel rendering paths')
