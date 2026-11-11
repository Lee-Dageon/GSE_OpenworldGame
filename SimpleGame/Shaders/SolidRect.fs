#version 330

layout(location=0) out vec4 FragColor;

uniform vec4 u_Color;
uniform float u_Time;
uniform int u_EffectType;
uniform sampler2D u_TerrainTexture;
uniform int u_TerrainColumn;
uniform vec2 u_TerrainTileSize;
uniform vec2 u_RoadHouses[32];
uniform vec2 u_RoadBends[32];
uniform float u_RoadJunctions[32];
uniform int u_RoadHouseCount;
uniform vec2 u_RoadPlaza;
uniform bool u_WorldLighting;
uniform vec3 u_LocalLight;
uniform vec2 u_Campfires[32];
uniform int u_CampfireCount;
uniform int u_RegionCount;
uniform vec4 u_RegionBounds[16];
uniform vec3 u_RegionColors[16];
in vec2 v_LocalPosition;

float Hash(vec2 value)
{
	return fract(sin(dot(value, vec2(127.1, 311.7))) * 43758.5453);
}

float Noise(vec2 position)
{
	vec2 cell = floor(position);
	vec2 blend = fract(position);
	blend = blend * blend * (3.0 - 2.0 * blend);
	return mix(mix(Hash(cell), Hash(cell + vec2(1.0, 0.0)), blend.x),
		mix(Hash(cell + vec2(0.0, 1.0)), Hash(cell + vec2(1.0, 1.0)), blend.x), blend.y);
}

float LineDistance(vec2 p, vec2 a, vec2 b)
{
	vec2 segment = b - a;
	return length(p - a - segment * clamp(dot(p-a, segment) / max(dot(segment, segment), .0001), 0.0, 1.0));
}

float RoadDistance(vec2 p)
{
	float roadZ=clamp(p.y,-55.0,65.0);
	float distanceToRoad=length(p-vec2(3.5*sin(roadZ*.065),roadZ))-(1.55+.22*cos(roadZ*.11));
	for (int index=0;index<u_RoadHouseCount;++index)
	{
		float junction=u_RoadJunctions[index];
		vec2 start=vec2(3.5*sin(junction*.065),junction);
		vec2 door=u_RoadHouses[index]+vec2(.78,-.28);
		float width=.48+.44*clamp(length(p-door)/max(length(start-door),.01),0.0,1.0);
		distanceToRoad=min(distanceToRoad,LineDistance(p,start,u_RoadBends[index])-width);
		distanceToRoad=min(distanceToRoad,LineDistance(p,u_RoadBends[index],door)-width);
	}
	float plaza=(length((p-u_RoadPlaza)/vec2(3.4,2.6))-1.0)*2.6;
	return min(distanceToRoad,plaza);
}

