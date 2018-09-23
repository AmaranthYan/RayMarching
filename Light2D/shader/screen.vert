#version 460 core
layout(location = 0) in vec3 position;
layout (location = 1) in vec2 in_tex_coords;

out vec4 gl_Position;
out vec2 tex_coords;

void main()
{
	gl_Position = vec4(position.x, position.y, position.z, 1.0);
	tex_coords = in_tex_coords;
}