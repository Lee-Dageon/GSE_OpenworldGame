#version 330

in vec2 v_UV;
layout(location = 0) out vec4 FragColor;
uniform sampler2D u_Source;
uniform vec2 u_Resolution;
uniform float u_Time;

float randomNoise(vec2 position)
{
	return fract(sin(dot(position, vec2(12.9898, 78.233)) + u_Time) * 43758.5453);
}

void main()
{
	vec2 texel = 1.0 / u_Resolution;
	vec2 centered = v_UV * 2.0 - 1.0;
	float edge = dot(centered, centered);
	vec3 color = texture(u_Source, v_UV).rgb;
	vec3 glow = texture(u_Source, v_UV + vec2(texel.x * 2.0, 0.0)).rgb;
	glow += texture(u_Source, v_UV - vec2(texel.x * 2.0, 0.0)).rgb;
	glow += texture(u_Source, v_UV + vec2(0.0, texel.y * 2.0)).rgb;
	glow += texture(u_Source, v_UV - vec2(0.0, texel.y * 2.0)).rgb;
	glow *= 0.25;
	color += max(glow - 0.72, 0.0) * 0.22;

	color *= vec3(1.04, 1.02, 0.96);
	color = color / (color + vec3(0.62));
	color = pow(color, vec3(0.86));
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	color = mix(vec3(luminance), color, 1.08);

	float vignette = 1.0 - smoothstep(0.35, 1.28, edge);
	color *= mix(0.82, 1.0, vignette);
	color += (randomNoise(gl_FragCoord.xy) - 0.5) * 0.006;
	FragColor = vec4(color, 1.0);
}
