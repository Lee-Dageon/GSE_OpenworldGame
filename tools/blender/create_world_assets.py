"""Export terrain tiles and eight-direction walking sprites with Blender.

Blender --background --python tools/blender/create_world_assets.py -- --asset human
Assets: terrain, human, boar, deer, sheep. No existing output is overwritten.
"""

import argparse
from array import array
import json
import math
from pathlib import Path
import random
import runpy
import sys
import tempfile

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "SimpleGame/Assets"
material = runpy.run_path(str(Path(__file__).with_name("create_tree.py")))["material"]


def attach(obj, parent, surface):
    obj.parent = parent
    obj.data.materials.append(surface)
    return obj


def box(name, position, size, surface, parent):
    bpy.ops.mesh.primitive_cube_add(size=1, location=position)
    obj = bpy.context.object
    obj.name, obj.scale = name, size
    return attach(obj, parent, surface)


def oval(name, position, size, surface, parent):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1, location=position)
    obj = bpy.context.object
    obj.name, obj.scale = name, size
    return attach(obj, parent, surface)


def bone(name, start, end, radius, surface, parent):
    delta = Vector(end) - Vector(start)
    bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=radius, radius2=radius * .65,
                                   depth=delta.length, location=(Vector(start) + Vector(end)) * .5)
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = delta.to_track_quat("Z", "Y").to_euler()
    return attach(obj, parent, surface)


def joint(name, position, parent):
    obj = bpy.data.objects.new(name, None)
    bpy.context.scene.collection.objects.link(obj)
    obj.location, obj.parent = position, parent
    return obj


