#version 330

in vec2 v_UV;
layout(location = 0) out vec4 FragColor;
uniform sampler2D u_Texture;
uniform float u_Alpha;
uniform vec3 u_Tint;

void main()
{
	vec4 color = texture(u_Texture, v_UV);
	if (color.a < 0.03)
	{
		discard;
	}
	FragColor = vec4(color.rgb * u_Tint, color.a * u_Alpha);
}
