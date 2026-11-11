#version 330

in vec2 v_UV;
layout(location = 0) out vec4 FragColor;
uniform sampler2D u_Texture;
uniform float u_Alpha;
uniform float u_AlphaCutoff;
uniform vec3 u_Tint;
uniform vec4 u_UvRect;
uniform bool u_WorldLighting;
uniform vec3 u_LocalLight;

void main()
{
	vec2 halfTexel = .5 / vec2(textureSize(u_Texture, 0));
	vec2 uv = clamp(v_UV, u_UvRect.xy + halfTexel, u_UvRect.zw - halfTexel);
	vec4 color = texture(u_Texture, uv);
	if (color.a < u_AlphaCutoff)
	{
		discard;
	}
	vec3 surface = color.rgb * u_Tint;
	if (u_WorldLighting)
	{
		// Atlas lighting is baked; apply only shared ambient and local illumination.
		surface = pow(max(surface, vec3(0.0)), vec3(2.2));
		surface = min(surface * (vec3(.96, .98, 1.0) + u_LocalLight), vec3(.95));
	}
	FragColor = vec4(surface, color.a * u_Alpha);
}
