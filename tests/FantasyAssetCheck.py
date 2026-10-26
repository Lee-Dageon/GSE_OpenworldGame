"""Validate fantasy PNG/Blender exports without starting the game."""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
TEXTURES = ROOT / "SimpleGame/Assets/Textures/Fantasy"

for asset in ("terrain", "human", "boar", "deer", "sheep", "tree", "house"):
    atlas = asset not in ("tree", "house")
    name = asset + ("_atlas" if atlas else "")
    metadata = json.loads((TEXTURES / (name + ".sprite.json")).read_text())
    width = metadata.get("cell_width", metadata.get("width"))
    height = metadata.get("cell_height", metadata.get("height"))
    columns, rows = metadata.get("columns", 1), metadata.get("rows", 1)
    assert metadata["camera_elevation_degrees"] == 30
    source = (TEXTURES / metadata["source"]).resolve()
    assert source.is_relative_to(ROOT / "SimpleGame/Assets/Source/Fantasy")
    assert source.read_bytes().startswith(b"BLENDER")
    with Image.open(TEXTURES / (name + ".png")) as image:
        assert image.mode == "RGBA" and image.size == (width*columns, height*rows)
        for row in range(rows):
            for column in range(columns):
                cell = image.crop((column*width,row*height,(column+1)*width,(row+1)*height))
                alpha = cell.getchannel("A")
                assert alpha.getextrema() == (0, 255)
                if asset != "terrain":
                    bounds = alpha.getbbox()
                    assert 0 < bounds[0] < bounds[2] < width and 0 < bounds[1] < bounds[3] < height
    if asset in ("human", "boar", "deer", "sheep"):
        original = json.loads((TEXTURES.parent/(name+".sprite.json")).read_text())
        assert (width,height,columns,rows)==(128,192,8,4)
        assert abs(original["pivot_y_from_bottom"]-metadata["pivot_y_from_bottom"])<1e-7
    print("PASS: fantasy export", asset)
