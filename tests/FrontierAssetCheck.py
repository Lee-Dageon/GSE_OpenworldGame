"""Validate the new NPC/prop sheets without launching Blender or the game."""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "SimpleGame/Assets/Frontier"
for name, columns, rows in (("human", 8, 4), ("props", 2, 1)):
    path = ASSETS / "Textures" / (name + "_atlas.png")
    metadata = json.loads(path.with_suffix(".sprite.json").read_text())
    image = Image.open(path)
    width, height = metadata["cell_width"], metadata["cell_height"]
    assert image.mode == "RGBA" and image.size == (width * columns, height * rows)
    assert metadata["columns"] == columns and metadata["rows"] == rows
    assert metadata["camera_elevation_degrees"] == 30
    assert (path.parent / metadata["source"]).read_bytes().startswith(b"BLENDER")
    for row in range(rows):
        for column in range(columns):
            cell = image.crop((column*width, row*height, (column+1)*width, (row+1)*height))
            alpha = cell.getchannel("A")
            assert alpha.getextrema() == (0, 255), (name, row, column, "missing geometry/alpha")
            bounds = alpha.getbbox()
            assert min(bounds[:2]) > 1 and bounds[2] < width-1 and bounds[3] < height-1, (name, "clipped frame")
    if name == "human":
        hero = json.loads((ROOT / "SimpleGame/Assets/Hero/Textures/human_atlas.sprite.json").read_text())
        assert abs(metadata["pivot_y_from_bottom"]-hero["pivot_y_from_bottom"]) < 1e-7
        assert len({image.crop((c*width, 0, (c+1)*width, height)).tobytes() for c in range(8)}) == 8
        for column in range(columns):
            assert len({image.crop((column*width,r*height,(column+1)*width,(r+1)*height)).tobytes()
                        for r in range(rows)}) >= 3
    else:
        assert abs(metadata["pivot_y_from_bottom"]-.21132453) < 1e-7
    print("PASS: frontier", name, "layout, source, alpha, bounds, camera and pivot")
