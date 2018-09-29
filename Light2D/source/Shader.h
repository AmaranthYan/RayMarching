#pragma once
#include <glad/glad.h>
#include <iostream>

#define MAX_SHADER_SIZE 16384 // single shader file cannot exceed 16 kb

class Shader
{
public:
	Shader(const char *vertex_file, const char *fragment_file);
	Shader(const char *compute_file);
	// remove copy constructor/assignment
	Shader(const Shader &) = delete;
	Shader &operator=(const Shader &) = delete;
	// define move constructor/assignment
	Shader(Shader &&shader) : shader_program(shader.shader_program)
	{
		shader.shader_program = 0;
	}
	Shader &operator=(Shader &&shader)
	{
		if (this != &shader)
		{
			// release prev shader
			Release();
			std::swap(shader_program, shader.shader_program);
		}
	}
	// release shader on destruct
	~Shader()
	{
		Release();
	}

	void Use();
	int GetUniform(const char *param_name);

private:
	unsigned int shader_program = 0;

	void Release()
	{
		glDeleteProgram(shader_program);
		shader_program = 0;
	}

	void Load(const char *shader_file, char *source);
	unsigned int Compile(const int shader_type, const char *shader_file);
	void Link(const unsigned int vertex_shader, const unsigned int fragment_shader);
	void Link(const unsigned int compute_shader);
};