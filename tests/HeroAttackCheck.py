"""Check exported attack frames and their alignment with the walking hero."""
import json
from pathlib import Path
import re

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "SimpleGame/Assets/Hero"
metadata = json.loads((ASSETS / "Textures/human_attack_atlas.sprite.json").read_text())
walking = json.loads((ASSETS / "Textures/human_atlas.sprite.json").read_text())
assert metadata["columns"] == metadata["rows"] == 8
assert metadata["duration_seconds"] == .32
assert metadata["row_order"] == "top_to_bottom"
assert (ASSETS / "Source/human_attack.blend").read_bytes().startswith(b"BLENDER")
image = Image.open(ASSETS / "Textures/human_attack_atlas.png")
assert image.mode == "RGBA" and image.size == (2048, 2048)
first_poses = []
for direction in range(8):
    poses = []
    for frame in range(8):
        cell = image.crop((direction*256, frame*256, (direction+1)*256, (frame+1)*256))
        alpha = cell.getchannel("A")
        assert alpha.getextrema() == (0, 255), (direction, frame, "empty or opaque frame")
        bounds = alpha.getbbox()
        assert all(value > 1 for value in bounds[:2]) and all(value < 255 for value in bounds[2:]), (direction, frame, "clipped pose")
        poses.append(cell.tobytes())
    assert len(set(poses)) >= 6, (direction, "missing attack motion")
    assert poses[0] == poses[-1], (direction, "attack does not return to rest")
    first_poses.append(poses[0])
assert len(set(first_poses)) == 8, "directions are duplicated"
ratio = metadata["display_height_ratio"]
assert abs(ratio - 256/192) < 1e-7
assert abs((.5-metadata["pivot_y_from_bottom"])*ratio - (.5-walking["pivot_y_from_bottom"])) < 1e-7
source = (ROOT / "SimpleGame/WorldRenderer.cpp").read_text(encoding="utf-8-sig")
pivot = float(re.search(r'pivotY = ([.0-9]+)f;', source).group(1))
assert abs(pivot-metadata["pivot_y_from_bottom"]) < 1e-7
print("PASS: 64 frames, eight distinct directions, attack poses, recovery, alpha bounds, body scale and pivot")
