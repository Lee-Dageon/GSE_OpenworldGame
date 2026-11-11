#version 330

in vec3 a_Position;
uniform vec4 u_Trans;
uniform vec4 u_UvRect;
uniform float u_Sway;
uniform float u_SwayPivot;
uniform float u_Rotation;
uniform float u_Aspect;
out vec2 v_UV;

void main()
{
	vec2 position = a_Position.xy;
	// Linear shear keeps the measured ground pivot fixed after vertex interpolation.
	position.x += u_Sway * ((position.y + 1.0) * .5 - u_SwayPivot) / max(.01, 1.0 - u_SwayPivot);
	vec2 offset = position * u_Trans.zw;
	// Rotate in pixel space so non-square windows do not stretch the sprite.
	offset.x *= u_Aspect;
	float c = cos(u_Rotation), s = sin(u_Rotation);
	offset = mat2(c, s, -s, c) * offset;
	offset.x /= u_Aspect;
	gl_Position = vec4(offset + u_Trans.xy, 0.0, 1.0);
	vec2 localUV = a_Position.xy * 0.5 + 0.5;
	v_UV.x = mix(u_UvRect.x, u_UvRect.z, localUV.x);
	v_UV.y = mix(u_UvRect.w, u_UvRect.y, localUV.y);
}
