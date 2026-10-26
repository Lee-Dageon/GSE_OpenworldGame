"""Create an original stone-and-timber house matching the fantasy sprite set."""
import json
from pathlib import Path
import runpy
import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(Path(__file__).with_name("create_world_assets.py")))
finish = runpy.run_path(str(Path(__file__).with_name("restyle_fantasy.py")))["finish_material"]
source = ROOT / "SimpleGame/Assets/Source/Fantasy/house.blend"
texture = ROOT / "SimpleGame/Assets/Textures/Fantasy/house.png"
if source.exists() or texture.exists():
    raise FileExistsError("Existing house is preserved")
scene = helpers["studio"]("Fantasy_House", 512, 512, 1.6, 7.2)
scene.cycles.samples = 24
root = helpers["joint"]("House", (0, 0, 0), None)
material, box, bone = helpers["material"], helpers["box"], helpers["bone"]
stone = material("Stone", (.29, .30, .25))
plaster = material("Plaster", (.49, .43, .30))
wood = material("Dark_Timber", (.16, .075, .025))
roof = material("Roof_Shingle", (.19, .095, .045))
glass = material("Warm_Window", (.75, .39, .09))
for surface in (stone, plaster, wood, roof):
    finish(surface, False)
box("Foundation", (0, 0, .16), (3.6, 2.9, .32), stone, root)
box("Walls", (0, 0, 1.23), (3.2, 2.5, 2.10), plaster, root)
for x in (-1.58, 1.58):
    for y in (-1.26, 1.26):
        box("Corner_Post", (x, y, 1.27), (.16, .16, 2.2), wood, root)
for y in (-1.27, 1.27):
    for z in (.42, 2.20):
        box("Wall_Beam", (0, y, z), (3.4, .14, .15), wood, root)
vertices = [(-2, -1.6, 2.25), (-2, 1.6, 2.25), (0, -1.6, 3.50), (0, 1.6, 3.50),
            (2, -1.6, 2.25), (2, 1.6, 2.25)]
mesh = bpy.data.meshes.new("Gabled_Roof")
mesh.from_pydata(vertices, [], [(0,1,3,2), (2,3,5,4)])
obj = bpy.data.objects.new("Gabled_Roof", mesh)
scene.collection.objects.link(obj)
helpers["attach"](obj, root, roof)
for y in (-1.61, 1.61):
    bone("Roof_Edge", (-2,y,2.25), (0,y,3.52), .075, wood, root)
    bone("Roof_Edge", (0,y,3.52), (2,y,2.25), .075, wood, root)
    for x in (-1.2, -.6, .6, 1.2):
        z=3.5-abs(x)*.625
        box("Roof_Shingle_Line", (x,0,z+.025), (.05,3.24,.04), wood, root)
box("Door", (0,-1.29,.90), (.66,.12,1.42), wood, root)
for x in (-.27,-.09,.09,.27):
    box("Door_Plank", (x,-1.36,.90), (.015,.015,1.35), stone, root)
for x in (-1.0, 1.0):
    box("Window_Frame", (x,-1.29,1.40), (.63,.12,.68), wood, root)
    box("Window_Glass", (x,-1.36,1.40), (.48,.03,.51), glass, root)
    box("Window_Bar", (x,-1.39,1.40), (.035,.025,.54), wood, root)
    box("Window_Bar", (x,-1.39,1.40), (.50,.025,.035), wood, root)
box("Chimney", (.85,.65,3.30), (.42,.45,1.35), stone, root)
box("Chimney_Cap", (.85,.65,4.0), (.57,.60,.14), wood, root)
box("Step", (0,-1.58,.12), (.96,.48,.24), stone, root)
for obj in root.children_recursive:
    if obj.type=="MESH" and len(obj.data.polygons)==6:
        bevel=obj.modifiers.new("Edge_Wear", "BEVEL")
        bevel.width, bevel.segments=.025,2
bpy.context.view_layer.update()
for obj in root.children_recursive:
    if obj.type=="MESH":
        for vertex in obj.data.vertices:
            point=world_to_camera_view(scene,scene.camera,obj.matrix_world@vertex.co)
            assert .02<point.x<.98 and .02<point.y<.98
pivot=world_to_camera_view(scene,scene.camera,Vector((0,0,0)))
scene.render.filepath=str(texture)
bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(source))
texture.with_suffix(".sprite.json").write_text(json.dumps({"width":512,"height":512,
    "pivot_x":round(pivot.x,8),"pivot_y_from_bottom":round(pivot.y,8),"camera_elevation_degrees":30,
    "source":"../../Source/Fantasy/house.blend"},indent=2)+"\n",encoding="utf-8")
print("GSE_HOUSE_EXPORT",pivot.y)
