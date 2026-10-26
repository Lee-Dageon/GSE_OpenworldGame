"""Export a proportioned armored player using the existing 8-direction atlas pipeline."""
from pathlib import Path
import runpy
import math
import bpy

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(Path(__file__).with_name("create_world_assets.py")))
finish = runpy.run_path(str(Path(__file__).with_name("restyle_fantasy.py")))["finish_material"]


def warrior(root):
    material, oval, box, bone, joint = (helpers[name] for name in ("material","oval","box","bone","joint"))
    steel = material("Worn_Steel", (.29,.32,.32))
    trim = material("Aged_Brass", (.35,.23,.085))
    leather = material("Dark_Leather", (.065,.035,.018))
    cloth = material("Wine_Surcoat", (.17,.025,.018))
    skin = material("Skin", (.48,.29,.17))
    dark = material("Dark_Eyes", (.012,.010,.008))
    for surface in (steel,trim,leather,cloth):
        finish(surface,False)
    for surface in (steel,trim):
        shader=surface.node_tree.nodes.get("Principled BSDF")
        shader.inputs["Metallic"].default_value=.78
        shader.inputs["Roughness"].default_value=.35
    oval("Waist", (0,0,1.13), (.23,.16,.18), leather,root)
    oval("Breastplate", (0,0,1.48), (.32,.19,.37),steel,root)
    box("Belt", (0,-.01,1.13),(.48,.36,.08),leather,root)
    box("Buckle", (0,-.205,1.13),(.09,.035,.08),trim,root)
    box("Surcoat", (0,.025,.99),(.43,.30,.25),cloth,root)
    bone("Neck", (0,0,1.75),(0,0,1.91),.085,skin,root)
    oval("Head", (0,-.005,2.02),(.135,.125,.18),skin,root)
    oval("Helm", (0,.025,2.12),(.153,.145,.13),steel,root)
    box("Nose_Guard",(0,-.135,2.055),(.025,.025,.18),trim,root)
    for side in (-1,1):
        oval("Eye",(side*.048,-.122,2.045),(.012,.014,.015),dark,root)
    limbs=[]
    for side in (-1,1):
        leg=joint("Leg",(side*.13,0,1.08),root)
        bone("Thigh",(0,0,-.02),(0,0,-.48),.10,leather,leg)
        oval("Knee_Plate",(0,-.035,-.46),(.11,.105,.14),steel,leg)
        bone("Greave",(0,0,-.48),(0,0,-.91),.075,steel,leg)
        box("Boot",(0,-.055,-.98),(.16,.30,.18),leather,leg)
        arm=joint("Arm",(side*.34,0,1.72),root)
        oval("Pauldron",(side*.035,0,-.025),(.18,.18,.14),steel,arm)
        bone("Upper_Arm",(0,0,-.04),(side*.035,0,-.35),.085,leather,arm)
        bone("Forearm_Plate",(side*.035,0,-.35),(side*.02,-.02,-.61),.075,steel,arm)
        oval("Glove",(side*.02,-.02,-.65),(.07,.075,.09),leather,arm)
        if side==1:
            box("Sword_Grip",(.02,-.06,-.73),(.035,.04,.19),leather,arm)
            box("Sword_Guard",(.02,-.06,-.82),(.25,.055,.035),trim,arm)
            box("Sword_Blade",(.02,-.06,-1.10),(.065,.025,.52),steel,arm)
        limbs.extend(((leg,side),(arm,-side*.55)))
    for obj in root.children_recursive:
        if obj.type=="MESH":
            for face in obj.data.polygons: face.use_smooth=True
            if len(obj.data.polygons)==6:
                bevel=obj.modifiers.new("Worn_Edge","BEVEL")
                bevel.width,bevel.segments=.06,2
    return limbs


export = helpers["main"]
export.__globals__["human"] = warrior
export.__globals__["ASSETS"] = ROOT / "SimpleGame/Assets/Hero"
export("human")
