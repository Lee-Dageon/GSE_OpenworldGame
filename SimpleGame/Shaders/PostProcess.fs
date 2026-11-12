#version 330

in vec2 v_UV;
layout(location = 0) out vec4 FragColor;
uniform sampler2D u_Source;
uniform vec2 u_Resolution;
uniform float u_Time;
uniform vec2 u_CameraShake;
uniform float u_DamageFlash;

vec3 Emission(vec2 uv)
{
	vec3 sampleColor = texture(u_Source, uv).rgb;
	return max(sampleColor - vec3(1.0), vec3(0.0));
}

void main()
{
	vec2 texel = 1.0 / u_Resolution;
	// Integer screen pixels keep world sprites/effects on one grid; HUD is drawn afterwards.
	float pixelScale = max(1.0, floor(u_Resolution.y / 360.0));
	vec2 shakenUV = clamp(v_UV + u_CameraShake / u_Resolution, vec2(0.0), vec2(1.0));
	vec2 pixelUV = (floor(shakenUV * u_Resolution / pixelScale) * pixelScale + pixelScale * .5) / u_Resolution;
	vec2 centered = v_UV * 2.0 - 1.0;
	float edge = dot(centered, centered);
	vec3 color = texture(u_Source, pixelUV).rgb;
	vec3 glow = vec3(0.0);
	// Only HDR emission exceeds one; diffuse surfaces cannot feed this bloom.
	for (int ring = 1; ring <= 3; ++ring)
	{
		vec2 radius = texel * float(ring * 3) * (u_Resolution.y / 720.0);
		for (int direction = 0; direction < 8; ++direction)
		{
			float angle = float(direction) * .78539816;
			glow += Emission(pixelUV + vec2(cos(angle), sin(angle)) * radius) / 24.0;
		}
	}
	color += glow * .22;
	color *= vec3(1.025, 1.0, .96);
	color = (color * (2.51 * color + .03)) / (color * (2.43 * color + .59) + .14);
	color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	color = mix(vec3(luminance), color, .96);

	float vignette = 1.0 - smoothstep(0.35, 1.28, edge);
	color *= mix(0.97, 1.0, vignette);
	float border = smoothstep(.55, 1.0, max(abs(centered.x), abs(centered.y)));
	color = mix(color, vec3(.72, .025, .015), border * clamp(u_DamageFlash, 0.0, 1.0) * .65);
	FragColor = vec4(color, 1.0);
}
