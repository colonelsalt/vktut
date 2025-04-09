#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint32_t b32;
typedef float f32;
typedef double f64;

#define AllocArray(T, N) ((T*)malloc(N * sizeof(T)));
#define ArrayCount(X) (sizeof(X) / sizeof((X)[0]))
#define Assert(X) if (!(X)) __debugbreak()

static u32 Clamp(u32 Value, u32 Min, u32 Max)
{
	u32 Result = Value;
	if (Value < Min)
	{
		Result = Min;
	}
	else if (Value > Max)
	{
		Result = Max;
	}
	return Result;
}

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

static constexpr const char* DEVICE_EXTENSIONS[] =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
		Func(Instance, &CreateInfo, nullptr, &s_DebugHandle); // pAllocator
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
		Func(Instance, s_DebugHandle, nullptr); // pAllocator
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

struct swap_chain_deets
{
	VkSurfaceCapabilitiesKHR Capabilities;
	
	VkSurfaceFormatKHR* Formats;
	u32 NumFormats;

	VkPresentModeKHR* PresentModes;
	u32 NumPresentModes;
};

static swap_chain_deets QuerySwapChainSupport(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	swap_chain_deets Result = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Device, Surface, &Result.Capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &Result.NumFormats, nullptr);
	if (Result.NumFormats != 0)
	{
		Result.Formats = AllocArray(VkSurfaceFormatKHR, Result.NumFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(Device, Surface, &Result.NumFormats, Result.Formats);
		
		vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &Result.NumPresentModes, nullptr);
		if (Result.NumPresentModes == 0)
		{
			free(Result.Formats);
			Result.Formats = nullptr;
		}
		else
		{
			Result.PresentModes = AllocArray(VkPresentModeKHR, Result.NumPresentModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(Device, Surface, &Result.NumPresentModes, Result.PresentModes);
		}
	}
	return Result;
}

b32 CheckDeviceSupportsExtensions(VkPhysicalDevice Device)
{
	u32 NumExtensions = 0;
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &NumExtensions, nullptr);
	VkExtensionProperties* AvailableExtensions = AllocArray(VkExtensionProperties, NumExtensions);
	vkEnumerateDeviceExtensionProperties(Device, nullptr, &NumExtensions, AvailableExtensions);
	
	u32 FoundExtensions = 0;
	for (u32 i = 0; i < ArrayCount(DEVICE_EXTENSIONS); i++)
	{
		for (u32 j = 0; j < NumExtensions; j++)
		{
			if (strcmp(DEVICE_EXTENSIONS[i], AvailableExtensions[j].extensionName) == 0)
			{
				FoundExtensions++;
				break;
			}
		}
	}

	free(AvailableExtensions);
	b32 Result = FoundExtensions == ArrayCount(DEVICE_EXTENSIONS);
	return Result;
}

enum queue_family_flags : u32
{
	QueueFamily_Graphics = 1,
	QueueFamily_Present  = 1 << 1
};

struct queue_family_indices
{
	u32 GraphicsFamily;
	u32 PresentFamily;
	u32 ValidFlags;
};

