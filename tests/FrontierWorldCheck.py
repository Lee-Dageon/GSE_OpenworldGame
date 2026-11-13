"""Offline content/resource/path checks; never launches or builds the game."""
import hashlib
import heapq
import json
import math
import shlex
from functools import lru_cache
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'SimpleGame'
def records(name):
    return [shlex.split(line,comments=True) for line in (GAME/'Data'/name).read_text(encoding='utf-8-sig').splitlines() if line.strip()]
rows=records('map.gse'); story=records('story.gse')
by=lambda tag:[r for r in rows if r and r[0]==tag]
assert by('bounds')[0][1:]==['-900','900']
houses=[tuple(map(float,r[2:])) for r in by('house')]
trees=[(float(r[2]),float(r[3]),float(r[-1])) for r in by('tree')]
rocks=[(float(r[3]),float(r[4]),float(r[-1])+.18) for r in by('landmark') if float(r[-1])>0]
lake=tuple(map(float,by('lake')[0][1:]))
terrain=[tuple(map(float,r[2:])) for r in by('terrain')]
ravines=[]
for kind,ax,az,bx,bz,width,height in terrain:
    assert -40>max(ax,bx) or 40<min(ax,bx) or -40>max(az,bz) or 40<min(az,bz)
    length=math.hypot(bx-ax,bz-az)
    if kind!=0:ravines.append((ax,az,(bx-ax)/length,(bz-az)/length,length,width))
buckets={}
for a,b,r in trees+rocks:
    for x in range(math.floor((a-r)/16),math.floor((a+r)/16)+1):
        for z in range(math.floor((b-r)/16),math.floor((b+r)/16)+1):
            buckets.setdefault((x,z),[]).append((a,b,r))
@lru_cache(maxsize=250000)
def blocked(p):
    x,z=p
    return (math.hypot(x-lake[0],z-lake[1])<lake[2] or
        any(length*.04<(x-a)*dx+(z-b)*dz<length*.96 and width<abs(-(x-a)*dz+(z-b)*dx)<width+8
            for a,b,dx,dz,length,width in ravines) or
        any(abs(x-h[0])<h[5] and abs(z-h[1])<h[6] for h in houses) or
        any((x-a)**2+(z-b)**2<r*r for a,b,r in buckets.get((math.floor(x/16),math.floor(z/16)),[])))
steps=[r for r in story if r and r[0]=='step']
targets={r[1]:(float(r[4]),float(r[5])) for r in steps}
spawn=tuple(map(float,by('spawn')[0][1:]))
assert math.dist(spawn,targets['ancient_hunt'])>800
for a,b in [('south_travel','south_tracks'),('forest_travel','forest_tracks'),('sand_travel','sand_records'),
            ('expedition_travel','expedition_records'),('ruin_entrance','ruin_control'),('ruin_control','ancient_hunt')]:
    assert math.dist(targets[a],targets[b])>65,(a,b)
nodes=by('gather')
assert len(nodes)>=450 and {int(r[2]) for r in nodes}==set(range(16))
ore=sum(4<=int(r[2])<=11 for r in nodes)
assert ore/len(nodes)<.12 and ore<=55
assert len(terrain)==11 and {int(t[0]) for t in terrain}=={0,1,2}
for r in nodes:
    p=tuple(map(float,r[3:5]))
    assert not blocked(p),r
    assert not(-40<=p[0]<=40 and -40<=p[1]<=40), 'village must keep its existing density'
    assert min(math.dist(p,q) for q in targets.values())>5.9,r
for p in targets.values(): assert not blocked(p),p
# A* on a two-unit lattice proves broad routes exist around the shared obstacles.
def reachable(start,goal):
    a=tuple(round(v/2) for v in start); b=tuple(round(v/2) for v in goal)
    low=(min(a[0],b[0])-20,min(a[1],b[1])-20)
    high=(max(a[0],b[0])+20,max(a[1],b[1])+20)
    queue=[(0,a)];cost={a:0};visited=set()
    def heuristic(p):
        dx,dz=abs(p[0]-b[0]),abs(p[1]-b[1])
        return max(dx,dz)+(math.sqrt(2)-1)*min(dx,dz)
    while queue:
        _,p=heapq.heappop(queue)
        if p in visited: continue
        if p==b:return True
        visited.add(p)
        assert len(visited)<150000, 'route search exceeded bound'
        for dx,dz in [(1,0),(-1,0),(0,1),(0,-1),(1,1),(1,-1),(-1,1),(-1,-1)]:
            q=(p[0]+dx,p[1]+dz)
            if not(low[0]<=q[0]<=high[0] and low[1]<=q[1]<=high[1]):continue
            # Sample the swept step, including the midpoint, so the coarse grid cannot jump over trees.
            if any(blocked(((p[0]+dx*t)*2,(p[1]+dz*t)*2)) for t in [.25,.5,.75,1]):continue
            value=cost[p]+math.hypot(dx,dz)
            if value<cost.get(q,float('inf')):
                cost[q]=value;heapq.heappush(queue,(value+heuristic(q),q))
    return False
