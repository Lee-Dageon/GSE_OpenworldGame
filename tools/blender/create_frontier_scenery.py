"""Export seven village/forest landmarks using the shared quarter-view studio."""
from array import array
import json
import math
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
    source = ASSETS / "Source/scenery.blend"
    target = ASSETS / "Textures/scenery_atlas.png"
    metadata = target.with_suffix(".sprite.json")
    if any(path.exists() for path in (source, target, metadata)):
        raise FileExistsError("Existing scenery outputs are preserved")
    scene = helpers["studio"]("scenery", 256, 256, .9, 3.4)
    material, box, oval, bone, joint = (helpers[key] for key in ("material", "box", "oval", "bone", "joint"))
    wood = material("Weathered_Timber", (.20, .105, .045))
    cut = material("Cut_Endgrain", (.37, .25, .12))
    stone = material("Weathered_Granite", (.29, .30, .27))
    moss = material("Moss", (.13, .23, .07))
    leaf = material("Forest_Leaf", (.10, .21, .055))
    iron = material("Iron", (.07, .075, .065))
    for surface in (wood, cut, stone, moss, leaf):
        finish(surface, False)
    names = ("boulder", "bush", "log", "fence", "cart", "ruin", "pine")
    groups = [joint(name, (0, 0, 0), None) for name in names]
    for i, (x, y, size) in enumerate(((-.45, 0, .60), (.32, .15, .72), (.02, -.4, .36))):
        rock = oval("Boulder", (x, y, size*.55), (size, size*.7, size*.7), stone, groups[0])
        rock.rotation_euler.z = i*.7
        oval("Moss_Cap", (x-.07, y, size*1.12), (size*.65, size*.45, .07), moss, groups[0])
    for i in range(7):
        angle = i*2.4
        oval("Bush_Leaves", (math.cos(angle)*.35, math.sin(angle)*.25, .35+(i%3)*.13),
             (.40, .30, .30), leaf, groups[1])
    bone("Fallen_Trunk", (-.85, 0, .23), (.85, 0, .23), .26, wood, groups[2])
    bone("Cut_End", (-.86, 0, .23), (-.88, 0, .23), .22, cut, groups[2])
    bone("Branch", (.1, 0, .26), (.35, -.30, .65), .08, wood, groups[2])
    for x in (-.8, 0, .8):
        box("Fence_Post", (x, 0, .50), (.105, .12, 1), wood, groups[3])
    for height in (.35, .73):
        box("Fence_Rail", (0, -.035, height), (1.85, .09, .09), wood, groups[3])
    for plank in range(5):
        box("Cart_Bed", ((plank-2)*.21, 0, .53), (.20, .92, .08), wood, groups[4])
    for side in (-1, 1):
        box("Cart_Side", (side*.54, 0, .73), (.07, 1.03, .35), wood, groups[4])
        bone("Handle", (side*.40, -.4, .55), (side*.40, -1.20, .48), .04, wood, groups[4])
        bpy.ops.mesh.primitive_torus_add(major_segments=16, minor_segments=6, major_radius=.25,
            minor_radius=.035, location=(side*.65, .08, .30), rotation=(0, math.pi/2, 0))
        helpers["attach"](bpy.context.object, groups[4], iron)
        for angle in (0, math.pi/2, math.pi/4, -math.pi/4):
            bone("Wheel_Spoke", (side*.65, .08-math.cos(angle)*.25, .30-math.sin(angle)*.25),
                 (side*.65, .08+math.cos(angle)*.25, .30+math.sin(angle)*.25), .017, wood, groups[4])
    for i in range(3):
        height = .65+i*.35
        box("Broken_Pillar", ((i-1)*.48, .15*(i%2), height*.5), (.40, .43, height), stone, groups[5])
        oval("Pillar_Moss", ((i-1)*.48, .15*(i%2), height), (.24, .23, .055), moss, groups[5])
    bone("Pine_Trunk", (0, 0, 0), (0, 0, 1.9), .13, wood, groups[6])
    for i in range(4):
        bpy.ops.mesh.primitive_cone_add(vertices=10, radius1=.72-i*.14, radius2=0,
            depth=.9, location=(0, 0, 1.05+i*.38))
        helpers["attach"](bpy.context.object, groups[6], leaf)
    for group in groups:
        for obj in group.children_recursive:
            if obj.type == "MESH" and len(obj.data.polygons) == 6:
                bevel = obj.modifiers.new("Worn_Edge", "BEVEL")
                bevel.width, bevel.segments = .035, 2
    width = 256 * len(groups)
    pixels = array("f", [0]) * (width * 256 * 4)
    with tempfile.TemporaryDirectory(prefix="gse-scenery-") as temporary:
        for column, active in enumerate(groups):
            for group in groups:
                for obj in group.children_recursive:
                    obj.hide_render = group != active
            scene.render.filepath = str(Path(temporary) / (str(column)+".png"))
            bpy.ops.render.render(write_still=True)
            image = bpy.data.images.load(scene.render.filepath, check_existing=False)
            frame = array("f", [0]) * (256*256*4)
            image.pixels.foreach_get(frame)
            assert min(frame[3::4]) == 0 and max(frame[3::4]) > .99
            for y in range(256):
                start = (y*width + column*256)*4
                pixels[start:start+1024] = frame[y*1024:(y+1)*1024]
            bpy.data.images.remove(image)
    atlas = bpy.data.images.new("Frontier_Scenery", width=width, height=256, alpha=True)
    atlas.pixels.foreach_set(pixels)
    atlas.filepath_raw, atlas.file_format = str(target), "PNG"
    atlas.save()
    pivot = world_to_camera_view(scene, scene.camera, Vector((0, 0, 0)))
    metadata.write_text(json.dumps(dict(cell_width=256, cell_height=256, columns=len(groups), rows=1,
        pivot_y_from_bottom=round(pivot.y, 8), camera_elevation_degrees=30,
        columns_description=names, source="../Source/scenery.blend"), indent=2)+"\n")
    for group in groups:
        for obj in group.children_recursive:
            obj.hide_render = group != groups[0]
    bpy.ops.wm.save_as_mainfile(filepath=str(source))
    print("GSE_SCENERY_EXPORT", round(pivot.y, 8), flush=True)


if __name__ == "__main__":
    main()
