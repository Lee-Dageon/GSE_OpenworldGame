#version 330

in vec3 a_Position;
uniform vec4 u_Trans;
uniform vec4 u_UvRect;
out vec2 v_UV;

void main()
{
	gl_Position = vec4(a_Position.xy * u_Trans.zw + u_Trans.xy, 0.0, 1.0);
	vec2 localUV = a_Position.xy * 0.5 + 0.5;
	v_UV.x = mix(u_UvRect.x, u_UvRect.z, localUV.x);
	v_UV.y = mix(u_UvRect.w, u_UvRect.y, localUV.y);
}
