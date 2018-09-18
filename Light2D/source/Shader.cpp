#include <assert.h>
#include "Shader.h"

#define LOG_SIZE 512

using namespace std;

Shader::Shader(const char *vertex_file, const char *fragment_file)
{
	unsigned int vertex_shader = Compile(GL_VERTEX_SHADER, vertex_file);
	unsigned int fragment_shader = Compile(GL_FRAGMENT_SHADER, fragment_file);
		
	shader_program = glCreateProgram();
	Link(vertex_shader, fragment_shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

void Shader::Use()
{
	glUseProgram(shader_program);
}

int Shader::GetUniform(const char *param_name)
{
	return glGetUniformLocation(shader_program, param_name);
}

void Shader::Load(const char *shader_file, char *source)
{
	FILE *stream;
	fopen_s(&stream, shader_file, "r");
	if (stream != nullptr)
	{
		size_t size = fread(source, sizeof(char), MAX_SHADER_SIZE, stream);
		source[size] = 0;
		fclose(stream);
	}
}

unsigned int Shader::Compile(const int shader_type, const char *shader_file)
{
	assert(shader_type == GL_VERTEX_SHADER || shader_type == GL_FRAGMENT_SHADER);

	int success;
	char info_log[LOG_SIZE];

	char source[MAX_SHADER_SIZE];
	Load(shader_file, source);

	unsigned int shader;
	shader = glCreateShader(shader_type);
	const GLchar *const source_ptr = static_cast<const GLchar *const>(source);
	glShaderSource(shader, 1, &source_ptr, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, LOG_SIZE, NULL, info_log);
		std::cout << "shader compile failed " << shader_file << info_log << std::endl;
	}
	return shader;
}

void Shader::Link(const unsigned int vertex_shader, const unsigned int fragment_shader)
{
	int success;
	char info_log[LOG_SIZE];

	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shader_program, LOG_SIZE, NULL, info_log);
		std::cout << "shader program link failed" << info_log << std::endl;
	}
}