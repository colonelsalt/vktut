#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <glm/glm.hpp>

typedef int8_t s8;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
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

struct vertex
{
	glm::vec2 Position;
	glm::vec3 Colour;

	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription Result
		{
			.binding = 0,
			.stride = sizeof(vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
		return Result;
	}

	struct attr_desc
	{
		static constexpr u32 Size = 2;
		VkVertexInputAttributeDescription Data[Size];
	};
	static attr_desc GetAttributeDescriptions()
	{
		attr_desc Result
		{
			.Data
			{
				{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(vertex, Position)
				},
				{
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32_SFLOAT,
					.offset = offsetof(vertex, Colour)
				}
			}
		};
		return Result;
	}
};

static const vertex s_Vertices[]
{
	{.Position = { 0.0f, -0.5f}, .Colour = {1.0f, 0.0f, 0.0f}},
	{.Position = { 0.5f,  0.5f}, .Colour = {0.0f, 1.0f, 0.0f}},
	{.Position = {-0.5f,  0.5f}, .Colour = {0.0f, 0.0f, 1.0f}}
};

struct file_buffer
{
	u8* Contents;
	u32 Size;
};

static file_buffer LoadFile(const char* FileName)
{
	file_buffer Result = {};

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Seems like file '%s' doesn't exist, bro\n", FileName);
		Assert(false);
	}
	else
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			Result.Size = (u32)FileSize.QuadPart;
			Result.Contents = AllocArray(u8, Result.Size);
			DWORD BytesRead;
			if (!ReadFile(FileHandle, Result.Contents, Result.Size, &BytesRead, nullptr) ||
				Result.Size != BytesRead)
			{
				fprintf(stderr, "Failed to read file '%s'\n", FileName);
				free(Result.Contents);
				Result = {};
			}
		}
		else
		{
			Assert(false); // Couldn't get file size??
		}
	}
	return Result;
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

