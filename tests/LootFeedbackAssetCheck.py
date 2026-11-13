"""Check hit/loot resources and integration without building or launching the game."""
from pathlib import Path
import hashlib
import json
import re
import xml.etree.ElementTree as ET
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
game = root / "SimpleGame"
animals = game / "Assets/PixelArt/Animals"
items = game / "Assets/PixelArt/Items"
metadata = json.loads((animals / "animal-hit-v1.json").read_text())
assert [a["species"] for a in metadata["atlases"]] == ["deer", "boar", "sheep"]
entries = re.findall(r"\{([\d.]+)f, ([\d.]+)f, ([\d.]+)f\}", (game / "AnimalHitFrames.h").read_text())
assert len(entries) == 48
index = 0
for atlas in metadata["atlases"]:
    path = animals / atlas["image"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == atlas["sha256"]
    with Image.open(path) as image:
        assert image.mode == "RGBA" and list(image.size) == atlas["dimensions"]
        alpha = image.getchannel("A")
        assert alpha.getextrema() == (0, 255)
        silhouettes = []
        for row in range(4):
            for column in range(4):
                cell = alpha.crop((round(column * image.width / 4), round(row * image.height / 4),
                                   round((column + 1) * image.width / 4), round((row + 1) * image.height / 4)))
                mask = cell.point(lambda a: 255 if a >= 128 else 0)
                bounds = mask.getbbox()
                assert bounds and 0 < bounds[0] < bounds[2] < cell.width and 0 < bounds[1] < bounds[3] < cell.height, (atlas["species"], row, column, bounds)
                frame = atlas["frames"][row * 4 + column]
                assert list(bounds) == frame["box"]
                assert abs(frame["x"] - (bounds[0] + bounds[2]) / (2 * cell.width)) < 1e-7
                assert abs(frame["y"] - bounds[3] / cell.height) < 1e-7
                assert all(abs(float(v) - frame[key]) < 1e-7 for v, key in zip(entries[index], ("x", "y", "scale")))
                assert .1 < frame["scale"] < 3
                index += 1
                silhouettes.append(mask.resize((128, 128)))
        for column in range(4):
            assert ImageChops.difference(silhouettes[column], silhouettes[8 + column]).getbbox()

loot = json.loads((items / "loot-items-v1.json").read_text())
assert {item["id"] for item in loot["items"]} == {"raw-meat", "wool", "antler", "beast-bone", "guardian-scale", "ancient-crystal"}
inventory = (game / "Inventory.h").read_text(encoding="utf-8")
for item in loot["items"]:
    path = items / item["image"]
    assert item["image"] in inventory
    assert hashlib.sha256(path.read_bytes()).hexdigest() == item["sha256"]
    with Image.open(path) as image:
        assert image.mode == "RGBA" and list(image.size) == item["dimensions"]
        mask = image.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
        box = mask.getbbox()
        assert box and 0 < box[0] < box[2] < image.width and 0 < box[1] < box[3] < image.height
effects = game / "Assets/PixelArt/Effects"
fx = json.loads((effects / "quest-glimmer.json").read_text())["meta"]["pixelfx"]
assert fx["frames"] == 72 and fx["fps"] == 30 and fx["loop"] and fx["columns"] == 10
with Image.open(effects / "quest-glimmer.png") as image:
    assert image.mode == "RGBA" and image.size == (960, 896)
assert (effects / "quest-glimmer-LICENSE.txt").exists()
constants = (game / "GameConstants.h").read_text(encoding="utf-8")
assert "SpriteScale = 5.0f" in constants
source = (game / "WorldRenderer.cpp").read_text(encoding="utf-8")
assert "DrawAnimalHit(animal.position" in source and "DrawQuestGlimmer(StorySystem::Target(state), height)" in source
assert "DrawLoot(animal.position, LootTable::Animal(animal.column))" in source
assert "DrawLoot(state.monster.position, LootTable::Monster(state.monster.definition))" in source
for path in ("GameManager.cpp", "StorySystem.cpp", "QuestSystem.cpp"):
    assert "LootTable::Grant" in (game / path).read_text(encoding="utf-8")
assert "uiState.itemCounts = g_GameState.inventory.stacks" in (game / "SimpleGame.cpp").read_text(encoding="utf-8")
ui = (game / "GameUI.cpp").read_text(encoding="utf-8")
assert "state.itemCounts[item]" in ui and "state.inventoryPage" in ui
ns = {"m": "http://schemas.microsoft.com/developer/msbuild/2003"}
for filename in ("SimpleGame.vcxproj", "SimpleGame.vcxproj.filters"):
    included = {n.attrib.get("Include") for n in ET.parse(game / filename).findall(".//m:ClInclude", ns)}
    assert {"LootTable.h", "AnimalHitFrames.h"} <= included
print("PASS: 48 hit poses, 6 loot icons, 72-frame quest glimmer, 5.0 display scale, project registration and shared loot/UI paths")