// TODO: Don't we just want to find the one queue that supports both graphics and presenting, and save some hassle?
queue_family_indices FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface)
{
	queue_family_indices Result = {};

	u32 NumFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(Device, &NumFamilies, nullptr);

	if (NumFamilies == 0)
	{
		fprintf(stderr, "No queue families WTF\n");
	}
	else
	{
		VkQueueFamilyProperties* QueueFamilies = AllocArray(VkQueueFamilyProperties, NumFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(Device, &NumFamilies, QueueFamilies);
		for (u32 i = 0; i < NumFamilies; i++)
		{
			if (QueueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				Result.GraphicsFamily = i;
				Result.ValidFlags |= QueueFamily_Graphics;
			}

			b32 HasPresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(Device, i, Surface, &HasPresentSupport);
			if (HasPresentSupport)
			{
				Result.PresentFamily = i;
				Result.ValidFlags |= QueueFamily_Present;
			}
			if ((Result.ValidFlags & QueueFamily_Present) && (Result.ValidFlags & QueueFamily_Graphics))
			{
				break;
			}
			else
			{
				Result = {};
			}
		}
		free(QueueFamilies);
	}
	return Result;
}

static u32 RateDeviceSuitability(VkPhysicalDevice Device, queue_family_indices QueueFamilyBois, swap_chain_deets* SwapChainDeets)
{
	u32 Score = 0;

	if ((QueueFamilyBois.ValidFlags & QueueFamily_Graphics) && 
		(QueueFamilyBois.ValidFlags & QueueFamily_Present) &&
		CheckDeviceSupportsExtensions(Device) &&
		SwapChainDeets->NumFormats > 0 &&
		SwapChainDeets->NumPresentModes > 0)
	{
		VkPhysicalDeviceProperties DeviceProps;
		vkGetPhysicalDeviceProperties(Device, &DeviceProps);
		VkPhysicalDeviceFeatures DeviceFeatures;
		vkGetPhysicalDeviceFeatures(Device, &DeviceFeatures);
		if (DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			Score += 1'000;
		}
		Score += DeviceProps.limits.maxImageDimension2D;
	}
	return Score;
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(VkSurfaceFormatKHR* Formats, u32 NumFormats)
{
	Assert(NumFormats != 0 && Formats);
	VkSurfaceFormatKHR Result = Formats[0]; // TODO: Pick the 'next best' option instead?
	for (u32 i = 0; i < NumFormats; i++)
	{
		VkSurfaceFormatKHR Format = Formats[i];
		if (Format.format == VK_FORMAT_B8G8R8_SRGB && Format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			Result = Format;
			break;
		}
	}
	return Result;
}

static VkPresentModeKHR ChooseSwapPresentMode(VkPresentModeKHR* PresentModes, u32 NumPresentModes)
{
	Assert(NumPresentModes > 0 && PresentModes);
	VkPresentModeKHR Result = VK_PRESENT_MODE_FIFO_KHR;
	for (u32 i = 0; i < NumPresentModes; i++)
	{
		VkPresentModeKHR PresentMode = PresentModes[i];
		if (PresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			Result = PresentMode;
			break;
		}
	}
	return Result;
}

static VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR* Capabilities, GLFWwindow* Window)
{
	VkExtent2D Result = Capabilities->currentExtent;
	if (Capabilities->currentExtent.width == 0xFF'FF'FF'FF)
	{
		// TODO: Is this shit actually necessary?
		s32 WindowWidth, WindowHeight;
		glfwGetFramebufferSize(Window, &WindowWidth, &WindowHeight);

		Result.width = Clamp((u32)WindowWidth, Capabilities->minImageExtent.width, Capabilities->maxImageExtent.width);
		Result.height = Clamp((u32)WindowHeight, Capabilities->minImageExtent.height, Capabilities->maxImageExtent.height);
	}
	return Result;
}

struct physical_device_deets
{
	VkPhysicalDevice Device;
	queue_family_indices QueueFamilyIndices;
	swap_chain_deets SwapChainDeets;
};

static physical_device_deets PickPhysicalDevice(VkInstance Instance, VkSurfaceKHR Surface)
{
	physical_device_deets Result = {};

	u32 NumDevices = 0;
	vkEnumeratePhysicalDevices(Instance, &NumDevices, nullptr);
	if (NumDevices == 0)
	{
		fprintf(stderr, "Found no GPUs with Vulkan support!\n");
	}
	else
	{
		VkPhysicalDevice* Devices = AllocArray(VkPhysicalDevice, NumDevices);
		vkEnumeratePhysicalDevices(Instance, &NumDevices, Devices);

		u32 HighestScore = 0;
		for (u32 i = 0; i < NumDevices; i++)
		{
			queue_family_indices QueueFamilyIndices = FindQueueFamilies(Devices[i], Surface);
			swap_chain_deets SwapChainDeets = QuerySwapChainSupport(Devices[i], Surface);
			u32 Score = RateDeviceSuitability(Devices[i], QueueFamilyIndices, &SwapChainDeets);
			if (Score > HighestScore)
			{
				Result = { .Device = Devices[i], .QueueFamilyIndices = QueueFamilyIndices, .SwapChainDeets = SwapChainDeets };
				HighestScore = Score;
			}
			else
			{
				if (SwapChainDeets.Formats)
				{
					free(SwapChainDeets.Formats);
				}
				if (SwapChainDeets.PresentModes)
				{
					free(SwapChainDeets.PresentModes);
				}
			}
		}
		if (!Result.Device)
		{
			Assert(false);
			fprintf(stderr, "Failed to find suitable GPU - they're all shit!\n");
		}
		free(Devices);
	}
	return Result;
}


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

		if (vkCreateInstance(&CreateInfo, nullptr, &Instance) != VK_SUCCESS) // pAllocator
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

struct logical_device
{
	VkDevice Device;
	VkQueue PresentQueue;
};

static VkDevice CreateLogicalDevice(physical_device_deets DeviceDeets)
{
	Assert((DeviceDeets.QueueFamilyIndices.ValidFlags & QueueFamily_Graphics) && (DeviceDeets.QueueFamilyIndices.ValidFlags & QueueFamily_Present));
	// TODO: Oink oink
	Assert(DeviceDeets.QueueFamilyIndices.GraphicsFamily == DeviceDeets.QueueFamilyIndices.PresentFamily);

	VkPhysicalDeviceFeatures DeviceFeatures = {}; // Apparently we're going to modify this later...

	f32 QueuePriority = 1.0f;
	VkDeviceQueueCreateInfo QueueCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = DeviceDeets.QueueFamilyIndices.GraphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &QueuePriority
	};

	VkDeviceCreateInfo DeviceCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &QueueCreateInfo,
		.enabledExtensionCount = ArrayCount(DEVICE_EXTENSIONS),
		.ppEnabledExtensionNames = DEVICE_EXTENSIONS,
		.pEnabledFeatures = &DeviceFeatures,
	};
#if _DEBUG
	DeviceCreateInfo.enabledLayerCount = ArrayCount(VALIDATION_LAYERS);
	DeviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
#endif
	VkDevice Result = VK_NULL_HANDLE;
	if (vkCreateDevice(DeviceDeets.Device, &DeviceCreateInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
	{
		Assert(false);
		fprintf(stderr, "Failed to create logical device!\n");
	}
	return Result;
}

static VkSurfaceKHR CreateSurface(VkInstance Instance, GLFWwindow* Window)
{
	VkWin32SurfaceCreateInfoKHR CreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = glfwGetWin32Window(Window),
	};

	VkSurfaceKHR Result = VK_NULL_HANDLE;
	if (vkCreateWin32SurfaceKHR(Instance, &CreateInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
	{
		Assert(false);
		fprintf(stderr, "Couldn't make a Win32 surface, bro\n");
	}
	return Result;
}

struct swap_chain
{
	VkSwapchainKHR Handle;
	VkImage* Images;
	VkImageView* ImageViews;
	u32 NumImages;
	VkFormat Format;
	VkExtent2D Extents;
};

static swap_chain CreateSwapChain(physical_device_deets* DeviceDeets, VkDevice LogicalDevice, GLFWwindow* Window, VkSurfaceKHR Surface)
{
	swap_chain Result = {};

	swap_chain_deets* SwapChainDeets = &DeviceDeets->SwapChainDeets;
	VkSurfaceFormatKHR SurfaceFormat = ChooseSwapSurfaceFormat(SwapChainDeets->Formats, SwapChainDeets->NumFormats);
	VkPresentModeKHR PresentMode = ChooseSwapPresentMode(SwapChainDeets->PresentModes, SwapChainDeets->NumPresentModes);
	Result.Extents = ChooseSwapExtent(&SwapChainDeets->Capabilities, Window);

	u32 ImageCount = SwapChainDeets->Capabilities.minImageCount + 1;
	if (SwapChainDeets->Capabilities.maxImageCount != 0 && ImageCount > SwapChainDeets->Capabilities.maxImageCount)
	{
		ImageCount = SwapChainDeets->Capabilities.maxImageCount;
	}

	// TODO: Oink, oink...
	Assert(DeviceDeets->QueueFamilyIndices.GraphicsFamily == DeviceDeets->QueueFamilyIndices.PresentFamily);
	Assert((DeviceDeets->QueueFamilyIndices.ValidFlags & QueueFamily_Graphics) && (DeviceDeets->QueueFamilyIndices.ValidFlags & QueueFamily_Present));
	VkSwapchainCreateInfoKHR CreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = Surface,
		.minImageCount = ImageCount,
		.imageFormat = SurfaceFormat.format,
		.imageColorSpace = SurfaceFormat.colorSpace,
		.imageExtent = Result.Extents,
		.imageArrayLayers = 1, // Only > 1 if we're doing stereo 3D rendering
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = SwapChainDeets->Capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = PresentMode,
		.clipped = VK_TRUE, // TODO: Any case where we don't want to do any clipping?
		// TODO: Handle window resizing via 'oldSwapchain'
	};
	Result.Format = SurfaceFormat.format;

	if (vkCreateSwapchainKHR(LogicalDevice, &CreateInfo, nullptr, &Result.Handle) == VK_SUCCESS) // pAllocator
	{
		vkGetSwapchainImagesKHR(LogicalDevice, Result.Handle, &Result.NumImages, nullptr);
		Result.Images = AllocArray(VkImage, Result.NumImages);
		vkGetSwapchainImagesKHR(LogicalDevice, Result.Handle, &Result.NumImages, Result.Images);

		Result.ImageViews = AllocArray(VkImageView, Result.NumImages);
		for (u32 i = 0; i < Result.NumImages; i++)
		{
			VkImageViewCreateInfo IvCreateInfo
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = Result.Images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = Result.Format,
				.components = 
				{
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};
			if (vkCreateImageView(LogicalDevice, &IvCreateInfo, nullptr, Result.ImageViews + i) != VK_SUCCESS) // pAllocator
			{
				Assert(false);
				fprintf(stderr, "Failed to create image view number %u...\n", i);
			}
		}
	}
	else
	{
		Assert(false);
		fprintf(stderr, "Failed to create swap chain, my dude\n");
	}
	return Result;
}

struct vulkan_stuff
{
	VkInstance Instance;
	VkDevice Device; // the logical device, obvs.
	VkSurfaceKHR Surface;
	VkQueue GraphicsQueue;
	swap_chain Swapchain;
};

static vulkan_stuff InitVulkan(GLFWwindow* Window)
{
	vulkan_stuff Result = {};

	Result.Instance = CreateInstance();
	Result.Surface = CreateSurface(Result.Instance, Window);
	physical_device_deets PhysicalDevice = PickPhysicalDevice(Result.Instance, Result.Surface);
	Result.Device = CreateLogicalDevice(PhysicalDevice);
	vkGetDeviceQueue(Result.Device, PhysicalDevice.QueueFamilyIndices.GraphicsFamily, 0, &Result.GraphicsQueue);
	Result.Swapchain = CreateSwapChain(&PhysicalDevice, Result.Device, Window, Result.Surface);

	return Result;
}

static void MainLoop(GLFWwindow* Window)
{
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}
}

static void CleanUp(GLFWwindow* Window, vulkan_stuff VulkanStuff)
{
#if _DEBUG
	DestroyDebugCallback(VulkanStuff.Instance);
#endif

	for (u32 i = 0; i < VulkanStuff.Swapchain.NumImages; i++)
	{
		vkDestroyImageView(VulkanStuff.Device, VulkanStuff.Swapchain.ImageViews[i], nullptr); // pAllocator
	}

	vkDestroySwapchainKHR(VulkanStuff.Device, VulkanStuff.Swapchain.Handle, nullptr); // pAllocator
	vkDestroyDevice(VulkanStuff.Device, nullptr); // pAllocator
	vkDestroySurfaceKHR(VulkanStuff.Instance, VulkanStuff.Surface, nullptr); // pAllocator
	vkDestroyInstance(VulkanStuff.Instance, nullptr); // pAllocator
	glfwDestroyWindow(Window);
	glfwTerminate();
}

int main()
{
	GLFWwindow* Window = InitWindow();
	vulkan_stuff VulkanStuff = InitVulkan(Window);
	MainLoop(Window);
	CleanUp(Window, VulkanStuff);
}