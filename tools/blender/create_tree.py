"""Create an original quarter-view tree in Blender and export a transparent sprite.

Run with Blender --background --python tools/blender/create_tree.py, or runpy
inside a separate MCP-connected Blender instance. Existing scene objects stay intact.
"""

import json
import math
from pathlib import Path
import random

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "SimpleGame/Assets/Source/tree.blend"
TEXTURE = ROOT / "SimpleGame/Assets/Textures/tree.png"


def material(name, color):
    result = bpy.data.materials.new(name)
    result.diffuse_color = (*color, 1)
    result.use_nodes = True
    surface = result.node_tree.nodes.get("Principled BSDF")
    surface.inputs["Base Color"].default_value = (*color, 1)
    surface.inputs["Roughness"].default_value = .85
    return result


def branch(start, end, radius, bark):
    direction = Vector(end) - Vector(start)
    bpy.ops.mesh.primitive_cone_add(vertices=9, radius1=radius, radius2=radius * .55,
                                   depth=direction.length, location=(Vector(start) + Vector(end)) * .5)
    result = bpy.context.object
    result.name = "Tree_Branch"
    result.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    for item in bark:
        result.data.materials.append(item)
    for face in result.data.polygons:
        face.material_index = face.index % len(bark)


def main():
    randomizer = random.Random(41)
    scene = bpy.data.scenes.new("GSE_Tree_Asset")
    bpy.context.window.scene = scene
    bark = [material("Bark_Warm", (.20, .105, .045)), material("Bark_Light", (.29, .17, .075)),
            material("Bark_Dark", (.12, .065, .025))]
    leaves = [material("Leaves_" + str(i), color) for i, color in enumerate(
        ((.12, .28, .045), (.19, .36, .065), (.27, .43, .095), (.34, .48, .12), (.14, .32, .075)))]

    branch((0, 0, 0), (.07, .02, 2.65), .22, bark)
    for i in range(5):
        angle = i * math.tau / 5
        branch((math.cos(angle) * .44, math.sin(angle) * .44, .025), (0, 0, .45), .105, bark)
        branch((.02, 0, 1.45), (math.cos(angle) * .95, math.sin(angle) * .95, 2.65), .095, bark)

    clusters = [(0, 0, 3.45, .90), (-.65, -.45, 3.0, .90), (.60, -.48, 2.95, .92),
                (-.65, .55, 3.1, .85), (.62, .55, 3.12, .90), (0, -.80, 2.62, .72),
                (-.93, .02, 2.66, .66), (.98, .05, 2.62, .65), (0, .85, 2.75, .70),
                (-.28, -.12, 3.62, .65), (.35, .10, 3.58, .65)]
    for index, (x, y, z, radius) in enumerate(clusters):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=radius, location=(x, y, z))
        crown = bpy.context.object
        crown.name = "Tree_Crown_" + str(index)
        crown.scale = (1.0, .93, .83)
        crown.rotation_euler = (randomizer.random(), randomizer.random(), randomizer.random())
        for leaf in leaves:
            crown.data.materials.append(leaf)
        for face in crown.data.polygons:
            face.material_index = randomizer.choices(range(len(leaves)), weights=(2, 5, 4, 1, 2))[0]

    camera_data = bpy.data.cameras.new("QuarterView_2to1")
    camera = bpy.data.objects.new("QuarterView_2to1", camera_data)
    scene.collection.objects.link(camera)
    target = Vector((0, 0, 2.0))
    camera.location = target + Vector((8, -8, math.sqrt(128 / 3)))
    camera.rotation_euler = (target - camera.location).to_track_quat("-Z", "Y").to_euler()
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = 5.5
    scene.camera = camera

    for name, position, energy, size in (("Key", (-3, -4, 7), 650, 5), ("Fill", (4, 1, 5), 180, 5)):
        light = bpy.data.lights.new(name, "AREA")
        light.energy, light.shape, light.size = energy, "DISK", size
        obj = bpy.data.objects.new(name, light)
        scene.collection.objects.link(obj)
        obj.location = position
        obj.rotation_euler = (target - obj.location).to_track_quat("-Z", "Y").to_euler()

    scene.world = bpy.data.worlds.new("GSE_Studio")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.60, .68, .80, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .35
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 24
    scene.cycles.use_denoising = True
    scene.render.resolution_x, scene.render.resolution_y = 512, 640
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.view_settings.view_transform = "AgX"
    scene.render.filepath = str(TEXTURE)
    scene.render.use_file_extension = True
    scene.render.image_settings.compression = 70

    bpy.context.view_layer.update()
    mesh_objects = [obj for obj in scene.objects if obj.type == "MESH"]
    assert len(mesh_objects) == 22
    for obj in mesh_objects:
        for vertex in obj.data.vertices:
            projected = world_to_camera_view(scene, camera, obj.matrix_world @ vertex.co)
            assert .02 < projected.x < .98 and .02 < projected.y < .98, "Tree outside sprite frame"
    pivot = world_to_camera_view(scene, camera, Vector((0, 0, 0)))
    assert abs(pivot.x - .5) < 1e-5 and 0 < pivot.y < .5
    SOURCE.parent.mkdir(parents=True, exist_ok=True)
    TEXTURE.parent.mkdir(parents=True, exist_ok=True)
    if SOURCE.exists() or TEXTURE.exists():
        raise FileExistsError("Move previous tree outputs before exporting a new version")
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE))
    bpy.ops.render.render(write_still=True)
    sprite = bpy.data.images.load(str(TEXTURE), check_existing=False)
    assert tuple(sprite.size) == (512, 640) and sprite.channels == 4
    alpha = list(sprite.pixels)[3::4]
    assert min(alpha) == 0.0 and max(alpha) > .99, "Sprite must contain both transparent and opaque pixels"
    bpy.data.images.remove(sprite)
    metadata = {"width": 512, "height": 640, "pivot_x": round(pivot.x, 8),
                "pivot_y_from_bottom": round(pivot.y, 8), "camera_elevation_degrees": 30,
                "ground_shadow": "drawn by game", "source": "../Source/tree.blend"}
    TEXTURE.with_suffix(".sprite.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print("GSE_TREE_EXPORT " + json.dumps(metadata))


if __name__ == "__main__":
    main()
