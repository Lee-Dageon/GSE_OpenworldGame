"""Check investigation asset coverage and render anchors without launching the game."""
from pathlib import Path
import hashlib
import json
import re
import shlex
import xml.etree.ElementTree as ET
from PIL import Image

root = Path(__file__).resolve().parents[1]
assets_dir = root / "SimpleGame/Assets/PixelArt/Investigation"
manifest = json.loads((assets_dir / "manifest.json").read_text(encoding="utf-8"))
header = (root / "SimpleGame/InvestigationSprites.h").read_text(encoding="utf-8")
assets = manifest["assets"]
assert len(assets) == 7
assert len({asset["sha256"] for asset in assets}) == 7
rows = re.findall(r'\{L"([^"]+)", ([\d.]+)f, ([\d.]+)f, ([\d.]+)f\}', header)
assert len(rows) == len(assets)
for asset, row in zip(assets, rows):
    path = assets_dir / asset["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == asset["sha256"]
    with Image.open(path) as image:
        assert image.mode == "RGBA" and list(image.size) == asset["dimensions"]
        alpha = image.getchannel("A")
        assert alpha.getextrema() == (0, 255)
        mask = alpha.point(lambda value: 255 if value >= 128 else 0)
        bounds = mask.getbbox()
        assert list(bounds) == asset["visible_bounds"]
        left, top, right, bottom = bounds
        assert 0 < left < right < image.width and 0 < top < bottom < image.height
        assert left / image.width < asset["pivot"][0] < right / image.width
        assert top / image.height < asset["pivot"][1] < bottom / image.height
        for x, y in ((0, 0), (image.width - 1, 0), (0, image.height - 1), (image.width - 1, image.height - 1)):
            assert alpha.getpixel((x, y)) < 128
    assert row[0] == asset["file"] and float(row[1]) == asset["size"]
    assert all(abs(float(row[index + 2]) - pivot) < 1e-7 for index, pivot in enumerate(asset["pivot"]))

steps = [shlex.split(line) for line in (root / "SimpleGame/Data/story.gse").read_text(encoding="utf-8").splitlines()
         if line.startswith("step ")]
expected = {step[1] for step in steps if step[3] in ("travel", "investigate")}
sites = manifest["sites"]
mapping = {site["step_id"]: site["asset"] for site in sites}
assert len(mapping) == len(sites) == 12 and set(mapping) == expected
assert dict((step, int(asset)) for step, asset in re.findall(r'\{"([^"]+)", (\d+)\}', header)) == mapping
assert all(0 <= index < len(assets) for index in mapping.values())
assert mapping["ruin_control"] == 6 and assets[0]["id"] == "tracks"
source = (root / "SimpleGame/WorldRenderer.cpp").read_text(encoding="utf-8")
assert "DrawType::Investigation, index" in source
assert "DrawInvestigation(item.position, 0)" in source
assert "control && state.ruinDeviceDisabled" in source
assert "control && !state.ruinDeviceDisabled" in source
assert "for (auto texture : m_InvestigationTextures)" in source
assert "1, tint, tint, tint, 0, 0, 0, 1, 1, .5f" in source
copy = (root / "tools/CopyRuntime.ps1").read_text(encoding="utf-8")
assert "'Assets'" in copy and "-Recurse -File" in copy and "'.png', '.json'" in copy
namespace = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}
for filename in ("SimpleGame.vcxproj", "SimpleGame.vcxproj.filters"):
    project = ET.parse(root / "SimpleGame" / filename)
    assert any(node.attrib.get("Include") == "InvestigationSprites.h"
               for node in project.findall(".//ms:ClInclude", namespace))
print("PASS: 7 distinct RGBA assets, measured anchors, 12/12 travel/investigation sites, tutorial tracks, device state, texture release and runtime-copy wiring")
