#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "NoiseGenerator.h"

using namespace std::chrono;

#define DEFAULT_WIDTH 1920
#define DEFAULT_HEIGHT 1080

#define ITERATION 32

unsigned int colorBuffer;
unsigned int iteration;

int windowWidth = DEFAULT_WIDTH, windowHeight = DEFAULT_HEIGHT;
double cursorX, cursorY;
bool editMode = true;
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	windowWidth = width;
	windowHeight = height;

	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);	

	iteration = 0;

	std::cout << "Resize viewport to " << width << " x " << height << std::endl;
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (editMode)
	{
		cursorX = xpos;
		cursorY = windowHeight - ypos;
		iteration = 0;

		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
		glClearTexImage(colorBuffer, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
	}

	std::cout << "Cursor pos " << xpos << ", " << ypos << std::endl;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		editMode = !editMode;
	}
}

int main(int argc, char * argv[])
{	
	//NoiseGenerator generator(42);
	//generator.CreateFloatNoiseTexture("gray.png", 1024);
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, "Light2D", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// load noise texture
	int width, height, nrChannels;
	stbi_us *data = stbi_load_16("noise_map.png", &width, &height, &nrChannels, 0);
	for (int i = 0; i < width * height * nrChannels; i++)
	{
		data[i] = ((data[i] & 0xff) << 8) + ((data[i] & 0xff00) >> 8);
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	float vertices[] = {
		// positions		// texture coords
		 1.f,  1.f, 0.f,	1.f, 1.f,	// top right
		 1.f, -1.f, 0.f,	1.f, 0.f,	// bottom right
		-1.f, -1.f, 0.f,	0.f, 0.f,	// bottom left
		-1.f,  1.f, 0.f,	0.f, 1.f	// top left 
	};
	unsigned int indices[] = {
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};
	
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	
	// positions
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);	
	// texture coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_TRUE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	float *f = new float[width * height];
	for (int i = 0; i < width * height; i++)
	{
		f[i] = sinf(i)*20;
	}
	// disable mipmaps
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, (float*)data);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	//glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);

	Shader shaderProgram("shader/ray.vert", "shader/ray.frag");
	int uniform_WindowSize = shaderProgram.GetUniform("viewport_size");
	int uniform_NoiseSize = shaderProgram.GetUniform("noise_size");
	float rot = 0;

	shaderProgram.Use();
	glUniform1i(shaderProgram.GetUniform("noise_map"), 0);
	glUniform2ui(uniform_NoiseSize, width, height);

	glUniform1i(shaderProgram.GetUniform("frame_canvas"), 1);
	glUniform1i(shaderProgram.GetUniform("iteration_count"), ITERATION);

	int uniform_LightPos = shaderProgram.GetUniform("light1.position");
	int uniform_LightRad = shaderProgram.GetUniform("light1.radius");
	int uniform_LightLum = shaderProgram.GetUniform("light1.luminance");

	// light attributes
	glUniform1f(uniform_LightRad, 0.04);
	glUniform3f(uniform_LightLum, 8, 8, 8);
	
	//std::cout << shaderProgram.GetUniform("noise_map") << std::endl;

	Shader shaderProgram2("shader/screen.vert", "shader/screen.frag");

	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, DrawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int frameRate = 0;
	double timer = 0;

	std::cout << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Start Rendering Loop" << std::endl;
	iteration = 0;

	while (!glfwWindowShouldClose(window))
	{
		//std::cout << glGetError() << std::endl;

		auto startFrame = std::chrono::high_resolution_clock::now();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// rendering
		if (iteration < ITERATION)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FBO);

			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			//glClear(GL_COLOR_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, colorBuffer);

			shaderProgram.Use();
			glUniform2f(uniform_WindowSize, windowWidth, windowHeight);
			glUniform2f(uniform_LightPos, cursorX, cursorY);
			int a[16] = {};
			for (int i = 0; i < 16; i++)
			{
				a[i] = iteration * 16 + i;
			}
			
			glUniform1iv(shaderProgram.GetUniform("rangle"), 16, a);

			iteration++;

			glBindVertexArray(VAO);
			//glDrawArrays(GL_TRIANGLES, 0, 3);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		shaderProgram2.Use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);		

		glfwSwapBuffers(window);
		glfwPollEvents();		

		auto endFrame = high_resolution_clock::now();
		double deltaTime = duration_cast<duration<double>>(endFrame - startFrame).count();
		frameRate++;

		timer += deltaTime;
		if (timer > 1)
		{
			std::cout << frameRate  << std::endl;
			timer = 0;
			frameRate = 0;
		}
	}

	glfwTerminate();
	return 0;
}