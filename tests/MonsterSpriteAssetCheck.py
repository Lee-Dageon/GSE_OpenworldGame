"""Static monster asset/anchor checks. Does not build or run the game."""
from pathlib import Path
import hashlib
import json
import re
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[1]
game = root / "SimpleGame"
boar = json.loads((game / "Assets/PixelArt/Animals/boar-death-v1.json").read_text(encoding="utf-8"))
hd_dir = game / "Assets/PixelArt/Monsters/Chapter3"
hd = json.loads((hd_dir / "chatacabra-hd-v1.json").read_text(encoding="utf-8"))


def check_atlas(path, columns, rows, frames, dimensions, digest):
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    with Image.open(path) as image:
        assert image.mode == "RGBA" and list(image.size) == dimensions
        assert image.getchannel("A").getextrema() == (0, 255)
        silhouettes = []
        for row in range(rows):
            for column in range(columns):
                cell = image.crop((round(column * image.width / columns), round(row * image.height / rows),
                                   round((column + 1) * image.width / columns), round((row + 1) * image.height / rows)))
                mask = cell.getchannel("A").point(lambda value: 255 if value >= 128 else 0)
                bounds = mask.getbbox()
                assert bounds
                left, top, right, bottom = bounds
                assert 0 < left < right < cell.width and 0 < top < bottom < cell.height, (path.name, row, column, bounds)
                frame = frames[row * columns + column]
                assert abs(frame["x"] - (frame["body_box"][0] + frame["body_box"][2]) / (2 * cell.width)) < 1e-7
                assert abs(frame["y"] - frame["body_box"][3] / cell.height) < 1e-7
                assert 0 < frame["x"] < 1 and 0 < frame["y"] < 1
                silhouettes.append(mask.resize((128, 128)))
        for column in range(columns):
            assert any(ImageChops.difference(silhouettes[column], silhouettes[row * columns + column]).getbbox()
                       for row in range(1, rows)), (path.name, "unchanged animation", column)


boar_frames = [dict(frame, body_box=frame["box"]) for frame in boar["frames"]]
check_atlas(game / "Assets/PixelArt/Animals" / boar["image"], 4, 4, boar_frames, boar["dimensions"], boar["sha256"])
assert boar["frame_seconds"] == .2 and not boar["loop"] and boar["hold_last_frame"]
boar_header = (game / "BoarDeathFrames.h").read_text(encoding="utf-8")
entries = re.findall(r"\{([\d.]+)f, ([\d.]+)f, ([\d.]+)f\}", boar_header)
assert len(entries) == 16
for row, frame in zip(entries, boar["frames"]):
    assert all(abs(float(value) - frame[key]) < 1e-7 for value, key in zip(row, ("x", "y", "scale")))

assert len(hd["atlases"]) == 10
all_frames = [[None] * 32 for _ in range(5)]
for atlas in hd["atlases"]:
    check_atlas(hd_dir / atlas["image"], 4, 4, atlas["frames"], atlas["dimensions"], atlas["sha256"])
    assert atlas["dimensions"][0] / 4 > atlas["old_dimensions"][0] / 8
    assert atlas["body_pixel_ratio"] > 1.2
    for index, frame in enumerate(atlas["frames"]):
        row, column = divmod(index, 4)
        direction = atlas["half"] * 4 + column
        all_frames[atlas["action"]][row * 8 + direction] = frame
        assert .1 < frame["scale"] < 2
hd_header = (game / "ChatacabraHDFrames.h").read_text(encoding="utf-8")
entries = re.findall(r"\{([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f\}", hd_header)
assert len(entries) == 160
for row, frame in zip(entries, (frame for action in all_frames for frame in action)):
    assert all(abs(float(value) - frame[key]) < 1e-7 for value, key in zip(row, ("x", "y", "scale", "scale")))

source = (game / "WorldRenderer.cpp").read_text(encoding="utf-8")
assert "m_ChatHdTextures[hdAction][direction / 4]" in source and "column = direction % 4" in source
assert "ChatacabraHDFrames::Frames[hdAction][pose * 8 + direction]" in source
assert "DrawBoarDeath(p, state.monster.animationFacing" in source
assert "DrawBoarDeath(animal.position, facing" in source
assert "for (const auto &action : m_ChatHdTextures)" in source
assert "glDeleteTextures(1, &m_BoarDeathTexture)" in source
print("PASS: boar 16 death poses; Chatacabra 10 HD atlases/160 poses; transparency, gutters, anchors, resolution gain and render paths")
print("Chatacabra body pixel width ratios:", ", ".join(f'{a["image"]}: {a["body_pixel_ratio"]:.2f}x' for a in hd["atlases"]))
