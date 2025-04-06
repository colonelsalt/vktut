#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <cstdint>
#include <cstdio>

typedef uint32_t u32;
typedef int32_t s32;
typedef uint32_t b32;

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* Window = glfwCreateWindow(800, 600, "This is a window, mate", nullptr, nullptr);

	u32 NumExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &NumExtensions, nullptr);

	printf("Looks like we support %u extensions, bro\n", NumExtensions);

	glm::mat4 Matrix;
	glm::vec4 Vector;

	glm::vec4 Test = Matrix * Vector;

	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(Window);
	glfwTerminate();
}