static VkShaderModule CreateShaderModule(VkDevice Device, file_buffer ShaderFile)
{
	VkShaderModuleCreateInfo CreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = ShaderFile.Size,
		.pCode = (u32*)ShaderFile.Contents, // TODO: Align to 4-byte memory necessary here?
	};
	VkShaderModule Result = VK_NULL_HANDLE;
	if (vkCreateShaderModule(Device, &CreateInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
	{
		fprintf(stderr, "Failed to create shader... :(\n");
		Assert(false);
	}
	return Result;
}

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

static b32 CheckDeviceSupportsExtensions(VkPhysicalDevice Device)
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
static queue_family_indices FindQueueFamilies(VkPhysicalDevice Device, VkSurfaceKHR Surface)
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
	VkPhysicalDevice Handle;
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
				Result = { .Handle = Devices[i], .QueueFamilyIndices = QueueFamilyIndices, .SwapChainDeets = SwapChainDeets };
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
		if (!Result.Handle)
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
	if (vkCreateDevice(DeviceDeets.Handle, &DeviceCreateInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
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
	VkFramebuffer* Framebuffers;
	u32 NumImages;
	VkFormat Format;
	VkExtent2D Extents;
};

static void CreateFramebuffers(swap_chain* Swapchain, VkDevice Device, VkRenderPass RenderPass)
{
	// TODO: Is this the cleanest way of reusing memory from previous framebuffers?
	Swapchain->Framebuffers = AllocArray(VkFramebuffer, Swapchain->NumImages);
	for (u32 i = 0; i < Swapchain->NumImages; i++)
	{
		VkImageView* Attachment = Swapchain->ImageViews + i;

		VkFramebufferCreateInfo FramebufferInfo
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = RenderPass,
			.attachmentCount = 1,
			.pAttachments = Attachment,
			.width = Swapchain->Extents.width,
			.height = Swapchain->Extents.height,
			.layers = 1
		};
		if (vkCreateFramebuffer(Device, &FramebufferInfo, nullptr, Swapchain->Framebuffers + i) != VK_SUCCESS) // pAllocator
		{
			fprintf(stderr, "Failed to create framebuffer number %u\n", i);
			Assert(false);
		}
	}
}

static swap_chain CreateSwapChain(physical_device_deets* DeviceDeets,
								  VkDevice LogicalDevice,
								  GLFWwindow* Window,
								  VkSurfaceKHR Surface)
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

static VkRenderPass CreateRenderPass(VkDevice Device, swap_chain* Swapchain)
{
	VkAttachmentDescription ColourAttachment
	{
		.format = Swapchain->Format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};
	VkAttachmentReference ColourAttachmentRef
	{
		.attachment = 0, // NOTE: This is the index of 'layout(location = 0)' in the fragment shader
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	VkSubpassDescription Subpass
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &ColourAttachmentRef,
	};

	// TODO: I don't understand this at all... what's going on here??
	VkSubpassDependency Dependency
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo RenderPassInfo
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &ColourAttachment,
		.subpassCount = 1,
		.pSubpasses = &Subpass,
		.dependencyCount = 1,
		.pDependencies = &Dependency,
	};

	VkRenderPass Result = VK_NULL_HANDLE;
	if (vkCreateRenderPass(Device, &RenderPassInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
	{
		fprintf(stderr, "Couldn't make that render pass, my dude\n");
		Assert(false);
	}
	return Result;
}

struct vulkan_pipeline
{
	VkPipeline Handle;
	VkPipelineLayout Layout;
};

static vulkan_pipeline CreateGraphicsPipeline(VkDevice Device, swap_chain* Swapchain, VkRenderPass RenderPass)
{
	file_buffer VertShaderCode = LoadFile("shaders/vert.spv");
	file_buffer FragShaderCode = LoadFile("shaders/frag.spv");

	VkShaderModule VertShaderModule = CreateShaderModule(Device, VertShaderCode);
	VkShaderModule FragShaderModule = CreateShaderModule(Device, FragShaderCode);
	free(VertShaderCode.Contents);
	free(FragShaderCode.Contents);

	VkPipelineShaderStageCreateInfo VertShaderStageInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = VertShaderModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo FragShaderStageInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = FragShaderModule,
		.pName = "main"
	};
	VkPipelineShaderStageCreateInfo ShaderStages[] = { VertShaderStageInfo, FragShaderStageInfo };

	static constexpr VkDynamicState DYNAMIC_STATES[] =  { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo DynamicState
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ArrayCount(DYNAMIC_STATES),
		.pDynamicStates = DYNAMIC_STATES
	};

	VkVertexInputBindingDescription BindingDesc = vertex::GetBindingDescription();
	vertex::attr_desc AttrDesc = vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo VertextInputInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &BindingDesc,
		.vertexAttributeDescriptionCount = AttrDesc.Size,
		.pVertexAttributeDescriptions = AttrDesc.Data,
	};

	VkPipelineInputAssemblyStateCreateInfo InputAssembly
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	};

	VkViewport Viewport
	{
		.x = 0.0f,
		.y = 0.0f,
		.width = (f32)Swapchain->Extents.width,
		.height = (f32)Swapchain->Extents.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	VkRect2D Scissor
	{
		.offset = { .x = 0, .y = 0 },
		.extent = Swapchain->Extents
	};

	VkPipelineViewportStateCreateInfo ViewportState
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};

	VkPipelineRasterizationStateCreateInfo Rasteriser
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f, // TODO: I'm not drawing any lines, do I need this??
	};

	VkPipelineMultisampleStateCreateInfo Multisampling
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
	};

	// TODO: This sets basic bitch alpha blending - is this actually what we want?
	VkPipelineColorBlendAttachmentState ColourBlendAttachment
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // TODO: Is this right?? We're not taking the dest alpha into account at all??
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo GlobalColourBlend
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.attachmentCount = 1,
		.pAttachments = &ColourBlendAttachment,
	};

	VkPipelineLayoutCreateInfo LayoutCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
	};

	vulkan_pipeline Result = {};
	if (vkCreatePipelineLayout(Device, &LayoutCreateInfo, nullptr, &Result.Layout) == VK_SUCCESS) // pAllocator
	{
		VkGraphicsPipelineCreateInfo PipelineInfo
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = ShaderStages,
			.pVertexInputState = &VertextInputInfo,
			.pInputAssemblyState = &InputAssembly,
			.pViewportState = &ViewportState,
			.pRasterizationState = &Rasteriser,
			.pMultisampleState = &Multisampling,
			.pColorBlendState = &GlobalColourBlend,
			.pDynamicState = &DynamicState,
			.layout = Result.Layout,
			.renderPass = RenderPass,
			.subpass = 0
		};
		if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Result.Handle) != VK_SUCCESS) // pAllocator
		{
			fprintf(stderr, "Failed to create graphics pipeline\n");
			Assert(false);
		}
	}
	else
	{
		fprintf(stderr, "Couldn't create that pipeline layout\n");
		Assert(false);
	}

	vkDestroyShaderModule(Device, VertShaderModule, nullptr); // pAllocator
	vkDestroyShaderModule(Device, FragShaderModule, nullptr); // pAllocator

	return Result;
}

