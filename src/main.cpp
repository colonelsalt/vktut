#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint32_t b32;

#define AllocArray(T, N) ((T*)malloc(N * sizeof(T)));
#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) if (!(X)) __debugbreak()

static GLFWwindow* InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* Window = glfwCreateWindow(800, 600, "This is a window, mate", nullptr, nullptr);
	return Window;
}

static constexpr const char* VALIDATION_LAYERS[] =
{
	"VK_LAYER_KHRONOS_validation"
};

struct extensions_list
{
	const char** Names;
	u32 Count;
};

static extensions_list GetRequiredExtensions()
{
	extensions_list Result = {};

	u32 NumGlfwExtensions = 0;
	const char** GlfwExtensionNames = glfwGetRequiredInstanceExtensions(&NumGlfwExtensions);

#if _DEBUG
	Result.Count = NumGlfwExtensions + 1;
	Result.Names = AllocArray(const char*, Result.Count);
	for (u32 i = 0; i < NumGlfwExtensions; i++)
	{
		Result.Names[i] = GlfwExtensionNames[i];
	}
	Result.Names[NumGlfwExtensions] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
	Result.Count = NumGlfwExtensions;
	Result.Names = GlfwExtensionNames;
#endif
	return Result;
}


#if _DEBUG
static VkDebugUtilsMessengerEXT s_DebugHandle;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT SeverityBits,
													VkDebugUtilsMessageTypeFlagsEXT TypeFlags,
													const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
													void* UserData)
{
	if (SeverityBits >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		fprintf(stderr, "Validation layer: %s\n", CallbackData->pMessage);
	}
	return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT DebugCallbackCreateInfo()
{
	VkDebugUtilsMessengerCreateInfoEXT CreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = DebugCallback
	};
	return CreateInfo;
}

static void InitDebugCallback(VkInstance Instance)
{
	VkDebugUtilsMessengerCreateInfoEXT CreateInfo = DebugCallbackCreateInfo();
	PFN_vkCreateDebugUtilsMessengerEXT Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance,
																										"vkCreateDebugUtilsMessengerEXT");
	if (Func)
	{
		Func(Instance, &CreateInfo, nullptr, &s_DebugHandle);
	}
	else
	{
		fprintf(stderr, "Failed to set up debug callback function\n");
		Assert(false);
	}
}

static void DestroyDebugCallback(VkInstance Instance)
{
	PFN_vkDestroyDebugUtilsMessengerEXT Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance,
																										  "vkDestroyDebugUtilsMessengerEXT");
	if (Func)
	{
		Func(Instance, s_DebugHandle, nullptr);
	}
}

static b32 AreValidationLayersSupported()
{
	u32 NumValidationLayers = 0;
	vkEnumerateInstanceLayerProperties(&NumValidationLayers, nullptr);

	VkLayerProperties* AvailableLayers = AllocArray(VkLayerProperties, NumValidationLayers);
	vkEnumerateInstanceLayerProperties(&NumValidationLayers, AvailableLayers);
	b32 Result = true;
	for (u32 i = 0; i < ArrayCount(VALIDATION_LAYERS); i++)
	{
		const char* LayerName = VALIDATION_LAYERS[i];
		b32 FoundLayer = false;
		for (u32 j = 0; j < NumValidationLayers; j++)
		{
			if (strcmp(LayerName, AvailableLayers[j].layerName) == 0)
			{
				FoundLayer = true;
				break;
			}
		}
		if (!FoundLayer)
		{
			Result = false;
			break;
		}
	}

	free(AvailableLayers);
	return Result;
}
#endif


static VkInstance CreateInstance()
{
	VkInstance Instance = nullptr;

	VkApplicationInfo AppInfo
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Sondie's Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "SondieEngine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	u32 NumSupportedExtensions = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &NumSupportedExtensions, nullptr);
	VkExtensionProperties* SupportedExtensions = AllocArray(VkExtensionProperties, NumSupportedExtensions);
	vkEnumerateInstanceExtensionProperties(nullptr, &NumSupportedExtensions, SupportedExtensions);

	printf("Available extensions (%u):\n", NumSupportedExtensions);
	for (u32 i = 0; i < NumSupportedExtensions; i++)
	{
		printf("\t%s\n", SupportedExtensions[i].extensionName);
	}

	extensions_list RequiredExtensions = GetRequiredExtensions();
	u32 NumExtensionMatches = 0;
	for (u32 i = 0; i < RequiredExtensions.Count; i++)
	{
		for (u32 j = 0; j < NumSupportedExtensions; j++)
		{
			if (strcmp(RequiredExtensions.Names[i], SupportedExtensions[j].extensionName) == 0)
			{
				NumExtensionMatches++;
				break;
			}
		}
	}

	free(SupportedExtensions);
	if (NumExtensionMatches >= RequiredExtensions.Count)
	{
		printf("All %u required extensions are supported.\n", RequiredExtensions.Count);

		VkInstanceCreateInfo CreateInfo
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &AppInfo,
			.enabledExtensionCount = RequiredExtensions.Count,
			.ppEnabledExtensionNames = RequiredExtensions.Names
		};

#if _DEBUG
		Assert(AreValidationLayersSupported());
		VkDebugUtilsMessengerCreateInfoEXT DebugCreateInfo = DebugCallbackCreateInfo();

		CreateInfo.enabledLayerCount = ArrayCount(VALIDATION_LAYERS);
		CreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
		CreateInfo.pNext = &DebugCreateInfo;
#endif

		if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS)
		{
			fprintf(stderr, "ERROR: Failed to create Vulkan instance\n");
		}

#if _DEBUG
		InitDebugCallback(Instance);
#endif
	}
	else
	{
		fprintf(stderr, "ERROR: Some extensions required for GLFW are not supported.\n");
	}
	return Instance;
}

static VkInstance InitVulkan()
{
	VkInstance Instance = CreateInstance();
	return Instance;
}

static void MainLoop(GLFWwindow* Window)
{
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}
}

static void CleanUp(GLFWwindow* Window, VkInstance Instance)
{
#if _DEBUG
	DestroyDebugCallback(Instance);
#endif

	vkDestroyInstance(Instance, nullptr);
	glfwDestroyWindow(Window);
	glfwTerminate();
}

int main()
{
	GLFWwindow* Window = InitWindow();
	VkInstance Instance = InitVulkan();
	MainLoop(Window);
	CleanUp(Window, Instance);
}