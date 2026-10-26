#version 330

layout(location=0) out vec4 FragColor;

uniform vec4 u_Color;
uniform float u_Time;
uniform int u_EffectType;
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

void main()
{
	vec4 color = u_Color;
	if (u_EffectType == 1)
	{
		float height = clamp((v_LocalPosition.y + 1.0) * 0.5, 0.0, 1.0);
		vec2 flow = vec2(v_LocalPosition.x * 3.5, height * 5.0 - u_Time * 2.8);
		float turbulence = Noise(flow) * .65 + Noise(flow * 2.1) * .35;
		float sway = sin(u_Time * 3.0 + height * 6.0) * height * .16;
		float width = mix(.66, .02, height) + (turbulence - .5) * .26;
		float distanceToFlame = abs(v_LocalPosition.x + sway);
		float flame = (1.0 - smoothstep(width - .08, width + .04, distanceToFlame)) *
			(1.0 - smoothstep(.72 + turbulence * .16, 1.0, height)) * smoothstep(0.0, .08, height);
		float core = (1.0 - smoothstep(.08, .32, distanceToFlame)) *
			(1.0 - smoothstep(.18, .62, height)) * flame;
		float flicker = .88 + .12 * sin(u_Time * 11.0 + turbulence * 3.0);
		float sparks = 0.0;
		for (int index = 0; index < 4; ++index)
		{
			float seed = float(index);
			float rise = fract(u_Time * (.28 + seed * .035) + seed * .27);
			vec2 spark = vec2(sin(seed * 5.0 + u_Time * 2.0) * (.08 + rise * .28), rise * 1.8 - .8);
			float distanceToSpark = length((v_LocalPosition - spark) * vec2(1.0, .7));
			sparks += (1.0 - smoothstep(.008, .035, distanceToSpark)) * (1.0 - rise);
		}
		color.rgb = mix(vec3(.95, .14 + turbulence * .17, .012), vec3(1.0, .92, .52), core);
		color.rgb = mix(color.rgb, vec3(1.0, .65, .16), clamp(sparks, 0.0, 1.0));
		color.a *= clamp(flame * flicker + sparks, 0.0, 1.0);
	}
	else if (u_EffectType == 2)
	{
		vec2 p = v_LocalPosition;
		float radius = length(p);
		float edge = max(fwidth(radius), .002);
		float mask = 1.0 - smoothstep(1.0 - edge, 1.0, radius);
		float firstWave = dot(p, vec2(19.0, 8.0)) - u_Time * 1.4;
		float secondWave = dot(p, vec2(-11.0, 23.0)) - u_Time * 1.1;
		vec2 slope = cos(firstWave) * vec2(.14, .06) + cos(secondWave) * vec2(-.08, .17);
		vec3 normal = normalize(vec3(-slope, 1.0));
		vec3 light = normalize(vec3(-.35, .45, 1.0));
		vec3 view = normalize(vec3(0.0, -.55, 1.0));
		float glint = pow(max(dot(normal, normalize(light + view)), 0.0), 90.0);
		float shallow = smoothstep(.55, 1.0, radius);
		vec3 water = mix(u_Color.rgb * .6, vec3(.20, .39, .32), shallow);
		float ripples = sin(firstWave) * sin(secondWave);
		water += vec3(.035, .065, .06) * ripples;
		float reflection = pow(1.0 - max(dot(normal, view), 0.0), 3.0);
		water = mix(water, vec3(.48, .64, .69), .12 + reflection * .55);
		float foam = smoothstep(.90, .98, radius) *
			(1.0 - smoothstep(.985, 1.0, radius)) * (.4 + .6 * Noise(p * 28.0 + u_Time * .35));
		color.rgb = water + vec3(.72, .78, .66) * foam * .35 + vec3(.85, .87, .72) * glint * .45;
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
		float shade = mix(p.x < 0.0 ? .92 : .64, 1.18, top);
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
	FragColor = color;
}