static VkCommandPool CreateCommandPool(VkDevice Device, u32 QueueFamilyIndex)
{
	VkCommandPoolCreateInfo PoolInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = QueueFamilyIndex,
	};
	VkCommandPool Result = VK_NULL_HANDLE;
	if (vkCreateCommandPool(Device, &PoolInfo, nullptr, &Result) != VK_SUCCESS) // pAllocator
	{
		fprintf(stderr, "Failed to create command pool homies\n");
		Assert(false);
	}
	return Result;
}

// TODO: How is this different from the number of Swapchain image views??
static constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

static VkCommandBuffer* CreateCommandBuffers(VkDevice Device, VkCommandPool CommandPool)
{
	VkCommandBufferAllocateInfo AllocInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = CommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT,
	};
	VkCommandBuffer* Result = AllocArray(VkCommandBuffer, MAX_FRAMES_IN_FLIGHT);
	// TODO: Does this just allocate GPU memory (in some opaque way), hence why we're not passing a pAllocator?
	if (vkAllocateCommandBuffers(Device, &AllocInfo, Result) != VK_SUCCESS)
	{
		fprintf(stderr, "Failed to allocate command buffers my dudes\n");
		Assert(false);
	}
	return Result;
}

struct vulkan_stuff
{
	VkInstance Instance;
	VkDevice Device; // the logical device, obvs.
	VkSurfaceKHR Surface;
	VkQueue GraphicsQueue;
	VkRenderPass RenderPass;
	VkCommandPool CommandPool;
	VkCommandBuffer* CommandBuffers; // MAX_FRAMES_IN_FLIGHT of these
	swap_chain Swapchain;
	vulkan_pipeline Pipeline;
	physical_device_deets PhysicalDevice;

	VkSemaphore* ImageAvailableSemaphores;
	VkSemaphore* RenderFinishedSempaphores;
	VkFence* InFlightFences;

	u32 CurrentFrame;
	b32 PendingFramebufferResize;
};

static void OnWindowResized(GLFWwindow* Window, s32 Width, s32 Height)
{
	vulkan_stuff* VulkanStuff = (vulkan_stuff*)glfwGetWindowUserPointer(Window);
	VulkanStuff->PendingFramebufferResize = true;
}

static GLFWwindow* InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* Window = glfwCreateWindow(800, 600, "This is a window, mate", nullptr, nullptr);
	glfwSetFramebufferSizeCallback(Window, OnWindowResized);
	return Window;
}

