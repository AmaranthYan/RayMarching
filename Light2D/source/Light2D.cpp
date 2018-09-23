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


unsigned int colorBuffer;
unsigned int iteration;

int windowWidth = DEFAULT_WIDTH, windowHeight = DEFAULT_HEIGHT;
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
	int uniform_noise_size = shaderProgram.GetUniform("noise_size");
	int uniform_rotation = shaderProgram.GetUniform("rotation");
	int uniform_iteration = shaderProgram.GetUniform("iteration");
	float rot = 0;

	shaderProgram.Use();
	glUniform1i(shaderProgram.GetUniform("noise_map"), 0);
	glUniform2ui(uniform_noise_size, width, height);

	glUniform1i(shaderProgram.GetUniform("frame"), 1);
	
	std::cout << shaderProgram.GetUniform("noise_map") << std::endl;


	Shader shaderProgram2("shader/screen.vert", "shader/screen.frag");

	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	int frameRate = 0;
	double timer = 0;

	std::cout << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Start Rendering Loop" << std::endl;
	iteration = 0;

	int angle[256] = 
	{
		12, 170, 95, 97, 243, 173, 102, 55, 132, 239, 149, 230, 87, 29, 203, 58,
		227, 37, 183, 167, 44, 219, 63, 108, 169, 126, 111, 251, 168, 17, 38, 30,
		162, 211, 0, 24, 19, 45, 136, 77, 244, 123, 231, 181, 92, 131, 25, 73,
		53, 152, 216, 143, 11, 21, 99, 159, 225, 40, 113, 81, 153, 242, 232, 67,
		139, 119, 3, 98, 128, 240, 51, 201, 224, 226, 207, 26, 43, 6, 142, 79,
		246, 76, 8, 125, 105, 196, 208, 179, 112, 228, 52, 223, 229, 130, 106, 198,
		192, 48, 5, 14, 33, 80, 18, 57, 78, 220, 84, 120, 234, 72, 61, 46,
		156, 212, 85, 254, 194, 215, 165, 74, 245, 47, 146, 164, 157, 59, 13, 188,
		4, 182, 236, 15, 221, 7, 66, 134, 247, 241, 101, 160, 91, 56, 222, 205,
		49, 186, 107, 238, 68, 138, 70, 16, 210, 83, 237, 127, 122, 202, 249, 75,
		176, 193, 189, 174, 154, 148, 214, 218, 187, 34, 135, 255, 22, 32, 28, 88,
		190, 117, 42, 151, 171, 60, 161, 140, 133, 175, 2, 200, 204, 118, 36, 114,
		248, 166, 86, 158, 177, 155, 185, 115, 172, 217, 197, 69, 141, 39, 10, 129,
		96, 199, 213, 206, 124, 93, 71, 145, 54, 137, 209, 233, 103, 64, 82, 23,
		121, 252, 90, 41, 184, 253, 104, 150, 147, 191, 235, 65, 1, 31, 94, 163,
		195, 116, 50, 27, 178, 180, 20, 250, 9, 62, 110, 89, 109, 144, 100, 35,
	};
	for (int i = 0; i < 256; i++)
	{
		//angle[i] = i;
	}

	while (!glfwWindowShouldClose(window))
	{
		//std::cout << glGetError() << std::endl;

		auto startFrame = std::chrono::high_resolution_clock::now();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		// rendering

		if (iteration < 16)
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
			glUniform1f(uniform_rotation, rot);
			glUniform1iv(shaderProgram.GetUniform("rangle"), 16, angle+iteration * 16);

			glUniform1i(uniform_iteration, iteration++);
			rot += 0.01;

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