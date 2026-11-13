"""Offline asset validation; does not build or launch the game."""
import hashlib
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "SimpleGame/Assets/PixelArt/Effects"
renderer = (ROOT / "SimpleGame/WorldRenderer.cpp").read_text(encoding="utf-8-sig")
manifest = json.loads((ASSETS / "chatacabra-hard-v1.json").read_text(encoding="utf-8"))
for asset in manifest["assets"]:
    path = ASSETS / asset["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == asset["sha256"], path
    assert asset["file"] in renderer, f"not loaded: {path}"
    with Image.open(path) as image:
        assert image.mode == "RGBA" and image.getchannel("A").getextrema() == (0, 255), path
        if "metadata" not in asset:
            continue
        data = json.loads((ASSETS / asset["metadata"]).read_text())["meta"]["pixelfx"]
        width, height = data["frameSize"]
        columns, count = data["columns"], data["frames"]
        assert image.size == (width * columns, height * ((count + columns - 1) // columns)), path
        assert 0 <= data["pivot"][0] < width and 0 <= data["pivot"][1] < height, path
        populated = 0
        for frame in range(count):
            x, y = frame % columns * width, frame // columns * height
            if image.crop((x, y, x + width, y + height)).getchannel("A").getbbox():
                populated += 1
        assert populated >= count * .8, (path, populated, count) # One-shots include a transparent tail.
        if not data["loop"]:
            x, y = (count - 1) % columns * width, (count - 1) // columns * height
            assert image.crop((x, y, x + width, y + height)).getchannel("A").getbbox() is None, path
assert "Hugo Duprez" in (ASSETS / "chatacabra-hard-LICENSE.txt").read_text()
assert "chatacabra-hard-LICENSE.txt" in (ROOT / "tools/CopyRuntime.ps1").read_text(encoding="utf-8-sig")
print("PASS: rage emblem + 3 VFX atlases, hashes, alpha, frame geometry, pivots, end frames, renderer and license wiring")
