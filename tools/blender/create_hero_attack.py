"""Export eight-direction sword attacks from the existing armored hero.

Run with Blender --background --python tools/blender/create_hero_attack.py.
Existing source and output files are never overwritten.
"""
from array import array
import json
import math
from pathlib import Path
import tempfile

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "SimpleGame/Assets/Hero"
SOURCE = ASSETS / "Source/human_attack.blend"
ATLAS = ASSETS / "Textures/human_attack_atlas.png"
METADATA = ATLAS.with_suffix(".sprite.json")
CELL, DIRECTIONS, FRAMES = 256, 8, 8
DURATION = .32
ORTHO_SCALE = 2.8 * CELL / 192
# shoulder X/Y, elbow X, torso Z: anticipation, strike, follow-through, recovery.
POSES = ((0, 0, 0, 0), (-1.0, -.35, -.35, -.12),
         (-1.9, -.45, -.65, -.22), (-1.35, .15, -.12, .02),
         (-.70, .85, -.08, .22), (-.25, .55, -.20, .14),
         (-.10, .20, -.12, .06), (0, 0, 0, 0))


def reparent(obj, parent):
    transform = obj.matrix_world.copy()
    obj.parent = parent
    obj.matrix_world = transform


def main():
    if any(path.exists() for path in (SOURCE, ATLAS, METADATA)):
        raise FileExistsError("Attack outputs already exist; preserve or relocate them first")
    bpy.ops.wm.open_mainfile(filepath=str(ASSETS / "Source/human.blend"))
    scene = bpy.context.scene
    root = bpy.data.objects["Asset_Root"]
    for obj in [root] + list(root.children_recursive):
        obj.animation_data_clear()
        if obj.type == "EMPTY":
            obj.rotation_euler = (0, 0, 0)
    bpy.context.view_layer.update()
    weapon_arm = next(obj for obj in root.children if obj.name.startswith("Arm")
                      and any(child.name.startswith("Sword_Blade") for child in obj.children))
    other_arm = next(obj for obj in root.children if obj.name.startswith("Arm") and obj != weapon_arm)
    torso = bpy.data.objects.new("Attack_Torso", None)
    scene.collection.objects.link(torso)
    torso.parent, torso.location = root, (0, 0, 1.15)
    bpy.context.view_layer.update()
    for obj in list(root.children):
        if obj != torso and not obj.name.startswith("Leg"):
            reparent(obj, torso)
    bpy.context.view_layer.update()
    elbow = bpy.data.objects.new("Attack_Elbow", None)
    scene.collection.objects.link(elbow)
    elbow.parent, elbow.location = weapon_arm, (.035, 0, -.35)
    bpy.context.view_layer.update()
    for obj in list(weapon_arm.children):
        if obj.name.startswith(("Forearm", "Glove", "Sword")):
            reparent(obj, elbow)
    scene.frame_start, scene.frame_end = 1, FRAMES
    scene.render.fps, scene.render.fps_base = 25, 1.0
    for frame, (shoulder_x, shoulder_y, elbow_x, twist) in enumerate(POSES, 1):
        weapon_arm.rotation_euler = (shoulder_x, shoulder_y, 0)
        elbow.rotation_euler.x = elbow_x
        torso.rotation_euler.z = twist
        other_arm.rotation_euler.x = shoulder_x * .22
        for obj in (weapon_arm, elbow, torso, other_arm):
            obj.keyframe_insert(data_path="rotation_euler", frame=frame)
    scene.camera.data.ortho_scale = ORTHO_SCALE
    scene.render.resolution_x = scene.render.resolution_y = CELL
    scene.render.resolution_percentage = 100
    scene.cycles.samples = 12
    scene.frame_set(1)
    bpy.context.view_layer.update()
    pivot = world_to_camera_view(scene, scene.camera, Vector((0, 0, 0)))
    # Check every pose/direction before spending time on renders.
    for frame in range(FRAMES):
        scene.frame_set(frame + 1)
        for direction in range(DIRECTIONS):
            root.rotation_euler.z = direction * math.pi / 4
            bpy.context.view_layer.update()
            for obj in root.children_recursive:
                if obj.type == "MESH":
                    for corner in obj.bound_box:
                        p = world_to_camera_view(scene, scene.camera, obj.matrix_world @ Vector(corner))
                        assert .01 < p.x < .99 and .01 < p.y < .99, (frame, direction, obj.name, tuple(p))
    width, height = CELL * DIRECTIONS, CELL * FRAMES
    pixels = array("f", [0]) * (width * height * 4)
    with tempfile.TemporaryDirectory(prefix="gse-attack-") as directory:
        for frame in range(FRAMES):
            scene.frame_set(frame + 1)
            for direction in range(DIRECTIONS):
                root.rotation_euler.z = direction * math.pi / 4
                bpy.context.view_layer.update()
                path = Path(directory) / "frame.png"
                scene.render.filepath = str(path)
                bpy.ops.render.render(write_still=True)
                image = bpy.data.images.load(str(path), check_existing=False)
                data = array("f", [0]) * (CELL * CELL * 4)
                image.pixels.foreach_get(data)
                assert min(data[3::4]) == 0 and max(data[3::4]) > .99
                for y in range(CELL):
                    start = (((FRAMES-frame-1)*CELL+y)*width + direction*CELL)*4
                    pixels[start:start+CELL*4] = data[y*CELL*4:(y+1)*CELL*4]
                bpy.data.images.remove(image)
                print("GSE_ATTACK_FRAME", frame, direction, flush=True)
    atlas = bpy.data.images.new("human_attack_atlas", width=width, height=height, alpha=True)
    atlas.pixels.foreach_set(pixels)
    atlas.filepath_raw, atlas.file_format = str(ATLAS), "PNG"
    atlas.save()
    root.rotation_euler.z = 0
    scene.frame_set(1)
    bpy.ops.wm.save_as_mainfile(filepath=str(SOURCE))
    metadata = dict(asset="human_attack", cell_width=CELL, cell_height=CELL,
                    columns=DIRECTIONS, rows=FRAMES, duration_seconds=DURATION,
                    pivot_x=round(pivot.x, 8), pivot_y_from_bottom=round(pivot.y, 8),
                    display_height_ratio=ORTHO_SCALE/2.8, row_order="top_to_bottom",
                    camera_elevation_degrees=30,
                    columns_description="model rotation Z: 0..315 degrees, step 45; front=-Y",
                    source="../Source/human_attack.blend")
    METADATA.write_text(json.dumps(metadata, indent=2)+"\n", encoding="utf-8")
    print("GSE_ATTACK_EXPORT", json.dumps(metadata), flush=True)


if __name__ == "__main__":
    main()
