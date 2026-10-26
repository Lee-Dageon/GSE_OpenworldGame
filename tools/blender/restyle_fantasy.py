"""Render textured fantasy derivatives of existing Blender assets; preserve originals."""

from array import array
import argparse
import json
import math
from pathlib import Path
import random
import sys
import tempfile

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "SimpleGame/Assets"


def finish_material(surface, terrain):
    if not surface.use_nodes:
        return
    nodes = surface.node_tree.nodes
    shader = nodes.get("Principled BSDF")
    if shader is None:
        return
    base = tuple(shader.inputs["Base Color"].default_value)
    label = surface.name.lower()
    if terrain:
        base = (*((.095, .22, .035) if "grass" in label else
                   ((.24, .15, .065) if "dirt" in label else (.25, .27, .24))), 1)
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 34 if terrain else 24
    noise.inputs["Detail"].default_value = 4
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = .20
    ramp.color_ramp.elements[0].color = tuple(c * .55 for c in base[:3]) + (1,)
    ramp.color_ramp.elements[1].position = .8
    ramp.color_ramp.elements[1].color = tuple(min(c * 1.35, 1) for c in base[:3]) + (1,)
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = .22 if terrain else .14
    bump.inputs["Distance"].default_value = .035 if terrain else .012
    links = surface.node_tree.links
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], shader.inputs["Base Color"])
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], shader.inputs["Normal"])
    shader.inputs["Roughness"].default_value = .72 if "leather" in label else .88
    if any(word in label for word in ("ivory", "hoof")):
        shader.inputs["Roughness"].default_value = .38


def main(asset):
    tree = asset == "tree"
    source = ASSETS / "Source" / (asset + ".blend")
    texture_name = "tree" if tree else asset + "_atlas"
    metadata = json.loads((ASSETS / "Textures" / (texture_name + ".sprite.json")).read_text())
    target_source = ASSETS / "Source/Fantasy" / source.name
    target_texture = ASSETS / "Textures/Fantasy" / (texture_name + ".png")
    if target_source.exists() or target_texture.exists():
        raise FileExistsError("Preserve existing fantasy outputs before rendering again")
    bpy.ops.wm.open_mainfile(filepath=str(source))
    scene = bpy.context.scene
    terrain = asset == "terrain"
    root = scene.objects.get("Asset_Root")
    used_materials = {slot.material for obj in scene.objects if obj.type == "MESH" for slot in obj.material_slots if slot.material}
    for surface in used_materials:
        finish_material(surface, terrain)
    for obj in scene.objects:
        if obj.type != "MESH":
            continue
        label = obj.name.lower()
        if any(word in label for word in ("head", "muzzle", "body", "crown", "wool", "ear", "nose", "hair")):
            modifier = obj.modifiers.new("Rounded_Organic_Form", "SUBSURF")
            modifier.levels = modifier.render_levels = 1
            for face in obj.data.polygons:
                face.use_smooth = True
        elif asset == "human" and len(obj.data.polygons) == 6:
            modifier = obj.modifiers.new("Soft_Cloth_Edge", "BEVEL")
            modifier.width, modifier.segments = .10, 3
            for face in obj.data.polygons:
                face.use_smooth = True
    # Keep the original framing, correcting the tree elevation to the same 2:1 ground projection.
    if tree:
        camera = scene.camera
        target = Vector((0, 0, 2))
        camera.location = target + Vector((8, -8, math.sqrt(128 / 3)))
        camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    scene.cycles.samples = 24
    scene.render.film_transparent = True
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Medium High Contrast"
    width = metadata.get("cell_width", metadata.get("width"))
    height = metadata.get("cell_height", metadata.get("height"))
    columns, rows = metadata.get("columns", 1), metadata.get("rows", 1)
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    groups = [child for child in root.children if child.type == "EMPTY"] if terrain else []
    if terrain:
        groups = [next(group for group in groups if group.name == name) for name in ("grass", "dirt", "stone")]
    pixels = array("f", [0]) * (width * columns * height * rows * 4)
    with tempfile.TemporaryDirectory(prefix="gse-fantasy-") as temporary:
        for row in range(rows):
            for column in range(columns):
                if terrain:
                    for index, group in enumerate(groups):
                        for obj in [group] + list(group.children_recursive):
                            obj.hide_render = index != column
                elif not tree:
                    root.rotation_euler.z = column * math.pi / 4
                    scene.frame_set(row + 1)
                bpy.context.view_layer.update()
                frame_path = Path(temporary) / (str(row) + "_" + str(column) + ".png")
                scene.render.filepath = str(frame_path)
                bpy.ops.render.render(write_still=True)
                image = bpy.data.images.load(str(frame_path), check_existing=False)
                frame_pixels = array("f", [0]) * (width * height * 4)
                image.pixels.foreach_get(frame_pixels)
                assert min(frame_pixels[3::4]) == 0 and max(frame_pixels[3::4]) > .99
                for y in range(height):
                    offset = (((rows - row - 1) * height + y) * width * columns + column * width) * 4
                    pixels[offset:offset + width * 4] = frame_pixels[y * width * 4:(y + 1) * width * 4]
                bpy.data.images.remove(image)
    target_texture.parent.mkdir(parents=True, exist_ok=True)
    target_source.parent.mkdir(parents=True, exist_ok=True)
    atlas = bpy.data.images.new("Fantasy_" + asset, width=width * columns, height=height * rows, alpha=True)
    atlas.pixels.foreach_set(pixels)
    atlas.filepath_raw, atlas.file_format = str(target_texture), "PNG"
    atlas.save()
    if root:
        root.rotation_euler.z = 0
    scene.frame_set(1)
    if terrain:
        for index, group in enumerate(groups):
            for obj in [group] + list(group.children_recursive):
                obj.hide_render = index != 0
    scene.render.filepath = str(target_texture)
    scene.render.fps = 8
    bpy.context.view_layer.update()
    pivot = world_to_camera_view(scene, scene.camera, Vector((0, 0, 0)))
    metadata.update(pivot_x=round(pivot.x, 8), pivot_y_from_bottom=round(pivot.y, 8),
                    camera_elevation_degrees=30, source="../../Source/Fantasy/" + source.name)
    target_texture.with_suffix(".sprite.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    bpy.ops.wm.save_as_mainfile(filepath=str(target_source))
    print("GSE_FANTASY_EXPORT", asset, flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", required=True, choices=("tree", "terrain", "human", "boar", "deer", "sheep"))
    main(parser.parse_args(sys.argv[sys.argv.index("--") + 1:]).asset)
