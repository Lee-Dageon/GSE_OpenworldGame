"""Static checks only: content, target access, asset alpha/frames and project wiring."""
from pathlib import Path
import math
import shlex
import json
import hashlib
import xml.etree.ElementTree as ET
from PIL import Image

root = Path(__file__).resolve().parents[1]
game = root / "SimpleGame"
def records(name):
    return [shlex.split(line, comments=True) for line in (game / "Data" / name).read_text(encoding="utf-8-sig").splitlines()
            if line.strip() and not line.lstrip().startswith("#")]

story = records("story.gse")
steps = [r for r in story if r[0] == "step"]
chapters = {r[1] for r in story if r[0] == "chapter"}
monsters = {r[1] for r in records("monsters.gse") if r[0] == "monster"}
assert len(steps) == 39 and len(chapters) == 5
assert len({r[1] for r in steps}) == len(steps)
assert sum(r[3] == "rescue" for r in steps) == 2
for i, r in enumerate(steps):
    assert len(r) == 11 and r[2] in chapters
    if r[3] == "hunt":
        assert r[7] in monsters and steps[i+1][3] == "collect"
    if r[3] == "route_choice":
        assert [s[3] for s in steps[i+1:i+3]] == ["hunt", "collect"]
        assert steps[i+3][1] == "drought_control"
assert steps[-1][3] == "final_choice"

map_rows = records("map.gse")
def blocked(x, z):
    for r in map_rows:
        if r[0] == "house":
            if abs(x-float(r[2])) < float(r[7]) and abs(z-float(r[3])) < float(r[8]):
                return True
        elif r[0] == "tree":
            if (x-float(r[2]))**2+(z-float(r[3]))**2 < float(r[-1])**2:
                return True
        elif r[0] == "landmark":
            radius = float(r[6])
            if radius > 0 and (x-float(r[3]))**2+(z-float(r[4]))**2 < (radius+.18)**2:
                return True
        elif r[0] == "lake":
            if (x-float(r[1]))**2+(z-float(r[2]))**2 <= float(r[3])**2:
                return True
        elif r[0] == "terrain" and int(r[2]) != 0:
            ax, az, bx, bz, width = map(float, r[3:8])
            length = math.hypot(bx-ax, bz-az)
            along = ((x-ax)*(bx-ax)+(z-az)*(bz-az))/length
            side = abs((z-az)*(bx-ax)-(x-ax)*(bz-az))/length
            if length*.04 < along < length*.96 and width < side < width+8:
                return True
    return False

for r in steps:
    x, z, radius = map(float, r[4:7])
    assert -900 <= x <= 900 and -900 <= z <= 900 and .1 <= radius <= 10
    assert not blocked(x, z), f"blocked goal: {r[1]}"
    # Every target has at least one unobstructed local approach.
    assert any(all(not blocked(x+math.cos(a)*d, z+math.sin(a)*d) for d in (.5,1,2,3))
               for a in (i*math.pi/4 for i in range(8))), r[1]

fields = [r for r in map_rows if r[0] == "field_monster"]
assert len(fields) == 15 and len({r[1] for r in fields}) == 15
for r in fields:
    assert r[2] in monsters and not blocked(float(r[3]), float(r[4]))

folder = game / "Assets/PixelArt/Investigation"
metadata = json.loads((folder / "endgame-assets.json").read_text(encoding="utf-8"))
for entry in metadata["assets"]:
    path = folder / entry["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == entry["sha256"]
    with Image.open(path) as image:
        assert image.mode == "RGBA" and list(image.size) == entry["dimensions"]
        assert image.getchannel("A").getextrema() == (0,255)
        for row in range(entry["rows"]):
            for col in range(entry["columns"]):
                cell = image.crop((col*image.width//entry["columns"], row*image.height//entry["rows"],
                                   (col+1)*image.width//entry["columns"], (row+1)*image.height//entry["rows"]))
                assert cell.getchannel("A").getbbox(), (row, col)

namespace = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}
for name in ("SimpleGame.vcxproj", "SimpleGame.vcxproj.filters"):
    project = ET.parse(game / name)
    assert any(n.attrib.get("Include") == "SaveGame.h" for n in project.findall(".//ms:ClInclude", namespace))
source = (game / "WorldRenderer.cpp").read_text(encoding="utf-8")
assert "glDeleteTextures(1,&m_EcologyGuardianTexture)" in source
assert "ecology-guardian-v1.png" in source
assert "progress.bak" in (game / "SaveGame.h").read_text(encoding="utf-8")
item_folder=game/"Assets/PixelArt/Items"
regional_items=json.loads((item_folder/"region-items.json").read_text(encoding="utf-8"))
assert len(regional_items["assets"])==2
for asset in regional_items["assets"]:
    path=item_folder/asset["file"]
    assert hashlib.sha256(path.read_bytes()).hexdigest()==asset["sha256"]
    with Image.open(path) as image:
        assert image.mode=="RGBA" and list(image.size)==asset["dimensions"] and image.getchannel("A").getextrema()==(0,255)
assert (game/"Data/Regions/index.gse").is_file() and (game/"Data/Regions/frost_reach.gse").is_file()
poses=sum(e["rows"]*e["columns"] for e in metadata["assets"])
print(f"PASS static: 5 chapters/39 goals, 15 field encounters, branch merge, 39 target/local approaches, {poses+2} RGBA poses, regional files and save/project wiring")