def studio(name, width, height, target_height, ortho_scale):
    scene = bpy.data.scenes.new("GSE_" + name)
    bpy.context.window.scene = scene
    camera_data = bpy.data.cameras.new("QuarterView_30deg")
    camera = bpy.data.objects.new("QuarterView_30deg", camera_data)
    scene.collection.objects.link(camera)
    target = Vector((0, 0, target_height))
    camera.location = target + Vector((8, -8, math.sqrt(128 / 3)))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.type, camera_data.ortho_scale = "ORTHO", ortho_scale
    scene.camera = camera
    for label, position, energy in (("Key", (-3, -4, 7), 550), ("Fill", (4, 2, 5), 160)):
        data = bpy.data.lights.new(label, "AREA")
        data.energy, data.size = energy, 5
        light = bpy.data.objects.new(label, data)
        scene.collection.objects.link(light)
        light.location = position
        light.rotation_euler = (target - light.location).to_track_quat("-Z", "Y").to_euler()
    scene.world = bpy.data.worlds.new("Studio_World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .4
    scene.render.engine = "CYCLES"
    scene.cycles.device, scene.cycles.samples = "CPU", 12
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = width, height
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "AgX"
    return scene


def human(root):
    skin = material("Skin", (.55, .32, .18))
    cloth = material("Hunter_Green", (.12, .24, .10))
    leather = material("Leather", (.18, .085, .035))
    pants = material("Trousers", (.17, .14, .09))
    black = material("Eyes", (.015, .012, .008))
    box("Coat", (0, 0, 1.10), (.48, .29, .59), cloth, root)
    box("Belt", (0, -.01, .85), (.50, .31, .07), leather, root)
    box("Pouch", (.25, -.04, .81), (.16, .18, .22), leather, root)
    oval("Head", (0, -.015, 1.60), (.20, .17, .24), skin, root)
    oval("Hair", (0, .015, 1.74), (.21, .17, .13), leather, root)
    box("Hat_Brim", (0, 0, 1.80), (.55, .47, .045), leather, root)
    box("Hat_Crown", (0, .025, 1.87), (.36, .29, .13), cloth, root)
    for side in (-1, 1):
        oval("Eye", (side * .075, -.167, 1.64), (.022, .016, .025), black, root)
    oval("Nose", (0, -.19, 1.58), (.04, .055, .045), skin, root)
    limbs = []
    for side in (-1, 1):
        leg = joint("Leg", (side * .125, 0, .75), root)
        box("Trousers_Leg", (0, 0, -.25), (.19, .22, .50), pants, leg)
        box("Boot", (0, -.055, -.64), (.21, .34, .22), leather, leg)
        arm = joint("Arm", (side * .32, 0, 1.32), root)
        box("Sleeve", (0, 0, -.20), (.17, .22, .40), cloth, arm)
        oval("Hand", (0, -.01, -.45), (.085, .09, .10), skin, arm)
        limbs.extend(((leg, side), (arm, -side)))
    return limbs


def animal(root, kind):
    wool = material("Wool", (.68, .65, .52))
    fur = material("Fur", {"boar": (.20, .10, .045), "deer": (.41, .23, .09), "sheep": (.16, .12, .08)}[kind])
    hoof = material("Hoof_Eye", (.025, .02, .015))
    ivory = material("Ivory", (.72, .66, .46))
    tall = kind == "deer"
    body_z, hip, length = (1.02, .85, .78) if tall else (.70, .56, .49)
    oval("Body", (0, .05, body_z), (.32 if tall else .43, .68, .36), fur, root)
    if kind == "sheep":
        for x, y, z in ((-.21, -.3, .82), (.21, -.3, .82), (-.23, .22, .84), (.23, .22, .84),
                        (0, -.28, 1.02), (0, .22, 1.03), (0, .53, .87)):
            oval("Wool_Cluster", (x, y, z), (.30, .34, .29), wool, root)
    head_z = 1.40 if tall else .83
    bone("Neck", (0, -.36, body_z), (0, -.60, head_z), .17, fur, root)
    oval("Head", (0, -.63, head_z), (.20, .28, .25), fur, root)
    oval("Muzzle", (0, -.87, head_z - .07), (.16, .21, .13), fur, root)
    for side in (-1, 1):
        oval("Ear", (side * .21, -.59, head_z + .23), (.12, .08, .18), fur, root)
        oval("Eye", (side * .16, -.78, head_z + .035), (.026, .028, .03), hoof, root)
        if tall:
            bone("Antler", (side * .13, -.54, head_z + .17), (side * .24, -.47, 2.04), .035, ivory, root)
            bone("Antler_Tine", (side * .20, -.50, 1.87), (side * .38, -.48, 1.99), .025, ivory, root)
        if kind == "boar":
            bone("Tusk", (side * .17, -.88, head_z - .14), (side * .21, -1.01, head_z + .10), .045, ivory, root)
    bone("Tail", (0, .62, body_z), (0, .89, body_z + .08), .045, fur, root)
    limbs = []
    for side in (-1, 1):
        for fore in (-1, 1):
            leg = joint("Leg", (side * .25, fore * .43, hip), root)
            bone("Lower_Leg", (0, 0, 0), (0, 0, -length + .10), .065, fur, leg)
            box("Hoof", (0, -.025, -length + .04), (.14, .18, .12), hoof, leg)
            limbs.append((leg, side * fore))
    return limbs


def terrain(root):
    randomizer = random.Random(81)
    colors = ((.17, .29, .055), (.28, .18, .085), (.29, .30, .27))
    groups = []
    for kind, color in zip(("grass", "dirt", "stone"), colors):
        group = joint(kind, (0, 0, 0), root)
        surfaces = [material(kind + str(i), tuple(value * (.85 + i * .075) for value in color)) for i in range(5)]
        vertices = [(x / 6 - 1, y / 6 - 1, 0) for y in range(13) for x in range(13)]
        faces = [(y * 13 + x, y * 13 + x + 1, (y + 1) * 13 + x + 1, (y + 1) * 13 + x)
                 for y in range(12) for x in range(12)]
        mesh = bpy.data.meshes.new(kind)
        mesh.from_pydata(vertices, [], faces)
        obj = bpy.data.objects.new(kind, mesh)
        bpy.context.scene.collection.objects.link(obj)
        obj.parent = group
        for surface in surfaces:
            mesh.materials.append(surface)
        for index, face in enumerate(mesh.polygons):
            edge = index % 12 in (0, 11) or index // 12 in (0, 11)
            face.material_index = 2 if edge else randomizer.randrange(5)
        for i in range(18):
            x, y = randomizer.uniform(-.75, .75), randomizer.uniform(-.75, .75)
            if kind == "grass":
                bone("Grass_Blade", (x, y, 0), (x + .025, y, .06), .012, surfaces[3], group)
            else:
                oval("Pebble", (x, y, .008), (.035, .045, .016), surfaces[i % 5], group)
        groups.append(group)
    return groups


def children(root):
    return [root] + list(root.children_recursive)


def main(asset):
    atlas_path = ASSETS / "Textures" / (asset + "_atlas.png")
    source_path = ASSETS / "Source" / (asset + ".blend")
    metadata_path = atlas_path.with_suffix(".sprite.json")
    if any(path.exists() for path in (atlas_path, source_path, metadata_path)):
        raise FileExistsError("Existing outputs are preserved: " + asset)
    ground = asset == "terrain"
    width, height = (256, 128) if ground else (128, 192)
    columns, rows = (3, 1) if ground else (8, 4)
    ortho_scale = math.sqrt(8) if ground else (2.8 if asset == "human" else 3.8)
    scene = studio(asset, width, height, 0 if ground else 1.0, ortho_scale)
    root = joint("Asset_Root", (0, 0, 0), None)
    groups = terrain(root) if ground else []
    limbs = [] if ground else (human(root) if asset == "human" else animal(root, asset))
    if not ground:
        scene.frame_start, scene.frame_end = 1, 4
        for frame in range(4):
            for limb, phase in limbs:
                limb.rotation_euler.x = math.sin(frame * math.pi / 2) * phase * .35
                limb.keyframe_insert(data_path="rotation_euler", frame=frame + 1)
        scene.frame_set(1)
    bpy.context.view_layer.update()
    pivot = world_to_camera_view(scene, scene.camera, Vector((0, 0, 0)))
    if ground:
        corners = [world_to_camera_view(scene, scene.camera, Vector(point)) for point in
                   ((-1, -1, 0), (-1, 1, 0), (1, -1, 0), (1, 1, 0))]
        assert abs(min(p.x for p in corners)) < 1e-5 and abs(max(p.x for p in corners) - 1) < 1e-5
        assert abs(min(p.y for p in corners)) < 1e-5 and abs(max(p.y for p in corners) - 1) < 1e-5
    source_path.parent.mkdir(parents=True, exist_ok=True)
    atlas_path.parent.mkdir(parents=True, exist_ok=True)
    atlas_width, atlas_height = width * columns, height * rows
    pixels = array("f", [0]) * (atlas_width * atlas_height * 4)
    with tempfile.TemporaryDirectory(prefix="gse-blender-frames-") as temporary:
        for row in range(rows):
            for column in range(columns):
                if ground:
                    for index, group in enumerate(groups):
                        for obj in children(group):
                            obj.hide_render = index != column
                else:
                    root.rotation_euler.z = column * math.pi / 4
                    scene.frame_set(row + 1)
                bpy.context.view_layer.update()
                # Verify model bounds in every direction and animation frame.
                if not ground:
                    for obj in root.children_recursive:
                        if obj.type == "MESH":
                            for vertex in obj.data.vertices:
                                point = world_to_camera_view(scene, scene.camera, obj.matrix_world @ vertex.co)
                                assert .01 < point.x < .99 and .01 < point.y < .99, (asset, row, column, obj.name)
                frame_path = Path(temporary) / (str(row) + "_" + str(column) + ".png")
                scene.render.filepath = str(frame_path)
                bpy.ops.render.render(write_still=True)
                image = bpy.data.images.load(str(frame_path), check_existing=False)
                frame_pixels = array("f", [0]) * (width * height * 4)
                image.pixels.foreach_get(frame_pixels)
                alpha = frame_pixels[3::4]
                assert min(alpha) == 0 and max(alpha) > .99, "Missing sprite transparency or geometry"
                for y in range(height):
                    destination = (((rows - row - 1) * height + y) * atlas_width + column * width) * 4
                    pixels[destination:destination + width * 4] = frame_pixels[y * width * 4:(y + 1) * width * 4]
                bpy.data.images.remove(image)
                print("GSE_FRAME", asset, row, column, flush=True)
    atlas = bpy.data.images.new(asset + "_atlas", width=atlas_width, height=atlas_height, alpha=True)
    atlas.pixels.foreach_set(pixels)
    atlas.filepath_raw, atlas.file_format = str(atlas_path), "PNG"
    atlas.save()
    # Leave the source in the first direction/pose, with only the first terrain group visible.
    root.rotation_euler.z = 0
    scene.frame_set(1)
    if ground:
        for index, group in enumerate(groups):
            for obj in children(group):
                obj.hide_render = index != 0
    scene.render.filepath = str(atlas_path.with_name(asset + "_preview.png"))
    scene.render.image_settings.color_mode = "RGBA"
    bpy.ops.wm.save_as_mainfile(filepath=str(source_path))
    metadata = {"asset": asset, "cell_width": width, "cell_height": height, "columns": columns, "rows": rows,
                "pivot_x": round(pivot.x, 8), "pivot_y_from_bottom": round(pivot.y, 8),
                "camera_elevation_degrees": 30, "row_order": "top_to_bottom", "walk_fps": 8 if not ground else 0,
                "columns_description": ["grass", "dirt", "stone"] if ground else "model rotation Z: 0..315 degrees, step 45; front=-Y",
                "source": "../Source/" + source_path.name}
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print("GSE_ASSET_EXPORT " + json.dumps(metadata), flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--asset", required=True, choices=("terrain", "human", "boar", "deer", "sheep"))
    main(parser.parse_args(sys.argv[sys.argv.index("--") + 1:]).asset)