void main()
{
	vec4 color = u_Color;
	if (u_EffectType == 1)
	{
		float height = clamp((v_LocalPosition.y + 1.0) * 0.5, 0.0, 1.0);
		vec2 flow = vec2(v_LocalPosition.x * 3.5, height * 5.0 - u_Time * 2.8);
		flow.x += (Noise(flow * .65 + vec2(13.7, 5.2)) - .5) * 1.8 * height;
		float turbulence = Noise(flow) * .55 + Noise(flow * 2.1) * .30 + Noise(flow * 4.3) * .15;
		float sway = (Noise(vec2(height * 3.0 - u_Time * 1.4, 8.3)) - .5) * height * .5;
		float width = mix(.66, .0, height) + (turbulence - .5) * mix(.12, .65, height);
		float distanceToFlame = abs(v_LocalPosition.x + sway);
		float flame = (1.0 - smoothstep(width - .08, width + .04, distanceToFlame)) *
			(1.0 - smoothstep(.72 + turbulence * .16, 1.0, height)) * smoothstep(0.0, .08, height);
		float core = (1.0 - smoothstep(.02, max(.04, width * .70), distanceToFlame)) *
			(1.0 - smoothstep(.12, .72, height + (turbulence - .5) * .20)) * flame;
		float flicker = .85 + .15 * Noise(vec2(u_Time * 9.0, 4.7));
		float sparks = 0.0;
		for (int index = 0; index < 4; ++index)
		{
			float seed = float(index);
			float rise = fract(u_Time * (.28 + seed * .035) + seed * .27);
			float cycle = floor(u_Time * (.28 + seed * .035) + seed * .27);
			float drift = Hash(vec2(seed, cycle)) * 2.0 - 1.0;
			vec2 spark = vec2(drift * (.12 + rise * .38) + sin(rise * 9.0 + seed) * rise * .06, rise * 1.8 - .8);
			float distanceToSpark = length((v_LocalPosition - spark) * vec2(1.0, .7));
			sparks += (1.0 - smoothstep(.004, .025, distanceToSpark)) * smoothstep(0.0, .12, rise) * (1.0 - smoothstep(.55, 1.0, rise));
		}
		vec3 outerFlame = mix(vec3(.65, .055, .006), vec3(1.0, .40, .025), smoothstep(.15, .85, turbulence));
		color.rgb = mix(outerFlame, vec3(1.0, .95, .70), core);
		color.rgb = mix(color.rgb, vec3(1.0, .65, .16), clamp(sparks, 0.0, 1.0));
		color.a *= clamp(flame * flicker + sparks, 0.0, 1.0);
	}
	else if (u_EffectType == 2)
	{
		vec2 p = v_LocalPosition;
		float radius = length(p);
		float edge = max(fwidth(radius), .002);
		float mask = 1.0 - smoothstep(1.0 - edge, 1.0, radius);
		// Short, crossing ripples replace lake-sized bands of white reflection.
		vec2 ripplePosition = p + vec2(.018, .009)*vec2(
			Noise(p*vec2(18.0, 46.0)+vec2(u_Time*.035, -u_Time*.025)),
			Noise(p*vec2(31.0, 67.0)+vec2(-u_Time*.025, u_Time*.018)));
		float firstWave = dot(ripplePosition, vec2(32.0, 280.0)) - u_Time * 1.6;
		float secondWave = dot(ripplePosition, vec2(-48.0, 430.0)) - u_Time * 1.2;
		float detailWave = dot(ripplePosition, vec2(75.0, 610.0)) - u_Time * 2.1;
		float firstFade = 1.0-smoothstep(.6, 2.5, fwidth(firstWave));
		float secondFade = 1.0-smoothstep(.6, 2.5, fwidth(secondWave));
		float detailFade = 1.0-smoothstep(.6, 2.5, fwidth(detailWave));
		float breakup = Noise(p*vec2(36.0, 95.0) + vec2(u_Time*.06, -u_Time*.025));
		vec2 slope = cos(firstWave)*vec2(.045,.023)*firstFade
			+ cos(secondWave)*vec2(-.022,.040)*secondFade
			+ cos(detailWave)*vec2(.016,.012)*detailFade;
		slope *= mix(.65, 1.65, breakup);
		vec3 normal = normalize(vec3(-slope, 1.0));
		vec3 light = normalize(vec3(-.12, .55, 1.0));
		vec3 view = normalize(vec3(0.0, -.55, 1.0));
		float glint = pow(max(dot(normal, normalize(light + view)), 0.0), 600.0)
			* smoothstep(.42, .82, breakup)*firstFade*secondFade;
		float shallow = smoothstep(.78, 1.0, radius);
		vec3 water = mix(u_Color.rgb*vec3(.55,.72,.80), vec3(.15,.31,.25), shallow);
		float ripples = sin(firstWave)*firstFade*.55 + sin(secondWave)*secondFade*.30
			+ sin(detailWave)*detailFade*.15;
		water += vec3(.012,.025,.027)*ripples;
		float reflection = .02 + .98*pow(1.0 - max(dot(normal, view), 0.0), 5.0);
		vec3 sky = mix(vec3(.12,.25,.28), vec3(.42,.57,.61), smoothstep(-.06,.08,normal.y));
		water = mix(water, sky, reflection);
		float shoreWave = sin(radius*480.0-u_Time*1.6+breakup*3.0);
		float foam = smoothstep(.965,.987,radius)*(1.0-smoothstep(.993,1.0,radius))
			* smoothstep(.35,.80,breakup)*smoothstep(.3,.9,shoreWave);
		color.rgb = water + vec3(.44,.57,.49)*foam*.22 + vec3(.72,.84,.81)*glint*.22;
		color.a *= mask;
	}
	else if (u_EffectType == 3)
	{
		float distanceToCenter = length(v_LocalPosition);
		color.a *= 1.0 - smoothstep(0.12, 1.0, distanceToCenter);
	}
	else if (u_EffectType == 4)
	{
		// Isometric prism: shared silhouette, bright top, two shaded upright faces.
		vec2 p = v_LocalPosition;
		float silhouette = min(1.0 - .24 * abs(p.x) - p.y, p.y + 1.0 - .24 * abs(p.x));
		float edge = max(fwidth(silhouette), .002);
		float top = step(.52 + .24 * abs(p.x), p.y);
		float shade = mix(p.x < 0.0 ? .92 : .64, 1.10, top);
		shade *= .94 + .10 * Noise(p * vec2(4.0, 28.0));
		color.rgb *= shade;
		color.a *= smoothstep(0.0, edge, silhouette);
	}
	else if (u_EffectType == 5)
	{
		// Faceted foliage/roof with a diamond footprint and a central ridge.
		vec2 p = v_LocalPosition;
		float silhouette = 1.0 - abs(p.x) - abs(p.y);
		float edge = max(fwidth(silhouette), .002);
		float shade = p.y > 0.0 ? (p.x < 0.0 ? 1.18 : .94) : (p.x < 0.0 ? .82 : .60);
		color.rgb *= shade;
		color.a *= smoothstep(0.0, edge, silhouette);
	}
	else if (u_EffectType == 6)
	{
		// Recover world X/Z from cached positions; texture phase stays fixed as the camera moves.
		vec2 world = vec2(v_LocalPosition.x / u_TerrainTileSize.x + v_LocalPosition.y / u_TerrainTileSize.y,
			v_LocalPosition.y / u_TerrainTileSize.y - v_LocalPosition.x / u_TerrainTileSize.x);
		// Sample the tile's interior color, then add world-space grain instead of warping its baked edges.
		vec2 tileUV = vec2(.5);
		vec4 grass = texture(u_TerrainTexture, vec2(tileUV.x / 3.0, tileUV.y));
		vec4 dirt = texture(u_TerrainTexture, vec2((1.0 + tileUV.x) / 3.0, tileUV.y));
		grass.rgb = mix(grass.rgb, vec3(.23, .32, .16), .75);
		dirt.rgb = mix(dirt.rgb, vec3(.37, .30, .21), .75);
		vec2 warp = vec2(Noise(world * 1.8), Noise(world * 1.8 + 29.0)) - .5;
		float erosion = (Noise(world * 7.0) - .5) * .12 + (Noise(world * 1.4) - .5) * .35;
		float distanceToRoad = RoadDistance(world) + erosion;
		float road = 1.0 - smoothstep(-.20, .28, distanceToRoad);
		vec4 tile=mix(grass,dirt,road);
		float grain = Noise(world * 37.0);
		tile.rgb *= .89 + .20 * grain;
		vec2 pebbleCell = floor(world * 8.0);
		vec2 pebble = fract(world * 8.0) - vec2(Hash(pebbleCell), Hash(pebbleCell + 17.0));
		float pebbleMask = (1.0-smoothstep(.03,.13,length(pebble))) * step(.86,Hash(pebbleCell+33.0));
		tile.rgb = mix(tile.rgb, vec3(.42,.40,.33), pebbleMask * (.12 + road * .25));
		float patch = Noise(world * .16);
		tile.rgb *= mix(vec3(.84, .90, .78), vec3(1.08, 1.04, .94), patch);
		float forest = smoothstep(14.0, 40.0, world.y);
		float rockField = (1.0-smoothstep(3.0, 10.0, length(world-vec2(12,42))))
			+ (1.0-smoothstep(3.0, 9.0, length(world-vec2(34,28))));
		vec3 forestSoil = mix(vec3(.19,.23,.115), vec3(.28,.25,.15), Noise(world*.7));
		tile.rgb = mix(tile.rgb, forestSoil, forest * (1.0-road) * .55);
		vec3 rockColor = mix(vec3(.28,.29,.26), vec3(.39,.38,.32), Noise(world*2.5));
		tile.rgb = mix(tile.rgb, rockColor, clamp(rockField,0.0,1.0) * (1.0-road) * .75);
		for (int index = 0; index < u_RegionCount; ++index)
		{
			vec4 bounds = u_RegionBounds[index];
			vec2 edge = min(world-bounds.xy,bounds.zw-world);
			float coverage = smoothstep(0.0,1.2,min(edge.x,edge.y));
			vec3 regionColor = u_RegionColors[index] * (.90+.18*grain);
			tile.rgb = mix(tile.rgb,regionColor,coverage*.85*(1.0-road*.85));
		}
		float edgeSoil = (1.0 - smoothstep(.05, .65, abs(distanceToRoad))) * Noise(world * 9.0);
		tile.rgb = mix(tile.rgb, dirt.rgb * .78, edgeSoil * .24);
		tile.rgb *= .93 + .12 * Noise(world * 2.2 + warp);
		color.rgb = mix(color.rgb, tile.rgb, tile.a * .9);
		vec3 fireLight = vec3(0.0);
		for (int index = 0; index < u_CampfireCount; ++index)
		{
			float distanceToFire = length(world - u_Campfires[index]);
			float falloff = pow(max(0.0, 1.0 - distanceToFire / 4.5), 2.0);
			float flicker = .94 + .06 * sin(u_Time * 6.0 + float(index));
			fireLight += vec3(.90, .28, .055) * falloff * flicker;
		}
		color.rgb = min(pow(max(color.rgb, vec3(0)), vec3(2.2)) * (vec3(.96,.98,1) + fireLight), vec3(.95));
	}
	else if (u_EffectType >= 7 && u_EffectType <= 9)
	{
		vec2 p = v_LocalPosition;
		float radius = length(p);
		float mask = 1.0 - smoothstep(.975, 1.0, radius);
		if (u_EffectType == 7)
		{
			float light = .72 + .25 * (1.0 + p.y) + .18 * (1.0 - radius);
			color.rgb *= light;
		}
		else if (u_EffectType == 8)
		{
			float grid = max(1.0-smoothstep(.02,.045,abs(fract(p.x*5.0+.5)-.5)),
				1.0-smoothstep(.02,.045,abs(fract(p.y*5.0+.5)-.5)));
			float lake = 1.0-smoothstep(u_Color.b-.015,u_Color.b,length(p-u_Color.rg));
			color.rgb = mix(vec3(.10,.16,.105),vec3(.09,.25,.29),lake) + grid*.025;
		}
		else
		{
			float angle = fract(atan(p.x,p.y)/6.2831853+1.0);
			mask *= step(angle,u_Color.r);
			color.rgb = vec3(.015,.02,.025);
		}
		color.a *= mask;
	}
	else if (u_EffectType >= 10 && u_EffectType <= 13)
	{
		vec2 p = v_LocalPosition;
		float distanceToIcon;
		if (u_EffectType == 10)
		{
			vec2 q=vec2(p.x+p.y,p.y-p.x)*.7071068;
			distanceToIcon=min(LineDistance(q,vec2(0,-.65),vec2(0,.62))-.09,
				LineDistance(q,vec2(-.28,-.27),vec2(.28,-.27))-.065);
		}
		else if (u_EffectType == 11)
		{
			distanceToIcon=min(min(LineDistance(p,vec2(-.6,.5),vec2(-.05,0)),LineDistance(p,vec2(-.05,0),vec2(-.6,-.5))),
				min(LineDistance(p,vec2(-.1,.5),vec2(.5,0)),LineDistance(p,vec2(.5,0),vec2(-.1,-.5))))-.075;
		}
		else if (u_EffectType == 12)
		{
			distanceToIcon=min(length((p-vec2(0,-.22))*vec2(1, .85))-.47,
				max(abs(p.x)-.17,abs(p.y-.32)-.30));
			color.rgb=mix(u_Color.rgb,vec3(1,.76,.38),smoothstep(.38,.65,p.y));
		}
		else
		{
			distanceToIcon=min(abs(abs(p.x)+abs(p.y)-.65)*.7-.045,length(p)-.13);
		}
		color.a *= 1.0-smoothstep(-.02,.02,distanceToIcon);
		color.rgb *= .85+.25*(1.0+p.y);
	}
	else if (u_EffectType == 14)
	{
		vec2 p = v_LocalPosition;
		float blade = 0.0;
		for (int index = 0; index < 5; ++index)
		{
			float seed = float(index);
			float height = .35 + .6 * Hash(vec2(seed, u_Color.g));
			float rise = clamp((p.y + 1.0) / (height * 2.0), 0.0, 1.0);
			float bend = (sin(u_Time * 1.7 + seed * .7 + u_Color.r * 30.0) * .17 + (seed - 2.0) * .10) * rise * rise;
			float x = -.7 + seed * .35 + bend;
			float edge = max(fwidth(p.x), .025);
			blade = max(blade, (1.0 - smoothstep(.015, .015 + edge, abs(p.x - x) - .07 * (1.0-rise)))
				* (1.0 - smoothstep(height * 2.0 - 1.1, height * 2.0 - 1.0, p.y)));
		}
		color.a *= blade;
		color.rgb *= mix(.65, 1.15, (p.y + 1.0) * .5);
	}
	else if (u_EffectType == 15)
	{
		vec2 p = v_LocalPosition;
		float radius = length(p);
		float arc = smoothstep(.36, .46, p.y) * (1.0 - smoothstep(.88, 1.0, radius));
		color.a *= arc * smoothstep(.62, .78, radius);
	}
	else if (u_EffectType == 16 || u_EffectType == 19)
	{
		vec2 p = v_LocalPosition;
		float density = Noise(p * 3.5 + vec2(0, -u_Time * .3));
		color.a *= (1.0 - smoothstep(.15, 1.0, length(p))) * (.55 + .45 * density);
	}
	else if (u_EffectType == 17)
	{
		vec2 p = v_LocalPosition;
		float edge = 1.0 - abs(p.x) * .78 - abs(p.y) * .64;
		color.a *= smoothstep(0.0, max(fwidth(edge), .02), edge);
		color.rgb *= p.y > -.2 + .35 * p.x ? 1.1 : .65;
	}
	else if (u_EffectType == 18)
	{
		vec2 p = v_LocalPosition;
		float edge = 1.0 - abs(p.x) - p.y * p.y;
		color.a *= smoothstep(0.0, max(fwidth(edge), .02), edge);
		color.rgb *= .8 + .25 * p.x;
	}
	if (u_WorldLighting && u_EffectType != 6)
	{
		vec3 linearColor = pow(max(color.rgb, vec3(0.0)), vec3(2.2));
		color.rgb = min(linearColor * (vec3(.96,.98,1) + u_LocalLight), vec3(.95));
		if (u_EffectType == 1 || u_EffectType == 15)
			color.rgb = linearColor * 3.5;
	}
	FragColor = color;
}
