#version 330

in vec3 a_Position;
out vec2 v_UV;

void main()
{
	gl_Position = vec4(a_Position, 1.0);
	v_UV = a_Position.xy * 0.5 + 0.5;
}
