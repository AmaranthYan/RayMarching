#version 460 core

out vec4 FragColor;
  
in vec2 tex_coords;

uniform sampler2D screenTexture;

void main()
{ 
    FragColor = texture(screenTexture, tex_coords);
}