"""Read PNG data only; no game launch, image output, or screen capture."""
from pathlib import Path
import re

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "SimpleGame/HorseSpriteFrames.h").read_text(encoding="utf-8-sig")


def values(name):
    return [int(value) for value in re.search(rf"{name}\[Rows\] = \{{([^}}]+)\}}", HEADER)[1].split(",")]


for filename, top_name, ground_name in (("horse-gallop-v1.png", "HorseTop", "Ground"),
                                       ("hero-mounted-v1.png", "RiderTop", "Ground"),
                                       ("horse-idle-v1.png", "IdleTop", "IdleGround")):
    image = Image.open(ROOT / "SimpleGame/Assets/PixelArt/Mounts" / filename)
    assert image.mode == "RGBA" and image.size == (887, 1774), filename
    top = values(top_name)
    ground = values(ground_name)
    directions = []
    for row in range(8):
        phases = []
        for frame in range(4):
            x, right = frame * 887 // 4, (frame + 1) * 887 // 4
            y, bottom = row * 1774 // 8 + top[row], row * 1774 // 8 + ground[row] + 2
            cell = np.asarray(image.crop((x, y, right, bottom)))
            visible = cell[:, :, 3] >= 128
            assert visible.sum() > 3000, (filename, row, frame, "missing figure")
            assert not any((visible[0].any(), visible[-1].any(), visible[:, 0].any(), visible[:, -1].any())), (
                filename, row, frame, "opaque sprite touches its render bounds")
            assert (cell[:, :, 3] == 0).any(), "missing transparent gutter"
            phases.append(cell.tobytes())
        assert len(set(phases)) == 4, (filename, row, "repeated animation phase")
        directions.append(phases[0])
    assert len(set(directions)) == 8, (filename, "repeated direction")
    print(f"PASS {filename}: 32 distinct frames, RGBA, render bounds, transparent gutters")