static void CreateSyncObjects(vulkan_stuff* OutVulkanStuff)
{
	VkSemaphoreCreateInfo SempahoreInfo
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	VkFenceCreateInfo FenceInfo
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	OutVulkanStuff->ImageAvailableSemaphores = AllocArray(VkSemaphore, MAX_FRAMES_IN_FLIGHT);
	OutVulkanStuff->RenderFinishedSempaphores = AllocArray(VkSemaphore, MAX_FRAMES_IN_FLIGHT);
	OutVulkanStuff->InFlightFences = AllocArray(VkFence, MAX_FRAMES_IN_FLIGHT);

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(OutVulkanStuff->Device, &SempahoreInfo, nullptr, OutVulkanStuff->ImageAvailableSemaphores + i) != VK_SUCCESS || // pAllocator
			vkCreateSemaphore(OutVulkanStuff->Device, &SempahoreInfo, nullptr, OutVulkanStuff->RenderFinishedSempaphores + i) != VK_SUCCESS || // pAllocator
			vkCreateFence(OutVulkanStuff->Device, &FenceInfo, nullptr, OutVulkanStuff->InFlightFences + i) != VK_SUCCESS) // pAllocator
		{
			fprintf(stderr, "Failed to create them semaphores and/or fences mate\n");
			Assert(false);
		}
	}
}

static void CleanUpSwapchain(VkDevice Device, swap_chain* Swapchain)
{
	for (u32 i = 0; i < Swapchain->NumImages; i++)
	{
		vkDestroyFramebuffer(Device, Swapchain->Framebuffers[i], nullptr); // pAllocator
	}
	free(Swapchain->Framebuffers);

	for (u32 i = 0; i < Swapchain->NumImages; i++)
	{
		vkDestroyImageView(Device, Swapchain->ImageViews[i], nullptr); // pAllocator
	}
	free(Swapchain->ImageViews);
	free(Swapchain->Images);

	vkDestroySwapchainKHR(Device, Swapchain->Handle, nullptr); // pAllocator
}

