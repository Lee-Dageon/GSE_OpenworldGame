"""Offline asset and stair-projection checks; no build, game launch or capture."""
import hashlib
import json
import math
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
game = root / 'SimpleGame'
texture = game / 'Assets/PixelArt/World/frontier-sandstone-steps-v2.png'
meta = json.loads(texture.with_suffix('.json').read_text(encoding='utf-8'))
assert hashlib.sha256(texture.read_bytes()).hexdigest() == meta['sha256']
with Image.open(texture) as image:
    assert list(image.size) == meta['dimensions'] and min(image.size) >= 1024
    assert image.convert('RGBA').getchannel('A').getextrema() == (255, 255)
    assert len(image.convert('RGB').getcolors(image.width * image.height)) > 128

def project(x, z, height=0):
    return (x-z)*32, (x+z)*16 + height*20

def height(t, peak):
    def smooth(v):
        v = max(0, min(1, v))
        return v*v*(3-2*v)
    return peak*min(smooth(t/.22), smooth((1-t)/.22))

assert project(1, 0) == (32, 16) and project(0, 1) == (-32, 16)
count = 0
for line in (game/'Data/map.gse').read_text(encoding='utf-8').splitlines():
    row = line.split()
    if not row or row[0] != 'terrain':
        continue
    sx, sz, ex, ez, width, peak = map(float, row[3:])
    length = math.hypot(ex-sx, ez-sz)
    segments = math.ceil(max(length/.65, peak*24))
    step_height = peak/math.ceil(peak/.65)
    def stair_height(t):
        return math.floor(height(t, peak)/step_height+.5)*step_height
    visible_steps = 0
    dx, dz = (ex-sx)/length, (ez-sz)/length
    # Adjacent horizontal treads must meet the same vertical riser's endpoints.
    for i in range(1, segments):
        t = i/segments
        lower, upper = stair_height((i-.5)/segments), stair_height((i+.5)/segments)
        if abs(upper-lower) > .001:
            visible_steps += 1
            assert 11 <= abs(upper-lower)*20 <= 13.001
        a = project(sx+dx*length*t, sz+dz*length*t, lower)
        b = project(sx+dx*length*t, sz+dz*length*t, upper)
        assert a[0] == b[0]
        assert math.isclose(b[1]-a[1], (upper-lower)*20, abs_tol=1e-8)
        # Coarser visible steps keep the existing smooth movement support.
        assert abs(height(t, peak)-lower)*20 < 10
    assert visible_steps >= 12
    count += 1
assert count == 11
source = (game/'WorldRenderer.cpp').read_text(encoding='utf-8')
shader = (game/'Shaders/SolidRect.fs').read_text(encoding='utf-8')
assert 'frontier-sandstone-steps-v2.png' in source
assert 'const float basis[]={ax,ay,' in source
assert 'texture(u_StepTexture, fract(uv))' in shader
assert 'if (layer==3 && index>0)' in source
assert 'const float stepHeight=strip.height/std::ceil(strip.height/.65f)' in source
assert 'vec3(.20,.25,.31),vec3(.78,.85,.91)' in shader
assert 'glBindTexture(GL_TEXTURE_2D,previousTexture)' in source
print('PASS: stone asset/hash, cool palette, 2:1 axes, 11 strips with 11-13 pixel risers, overlay order and smooth movement tolerance')
