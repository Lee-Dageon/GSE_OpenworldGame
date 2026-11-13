#version 330

in vec3 a_Position;
uniform vec4 u_Trans;
uniform float u_Rotation;
uniform bool u_UseBasis;
uniform vec4 u_Basis;
out vec2 v_LocalPosition;

void main()
{
	float sine = sin(u_Rotation);
	float cosine = cos(u_Rotation);
	vec2 rotated = vec2(
		a_Position.x * cosine - a_Position.y * sine,
		a_Position.x * sine + a_Position.y * cosine);
	v_LocalPosition = a_Position.xy;
	gl_Position = vec4(rotated * u_Trans.zw + u_Trans.xy, 0.0, 1.0);
	if (u_UseBasis)
		gl_Position = vec4(u_Basis.xy * a_Position.x + u_Basis.zw * a_Position.y + u_Trans.xy, 0.0, 1.0);
}