// TODO: Getting some ugly artefacts here: Framebuffer actually does not resize until user has 'let go' of the mouse drag;
// while dragging everything looks messed up. May be an issue with GLFW event polling - when I move to doing native Win32
// this should be fixed. Possibly relevant links:
// - Reddit discussion: https://www.reddit.com/r/vulkan/comments/174yy9b/smooth_window_resize_help/
// - vkCube source code: https://github.com/KhronosGroup/Vulkan-Tools/blob/main/cube/cube.c
static void RecreateSwapchain(vulkan_stuff* VulkanStuff, GLFWwindow* Window)
{
	s32 Width, Height;
	glfwGetFramebufferSize(Window, &Width, &Height);
	// TODO: This spinlock is gross - there must surely be a better way
	while (Width == 0 || Height == 0)
	{
		glfwGetFramebufferSize(Window, &Width, &Height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(VulkanStuff->Device);

	// TODO: Slightly piggy - we just need to retrieve the updated framebuffer size here...
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanStuff->PhysicalDevice.Handle,
											  VulkanStuff->Surface,
											  &VulkanStuff->PhysicalDevice.SwapChainDeets.Capabilities);
	
	CleanUpSwapchain(VulkanStuff->Device, &VulkanStuff->Swapchain);

	VulkanStuff->Swapchain = CreateSwapChain(&VulkanStuff->PhysicalDevice, VulkanStuff->Device, Window, VulkanStuff->Surface);

	CreateFramebuffers(&VulkanStuff->Swapchain, VulkanStuff->Device, VulkanStuff->RenderPass);
}

static vulkan_stuff InitVulkan(GLFWwindow* Window)
{
	vulkan_stuff Result = {};

	Result.Instance = CreateInstance();
	Result.Surface = CreateSurface(Result.Instance, Window);
	Result.PhysicalDevice = PickPhysicalDevice(Result.Instance, Result.Surface);
	Result.Device = CreateLogicalDevice(Result.PhysicalDevice);
	vkGetDeviceQueue(Result.Device, Result.PhysicalDevice.QueueFamilyIndices.GraphicsFamily, 0, &Result.GraphicsQueue);
	Result.Swapchain = CreateSwapChain(&Result.PhysicalDevice, Result.Device, Window, Result.Surface);
	Result.RenderPass = CreateRenderPass(Result.Device, &Result.Swapchain);
	Result.Pipeline = CreateGraphicsPipeline(Result.Device, &Result.Swapchain, Result.RenderPass);
	CreateFramebuffers(&Result.Swapchain, Result.Device, Result.RenderPass);
	Result.CommandPool = CreateCommandPool(Result.Device, Result.PhysicalDevice.QueueFamilyIndices.GraphicsFamily);
	Result.CommandBuffers = CreateCommandBuffers(Result.Device, Result.CommandPool);
	CreateSyncObjects(&Result);

	return Result;
}

static void RecordCommandBuffer(vulkan_stuff* VulkanStuff, u32 ImageIndex)
{
	VkCommandBufferBeginInfo BeginInfo
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	VkCommandBuffer CommandBuffer = VulkanStuff->CommandBuffers[VulkanStuff->CurrentFrame];
	if (vkBeginCommandBuffer(CommandBuffer, &BeginInfo) == VK_SUCCESS)
	{
		VkClearValue ClearColour { .color = { 0.0f, 0.0f, 0.0f, 1.0f } };
		Assert(ImageIndex < VulkanStuff->Swapchain.NumImages);
		VkRenderPassBeginInfo RenderPassInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = VulkanStuff->RenderPass,
			.framebuffer = VulkanStuff->Swapchain.Framebuffers[ImageIndex],
			.renderArea = { .offset = {0, 0}, .extent = VulkanStuff->Swapchain.Extents },
			.clearValueCount = 1,
			.pClearValues = &ClearColour
		};
		vkCmdBeginRenderPass(CommandBuffer, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VulkanStuff->Pipeline.Handle);
		
		VkViewport Viewport
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (f32)VulkanStuff->Swapchain.Extents.width,
			.height = (f32)VulkanStuff->Swapchain.Extents.height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};
		vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

		VkRect2D Scissor { .extent = VulkanStuff->Swapchain.Extents };
		vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

		vkCmdDraw(CommandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffer);

		if (vkEndCommandBuffer(CommandBuffer) != VK_SUCCESS)
		{
			fprintf(stderr, "Failed to end/record command buffer\n");
			Assert(false);
		}
	}
	else
	{
		fprintf(stderr, "Failed to begin recording command buffer or something\n");
		Assert(false);
	}
}

