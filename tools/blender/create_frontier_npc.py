"""Export a civilian with the hero's proportions, studio and worn material finish."""
from pathlib import Path
import runpy
import bpy

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(Path(__file__).with_name("create_world_assets.py")))
finish = runpy.run_path(str(Path(__file__).with_name("restyle_fantasy.py")))["finish_material"]


def civilian(root):
    material, oval, box, bone, joint = (helpers[key] for key in ("material", "oval", "box", "bone", "joint"))
    cloth = material("Moss_Linen", (.11, .19, .105))
    pants = material("Canvas_Trousers", (.13, .105, .075))
    leather = material("Worn_Leather", (.10, .052, .026))
    skin = material("Skin", (.48, .29, .17))
    hair = material("Hair", (.085, .05, .025))
    brass = material("Aged_Brass", (.35, .23, .085))
    for surface in (cloth, pants, leather, brass):
        finish(surface, False)
    oval("Tunic", (0, 0, 1.46), (.28, .18, .36), cloth, root)
    oval("Coat_Hem", (0, .015, 1.12), (.26, .18, .20), cloth, root)
    box("Belt", (0, -.01, 1.15), (.48, .35, .065), leather, root)
    box("Buckle", (0, -.193, 1.15), (.075, .025, .065), brass, root)
    box("Satchel", (.25, .04, 1.02), (.18, .23, .25), leather, root)
    bone("Neck", (0, 0, 1.73), (0, 0, 1.90), .075, skin, root)
    oval("Head", (0, -.005, 2.02), (.135, .125, .18), skin, root)
    oval("Hair", (0, .025, 2.12), (.143, .13, .12), hair, root)
    oval("Hat_Brim", (0, 0, 2.18), (.24, .21, .025), leather, root)
    oval("Hat", (0, .025, 2.23), (.15, .14, .07), cloth, root)
    oval("Nose", (0, -.125, 2.015), (.026, .035, .033), skin, root)
    for side in (-1, 1):
        oval("Eye", (side * .048, -.122, 2.045), (.012, .014, .015), hair, root)
    limbs = []
    for side in (-1, 1):
        leg = joint("Leg", (side * .13, 0, 1.08), root)
        bone("Trouser", (0, 0, -.02), (0, 0, -.80), .105, pants, leg)
        box("Boot", (0, -.055, -.98), (.16, .30, .18), leather, leg)
        arm = joint("Arm", (side * .28, 0, 1.71), root)
        bone("Sleeve", (0, 0, 0), (side * .03, 0, -.48), .095, cloth, arm)
        oval("Cuff", (side * .03, 0, -.48), (.075, .08, .07), leather, arm)
        oval("Hand", (side * .03, -.015, -.59), (.065, .07, .09), skin, arm)
        limbs.extend(((leg, side), (arm, -side * .55)))
    for obj in root.children_recursive:
        if obj.type == "MESH":
            for face in obj.data.polygons:
                face.use_smooth = True
            if len(obj.data.polygons) == 6:
                bevel = obj.modifiers.new("Worn_Edge", "BEVEL")
                bevel.width, bevel.segments = .06, 2
    return limbs


export = helpers["main"]
export.__globals__["human"] = civilian
export.__globals__["ASSETS"] = ROOT / "SimpleGame/Assets/Frontier"
export("human")
