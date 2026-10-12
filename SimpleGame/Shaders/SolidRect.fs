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

void main()
{
	vec4 color = u_Color;
	if (u_EffectType == 1)
	{
		float height = clamp((v_LocalPosition.y + 1.0) * 0.5, 0.0, 1.0);
		float noise = Hash(floor(vec2(v_LocalPosition.x * 14.0, height * 18.0 + u_Time * 9.0)));
		float sway = sin(u_Time * 8.0 + height * 8.0) * (0.10 + height * 0.16);
		float width = mix(0.62, 0.045, height) + (noise - 0.5) * 0.10;
		float outerFlame = (1.0 - smoothstep(width - 0.07, width, abs(v_LocalPosition.x + sway))) *
			(1.0 - smoothstep(0.88, 1.0, height));

		float innerSway = sin(u_Time * 13.0 + height * 11.0) * 0.07;
		float innerWidth = mix(0.33, 0.02, height);
		float innerFlame = (1.0 - smoothstep(innerWidth - 0.06, innerWidth,
			abs(v_LocalPosition.x + innerSway))) * (1.0 - smoothstep(0.38, 0.78, height));
		float flicker = 0.82 + 0.18 * sin(u_Time * 17.0 + noise * 6.28318);
		vec3 outerColor = mix(vec3(0.72, 0.05, 0.005), vec3(1.0, 0.32, 0.015), height);
		vec3 coreColor = vec3(1.0, 0.88, 0.31);
		color.rgb = mix(outerColor, coreColor, innerFlame);
		color.a *= outerFlame * flicker;
	}
	else if (u_EffectType == 2)
	{
		float ring = abs(fract(length(v_LocalPosition) * 4.0 - u_Time * 1.8) - 0.5);
		color.a *= smoothstep(0.50, 0.18, ring) * 0.65;
	}
	else if (u_EffectType == 3)
	{
		float distanceToCenter = length(v_LocalPosition);
		color.a *= 1.0 - smoothstep(0.12, 1.0, distanceToCenter);
	}
	FragColor = color;
}