static void DrawFrame(vulkan_stuff* VulkanStuff, GLFWwindow* Window)
{
	vkWaitForFences(VulkanStuff->Device, 1, VulkanStuff->InFlightFences + VulkanStuff->CurrentFrame, VK_TRUE, UINT64_MAX);

	u32 ImageIndex;
	// Sooo... the ImageIndex is written to immediately, but the image may in fact not be available to use until the semaphore has signalled..?
	VkResult CallResult = vkAcquireNextImageKHR(VulkanStuff->Device,
												VulkanStuff->Swapchain.Handle,
												UINT64_MAX,
												VulkanStuff->ImageAvailableSemaphores[VulkanStuff->CurrentFrame],
												VK_NULL_HANDLE,
												&ImageIndex);
	if (CallResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain(VulkanStuff, Window);
	}
	else if (CallResult != VK_SUCCESS && CallResult != VK_SUBOPTIMAL_KHR)
	{
		fprintf(stderr, "Failed to acquire image for drawing...\n");
		Assert(false);
	}
	else
	{
		vkResetFences(VulkanStuff->Device, 1, VulkanStuff->InFlightFences + VulkanStuff->CurrentFrame);

		vkResetCommandBuffer(VulkanStuff->CommandBuffers[VulkanStuff->CurrentFrame], 0);
		RecordCommandBuffer(VulkanStuff, ImageIndex);

		VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo SubmitInfo
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = VulkanStuff->ImageAvailableSemaphores + VulkanStuff->CurrentFrame,
			.pWaitDstStageMask = &WaitStage,
			.commandBufferCount = 1,
			.pCommandBuffers = VulkanStuff->CommandBuffers + VulkanStuff->CurrentFrame,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = VulkanStuff->RenderFinishedSempaphores + VulkanStuff->CurrentFrame,
		};

		if (vkQueueSubmit(VulkanStuff->GraphicsQueue, 1, &SubmitInfo, VulkanStuff->InFlightFences[VulkanStuff->CurrentFrame]) == VK_SUCCESS)
		{
			VkPresentInfoKHR PresentInfo
			{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = VulkanStuff->RenderFinishedSempaphores + VulkanStuff->CurrentFrame,
				.swapchainCount = 1,
				.pSwapchains = &VulkanStuff->Swapchain.Handle,
				.pImageIndices = &ImageIndex,
			};
			CallResult = vkQueuePresentKHR(VulkanStuff->GraphicsQueue, &PresentInfo);
			if (CallResult == VK_ERROR_OUT_OF_DATE_KHR || CallResult == VK_SUBOPTIMAL_KHR || VulkanStuff->PendingFramebufferResize)
			{
				VulkanStuff->PendingFramebufferResize = false;
				RecreateSwapchain(VulkanStuff, Window);
			}
			else if (CallResult != VK_SUCCESS)
			{
				fprintf(stderr, "Failed to queue image for presentation (whatever that means)\n");
				Assert(false);
			}

			VulkanStuff->CurrentFrame = (VulkanStuff->CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}
		else
		{
			fprintf(stderr, "Couldn't submit draw command buffer\n");
			Assert(false);
		}
	}

}

static void MainLoop(GLFWwindow* Window, vulkan_stuff* VulkanStuff)
{
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		DrawFrame(VulkanStuff, Window);
	}

	vkDeviceWaitIdle(VulkanStuff->Device);
}

static void CleanUp(GLFWwindow* Window, vulkan_stuff* VulkanStuff)
{
#if _DEBUG
	DestroyDebugCallback(VulkanStuff->Instance);
#endif
	CleanUpSwapchain(VulkanStuff->Device, &VulkanStuff->Swapchain);
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(VulkanStuff->Device, VulkanStuff->ImageAvailableSemaphores[i], nullptr); // pAllocator
		vkDestroySemaphore(VulkanStuff->Device, VulkanStuff->RenderFinishedSempaphores[i], nullptr); // pAllocator
		vkDestroyFence(VulkanStuff->Device, VulkanStuff->InFlightFences[i], nullptr); // pAllocator
	}
	vkDestroyCommandPool(VulkanStuff->Device, VulkanStuff->CommandPool, nullptr); // pAllocator
	vkDestroyPipeline(VulkanStuff->Device, VulkanStuff->Pipeline.Handle, nullptr); // pAllocator
	vkDestroyPipelineLayout(VulkanStuff->Device, VulkanStuff->Pipeline.Layout, nullptr); // pAllocator
	vkDestroyRenderPass(VulkanStuff->Device, VulkanStuff->RenderPass, nullptr); // pAllocator
	vkDestroyDevice(VulkanStuff->Device, nullptr); // pAllocator
	vkDestroySurfaceKHR(VulkanStuff->Instance, VulkanStuff->Surface, nullptr); // pAllocator
	vkDestroyInstance(VulkanStuff->Instance, nullptr); // pAllocator
	glfwDestroyWindow(Window);
	glfwTerminate();
}

int main()
{
	GLFWwindow* Window = InitWindow();
	vulkan_stuff VulkanStuff = InitVulkan(Window);
	glfwSetWindowUserPointer(Window, &VulkanStuff);
	MainLoop(Window, &VulkanStuff);
	CleanUp(Window, &VulkanStuff);
}