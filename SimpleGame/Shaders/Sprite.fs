#version 330

in vec2 v_UV;
layout(location = 0) out vec4 FragColor;
uniform sampler2D u_Texture;
uniform float u_Alpha;
uniform vec3 u_Tint;
uniform vec4 u_UvRect;

void main()
{
	vec2 halfTexel = .5 / vec2(textureSize(u_Texture, 0));
	vec2 uv = clamp(v_UV, u_UvRect.xy + halfTexel, u_UvRect.zw - halfTexel);
	vec4 color = texture(u_Texture, uv);
	if (color.a < 0.03)
	{
		discard;
	}
	FragColor = vec4(color.rgb * u_Tint, color.a * u_Alpha);
}
