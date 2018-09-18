#version 460 core
layout(location = 0) in vec3 position;

out vec4 gl_Position;

void main()
{
	gl_Position = vec4(position.x, position.y, position.z, 1.0);	
}