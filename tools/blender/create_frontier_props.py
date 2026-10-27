"""Export a worn noticeboard and herb patch with the character studio lighting."""
from array import array
import json
from pathlib import Path
import runpy
import tempfile
import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "SimpleGame/Assets/Frontier"
helpers = runpy.run_path(str(Path(__file__).with_name("create_world_assets.py")))
finish = runpy.run_path(str(Path(__file__).with_name("restyle_fantasy.py")))["finish_material"]


def main():
    source = ASSETS / "Source/props.blend"
    target = ASSETS / "Textures/props_atlas.png"
    metadata = target.with_suffix(".sprite.json")
    if any(path.exists() for path in (source, target, metadata)):
        raise FileExistsError("Existing frontier props are preserved")
    scene = helpers["studio"]("props", 256, 256, 1.0, 3.0)
    material, box, oval, bone, joint = (helpers[name] for name in ("material", "box", "oval", "bone", "joint"))
    wood = material("Weathered_Oak", (.20, .105, .042))
    edge = material("Dark_Endgrain", (.105, .054, .023))
    paper = material("Parchment", (.62, .51, .31))
    ink = material("Ink_Iron", (.075, .065, .047))
    leaf = material("Sage_Leaf", (.13, .25, .09))
    flower = material("Herb_Blossom", (.47, .34, .13))
    for surface in (wood, edge, paper, leaf):
        finish(surface, False)
    board = joint("Noticeboard", (0, 0, 0), None)
    for side in (-1, 1):
        box("Post", (side * .58, 0, .91), (.11, .12, 1.82), wood, board)
        box("Post_Cap", (side * .58, 0, 1.84), (.16, .16, .065), edge, board)
    for plank in range(5):
        box("Backing_Plank", ((plank-2)*.275, 0, 1.28), (.268, .085, .70), wood, board)
    for z in (.90, 1.66):
        box("Frame", (0, -.065, z), (1.54, .14, .09), edge, board)
    for side in (-1, 1):
        roof = box("Roof", (side * .37, 0, 1.87), (.85, .52, .055), wood, board)
        roof.rotation_euler.y = side * .22
    for note in range(3):
        x = (note - 1) * .39
        box("Paper", (x, -.069, 1.28 + (note % 2)*.035), (.30, .018, .43), paper, board)
        oval("Nail", (x, -.09, 1.47), (.018, .014, .018), ink, board)
        for line in range(4):
            box("Writing", (x, -.081, 1.37-line*.065), (.19-(line%2)*.05, .008, .012), ink, board)
    herb = joint("Herb_Patch", (0, 0, 0), None)
    for stem in range(5):
        x = (stem-2)*.12
        height = .42 + (stem%3)*.14
        bone("Stem", (x, 0, 0), (x+.035, 0, height), .018, edge, herb)
        for side in (-1, 1):
            petal = oval("Leaf", (x+side*.10, -.02, height*.58), (.16, .055, .075), leaf, herb)
            petal.rotation_euler.y = side * -.45
        oval("Blossom", (x+.035, 0, height), (.065, .06, .055), flower, herb)
    for root in (board, herb):
        for obj in root.children_recursive:
            if obj.type == "MESH":
                if len(obj.data.polygons) == 6:
                    bevel = obj.modifiers.new("Worn_Edge", "BEVEL")
                    bevel.width, bevel.segments = .025, 2
                else:
                    for face in obj.data.polygons:
                        face.use_smooth = True
    pixels = array("f", [0]) * (512 * 256 * 4)
    with tempfile.TemporaryDirectory(prefix="gse-frontier-props-") as temporary:
        for column, root in enumerate((board, herb)):
            for group in (board, herb):
                for obj in group.children_recursive:
                    obj.hide_render = group != root
            scene.render.filepath = str(Path(temporary) / (str(column)+".png"))
            bpy.ops.render.render(write_still=True)
            image = bpy.data.images.load(scene.render.filepath, check_existing=False)
            frame = array("f", [0]) * (256 * 256 * 4)
            image.pixels.foreach_get(frame)
            assert min(frame[3::4]) == 0 and max(frame[3::4]) > .99
            for y in range(256):
                start = (y*512 + column*256)*4
                pixels[start:start+1024] = frame[y*1024:(y+1)*1024]
            bpy.data.images.remove(image)
    atlas = bpy.data.images.new("Frontier_Props", width=512, height=256, alpha=True)
    atlas.pixels.foreach_set(pixels)
    atlas.filepath_raw, atlas.file_format = str(target), "PNG"
    atlas.save()
    pivot = world_to_camera_view(scene, scene.camera, Vector((0, 0, 0)))
    metadata.write_text(json.dumps(dict(cell_width=256, cell_height=256, columns=2, rows=1,
        pivot_y_from_bottom=round(pivot.y, 8), camera_elevation_degrees=30,
        columns_description=["noticeboard", "herb"], source="../Source/props.blend"), indent=2)+"\n")
    for group in (board, herb):
        for obj in group.children_recursive:
            obj.hide_render = group != board
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    print("GSE_FRONTIER_PROPS", round(pivot.y, 8), flush=True)


if __name__ == "__main__":
    main()
