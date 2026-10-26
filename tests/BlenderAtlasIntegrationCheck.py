"""Check exported assets against the renderer contract, without building/running the game.

Uses Pillow, already present in the asset-check environment.
Run: python tests/BlenderAtlasIntegrationCheck.py
"""

import json
import math
from pathlib import Path
import re

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "SimpleGame/WorldRenderer.cpp").read_text(encoding="utf-8-sig")


def constant(name):
    return float(re.search(r"constexpr float " + name + r" = ([.0-9]+)f;", SOURCE).group(1))


def main():
    for name in ("terrain", "human", "boar", "deer", "sheep"):
        path = ROOT / "SimpleGame/Assets/Textures" / (name + "_atlas.png")
        metadata = json.loads(path.with_suffix(".sprite.json").read_text())
        with Image.open(path) as image:
            columns, rows = metadata["columns"], metadata["rows"]
            width, height = metadata["cell_width"], metadata["cell_height"]
            assert image.size == (width * columns, height * rows) and image.mode == "RGBA"
            assert metadata["camera_elevation_degrees"] == 30
            assert metadata["row_order"] == "top_to_bottom"
            if name == "terrain":
                assert (columns, rows) == (3, 1)
                assert metadata["columns_description"] == ["grass", "dirt", "stone"]
                assert width / height == constant("TileWidth") / constant("TileHeight") == 2
            else:
                assert (width, height, columns, rows) == (128, 192, 8, 4)
                pivot = constant("HumanPivotY" if name == "human" else "AnimalPivotY")
                assert abs(metadata["pivot_y_from_bottom"] - pivot) < 1e-7
                assert abs(metadata["pivot_x"] - .5) < 1e-6
                for size in (88, 100, 110, 125, 128):
                    center = (.5 - pivot) * size
                    assert abs(center + (pivot - .5) * size) < 1e-10
            for row in range(rows):
                for column in range(columns):
                    cell = image.crop((column * width, row * height, (column + 1) * width, (row + 1) * height))
                    assert cell.getchannel("A").getextrema() == (0, 255)
        print("PASS: asset layout, alpha, renderer pivot:", name)

    # Independent direction contract: turning each exported Blender front must project
    # onto the requested game-space direction. This does not execute compiled C++.
    directions = ((-1, 0), (-1, -1), (0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1))
    for column, (x, z) in enumerate(directions):
        angle = column * math.pi / 4
        length = math.hypot(x, z)
        assert abs(-math.cos(angle) - x / length) < 1e-12
        assert abs(-math.sin(angle) - z / length) < 1e-12
    for x, z in ((-65, 85), (-1, -1), (0, 0), (12.5, 23.75)):
        sx, sy = (x - z) * constant("TileWidth") / 2, (x + z) * constant("TileHeight") / 2
        assert abs(sx / constant("TileWidth") + sy / constant("TileHeight") - x) < 1e-12
        assert abs(sy / constant("TileHeight") - sx / constant("TileWidth") - z) < 1e-12
        local_x, local_z = ((value / 2 % 1) * 2 - 1 for value in (x, z))
        u, v = .5 + (local_x - local_z) * .25, .5 - (local_x + local_z) * .25
        assert 0 <= u <= 1 and 0 <= v <= 1
    print("PASS: 8-direction projection and terrain inverse projection contracts")


if __name__ == "__main__":
    main()