route=[spawn]+[tuple(map(float,by('quest_point')[i][2:4])) for i in range(3)]+list(targets.values())
for a,b in zip(route,route[1:]):assert reachable(a,b),(a,b)
regions=[r for r in story if r and r[0]=='region' and r[1] in ('desert_wastes','sand_outpost')]
x,z=targets['ancient_hunt']
assert any(float(r[3])<x<float(r[5]) and float(r[4])<z<float(r[6]) for r in regions)
assets=GAME/'Assets/PixelArt'
manifest=json.loads((assets/'World/desert-frontier-v1.json').read_text())
for name,key in [('desert-props-v1.png','props_sha256'),('desert-sand-v1.png','sand_sha256')]:
    assert hashlib.sha256((assets/'World'/name).read_bytes()).hexdigest()==manifest[key]
assert len(manifest['frames'])==16
for frame in manifest['frames']:
    assert 0<frame['x']<1 and 0<frame['y']<1 and 1<frame['scale']<5
    sx,sy,sw,sh=frame['source']
    l,t,r,b=frame['bounds']
    assert 0<l<r<sw and 0<t<b<sh # No solid body touches a sampled edge.
console=manifest['frames'][14]
sx,sy,sw,sh=console['source']
assert sy < 907 and sy+sh > 1089 # Includes the roof and base of the original control-device body.
images=set()
for name in manifest['materials']+['lizard-scale','reptile-tail','camel-hide','camel-milk']:
    p=assets/'Items/Frontier'/('frontier-'+name+'.png')
    meta=json.loads(p.with_suffix('.json').read_text())['meta']['pixelfx']
    assert meta['frames']==24 and meta['frameSize']==[64,64] and meta['columns']==5 and meta['loop']
    with Image.open(p) as im:
        assert im.mode=='RGBA' and im.size==(320,320)
        for frame in range(24):
            x,y=frame%5*64,frame//5*64
            assert im.crop((x,y,x+64,y+64)).getchannel('A').getbbox()
        images.add(hashlib.sha256(im.crop((64,64,128,128)).tobytes()).hexdigest())
assert len(images)==20
assert (assets/'Effects/frontier-LICENSE.txt').exists()
animals=by('animal')
desert_animals=[r for r in animals if int(r[6])>=3]
assert len(animals)==48 and len(desert_animals)==20 and {int(r[6]) for r in desert_animals}=={3,4}
for row in desert_animals:
    x,z=map(float,row[2:4]);assert not blocked((x,z))
    assert any(float(r[3])<x<float(r[5]) and float(r[4])<z<float(r[6]) for r in regions)
wildlife=json.loads((assets/'Animals/Desert/desert-wildlife-v1.json').read_text())
for animal in wildlife['animals']:
    path=assets/'Animals/Desert'/(animal['name']+'-v1.png')
    assert hashlib.sha256(path.read_bytes()).hexdigest()==animal['sha256']
    with Image.open(path) as image:
        assert image.mode=='RGBA' and image.getchannel('A').getextrema()[0]==0
        assert image.getchannel('A').getextrema()[1]>=250
        assert len(animal['frames'])==24
        for frame in animal['frames']:
            x,y,w,h=frame['source'];l,t,r,b=frame['body_box']
            assert 0<=l<r<=w and 0<=t<b<=h
            assert image.crop((x,y,x+w,y+h)).getchannel('A').getbbox()
for name in ['lizard-scale','reptile-tail','camel-hide','camel-milk']:
    assert (assets/'Items/Frontier'/('frontier-'+name+'.png')).exists()
terrain_art=json.loads((assets/'World/frontier-terrain-v1.json').read_text())
assert hashlib.sha256((assets/'World/frontier-terrain-v1.png').read_bytes()).hexdigest()==terrain_art['sha256']
print(f'PASS: {len(nodes)} nodes ({ore} ore), 11 terrain routes/22 objective paths, 48 animals/20 desert, 48 new animal poses and 20 materials/loot loops')
