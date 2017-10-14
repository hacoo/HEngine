#include <VulkanApplication.h>
#include <set>
#include <sstream>
#include <unordered_map>
#include <algorithm>

void VulkanApplication::run()
{
	initWindow();

	initVulkan();

	mainLoop();

	cleanup();
}

void VulkanApplication::initWindow()
{
	// Start GLFW
	glfwInit();

	// Specify no client API, since we are using Vulkan (otherwise it will try to use OpenGL or something)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Turn off resizing for now
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// First nullptr would specify which monitor. Second is OpenGL-specific.
	window = glfwCreateWindow(width, height, "Hello, Triangle!", nullptr, nullptr);
}

void VulkanApplication::initVulkan()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Could not initialize Vulkan instance -- a requested validation layer was not available");
	}
		
	// VkApplicationInfo: optional, but provides optimization info to driver
	VkApplicationInfo appInfo  = { };
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_0;

	// Get required extensions
	std::vector<const char*> requiredExtensions = getRequiredExtensions();
	uint32_t requiredExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		
	// Instance creation context - specify extensions, validation layers
	VkInstanceCreateInfo instanceCreateInfo    = { };
	instanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo        = &appInfo;
	instanceCreateInfo.enabledLayerCount       = 0;
	instanceCreateInfo.enabledExtensionCount   = requiredExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();

	// Set up validation layers
	if (enableValidationLayers)
	{
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}

	// Print available / requested extensions
	uint32_t availableExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());

	std::cout << "Available extensions: " << std::endl;
	for (const auto& e : availableExtensions)
	{
		std::cout << " " << e.extensionName << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Required extensions: " << std::endl;
	for (size_t i = 0; i < requiredExtensions.size(); ++i)
	{
		std::cout << " " << requiredExtensions[i] << std::endl;
	}	std::cout << std::endl;

	// Create the instance
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
	{
		if (result == VK_ERROR_EXTENSION_NOT_PRESENT)
		{
			throw std::runtime_error("vkCreateInstance failed -- an extension was missing");
		}
		else
		{
			throw std::runtime_error("vkCreateInstance failed -- reason unknown");
		}
	}

	// Set up debug callback
	if (enableValidationLayers)
	{
		VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = { };
		callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		callbackCreateInfo.flags = debugFlags;
		callbackCreateInfo.pfnCallback = debugCallback;
		result = CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan initialization failed - could not register debug callback");
		}
	}

	// Create the window surface. This is platform-dependent.
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { };
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
	
	auto createWin32SurfaceKHR = 
		(PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

	if (!createWin32SurfaceKHR 
		|| createWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface) != VK_SUCCESS) 
	{
		throw std::runtime_error("Vulkan failed to create window surface!");
	}  

	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) 
	{
        throw std::runtime_error("GLFW failed to create window surface!");
    }

	// Pick physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("Could not find a GPU that supports Vulkan");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		
	uint32_t bestScore = 0;
	VkPhysicalDevice& bestDevice = devices[0];
	std::cout << "Available devices: " << std::endl << std::endl;
	for (VkPhysicalDevice& device : devices)
	{
		std::cout << getDeviceDescriptionString(device, surface) << std::endl;
		uint32_t score = calcSuitabilityScore(device, surface);
		if (score > bestScore)
		{
			bestDevice = device;
			bestScore = score;
		}
	}

	if (bestScore == 0)
	{
		throw std::runtime_error("Failed to find suitable GPU");
	}

	physicalDevice = bestDevice;
		
	std::cout << "Selected device: " << std::endl;
	std::cout << getDeviceDescriptionString(physicalDevice, surface) << std::endl;

	// Get queue indices for the selected device
	if (!queueIndices.initialize(physicalDevice, surface))
	{
		throw std::runtime_error("Could not find all required queue families");
	}
	
	// Create all queues:
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { queueIndices.graphics, queueIndices.present };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Create the logical device
	// Device features we want to use. For now, everything is turned off.
	VkPhysicalDeviceFeatures deviceFeatures = { };
		
	VkDeviceCreateInfo deviceCreateInfo = { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Enable device-specific extensions
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// Enable validation layers
	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	} 
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
	}

	// Set up swap chain
	SwapChainSupportInfo swapChainInfo;
	swapChainInfo.initialize(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainInfo.formats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainInfo.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainInfo.capabilities, width, height);

	uint32_t imageCount = swapChainInfo.capabilities.minImageCount + 1;
	if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
	{
		// Add an extra image for tripple buffering
		imageCount += 1;
	}
	
	if (swapChainInfo.capabilities.maxImageCount > 0 &&
		imageCount > swapChainInfo.capabilities.maxImageCount)
	{
		imageCount = swapChainInfo.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = { };
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1; // only higher for stereoscopic 3d
	// Color attachment = render to this swapchain directly
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.oldSwapchain = nullptr;
	
	uint32_t queueFamilyIndices[] = { (uint32_t)queueIndices.graphics, (uint32_t)queueIndices.present };
	if (queueIndices.graphics != queueIndices.present)
	{
		// Images can be use freely across multiple queue families. More expensive.
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		// Images are in one queue at a time and must be transferred explicity before
		// use in another queue family. Better performance.
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 1;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// Transforms to apply before sending to window; in this case, none
	swapchainCreateInfo.preTransform = swapChainInfo.capabilities.currentTransform;

	// Bit to use for blending with other system windows
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	swapchainCreateInfo.presentMode = presentMode;

	// clip unrended pixels - improves performance, bad if you need access to clipped pixels
	swapchainCreateInfo.clipped = VK_TRUE;

	// Initialize the device
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Vulkan logical device");
	}

	// Get final queue handles from device
	// Third argument is offset if storing in array
	vkGetDeviceQueue(device, queueIndices.graphics, 0, &graphicsQueue);	 
	vkGetDeviceQueue(device, queueIndices.present, 0, &presentQueue);

	// Make swap chain
	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain");
	}

	// Set up swapchain images. Number of images may have changed, so query it.
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

	swapchainFormat = surfaceFormat.format;
	swapchainExtent = extent;
}

uint32_t VulkanApplication::calcSuitabilityScore(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// Disqualifiers
	if (deviceFeatures.geometryShader == false)
	{
		return 0;
	}

	if (!checkDeviceExtensionSupport(device))
	{
		return 0;
	}

	// check swap chain support
	SwapChainSupportInfo swapchainSupportInfo;
	swapchainSupportInfo.initialize(device, surface);
	if (swapchainSupportInfo.formats.empty() || swapchainSupportInfo.presentModes.empty())
	{
		return 0;
	}

	uint32_t score = 0;

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 20000;
	}
	   
	score += deviceProperties.limits.maxImageDimension2D;
	return score;
}

std::string VulkanApplication::getDeviceDescriptionString(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	std::stringstream sstream;
	sstream << "Vulkan device description: " << std::endl;
	sstream << "  Name: " << deviceProperties.deviceName << std::endl;				
	sstream << "  isDiscrete: "
		<< (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		? "Yes" : "No")
		<< std::endl;
	sstream << "  Max 2D Image Dimension: " << deviceProperties.limits.maxImageDimension2D 
		<< std::endl;
	sstream << "  Geometry shaders supported: "
		<< (deviceFeatures.geometryShader ? "Yes" : "No")
		<< std::endl;
	sstream << "  Tesselation shaders supported: "
		<< (deviceFeatures.tessellationShader ? "Yes" : "No")
		<< std::endl;
	sstream << "  Suitability score: " << calcSuitabilityScore(device, surface) << std::endl;

	return sstream.str();
}

VkResult VulkanApplication::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanApplication::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

std::vector<const char*> VulkanApplication::getRequiredExtensions()
{
	uint32_t count = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
	
	std::vector<const char*> requiredExtensions;
	for (size_t i = 0; i < count; ++i)
	{
		requiredExtensions.push_back(glfwExtensions[i]);
	}
	
	if (enableValidationLayers)
	{
		requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	
	return  requiredExtensions;
}

bool VulkanApplication::checkValidationLayerSupport() const
{
	uint32_t availableLayerCount;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());


	// print:
	std::cout << "Available validation layers: " << std::endl;
	for (const auto& l : availableLayers)
	{
		std::cout << " " << l.layerName << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Requested layers: " << std::endl;
	for (const auto& l : validationLayers)
	{
		std::cout << " " << l << std::endl;
	}
	std::cout << std::endl;

	// Check that each layer is available:
	std::unordered_map<std::string, bool> availableLayerMap;
	for (const auto& l : availableLayers)
	{
		availableLayerMap.insert(std::make_pair(std::string(l.layerName), true));
	}

	for (const auto& l : validationLayers)
	{
		if (availableLayerMap.find(std::string(l)) == availableLayerMap.end())
		{
			std::cout << "The following layer was not found: " << l << std::endl;
			return false;
		}
	}

	return true;
}

VkSurfaceFormatKHR VulkanApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) 
	{
        return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFormat : availableFormats) 
	{
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanApplication::chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	// Use mailbox if available. Otherwise, use immediate (no vsync mode basically)
	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
{
	// swap extent is resolution of swap chain images -- basically, resolution of window
	
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	// if currentExtent.width is maxed, we need to extents by hand:
	VkExtent2D extent = { width, height };

	extent.width = std::max(capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, extent.width));

	extent.height = std::max(capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, extent.height));

	return extent;
}

bool VulkanApplication::checkDeviceExtensionSupport(const VkPhysicalDevice& device) const
{ 
	uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::cout << "Available device extensions: " << std::endl;
	for (const auto& e : availableExtensions)
	{
		std::cout << " " << e.extensionName << std::endl;
	}
	std::cout << std::endl;

	std::cout << "Required device extensions: " << std::endl;
	for (const auto& e : deviceExtensions)
	{
		std::cout << " " << e << std::endl;
	}
	std::cout << std::endl;

    std::set<std::string> availableExtensionSet(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : deviceExtensions) 
	{
		if (availableExtensionSet.find(extension) == availableExtensionSet.end())
		{
			std::cout << "WARNING - Extension " << extension << " not found " << std::endl;
			return false;
		}
    }

	return true;
}

// Callback for validation layers
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApplication::debugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{		
	std::stringstream sstr;

	bool information = false;
	bool warning = false;
	bool perf_warning = false;
	bool error = false;
	bool debug = false;

	if (flags && VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
	{
		sstr << "I";
		information = true;
	}
	if (flags && VK_DEBUG_REPORT_WARNING_BIT_EXT)
	{
		sstr << "W";
		warning = true;
	}
	if (flags && VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		sstr << "P";
		perf_warning = true;
	}
	if (flags && VK_DEBUG_REPORT_ERROR_BIT_EXT)
	{
		sstr << "E";
		error = true;
	}
	if (flags && VK_DEBUG_REPORT_DEBUG_BIT_EXT)
	{
		sstr << "D";
		debug = true;
	}

	std::cerr << "validation layer - info bits: " << sstr.str() << " -- " << msg << std::endl;
	return VK_FALSE;
}

void VulkanApplication::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}
}

void VulkanApplication::cleanup()
{
	// Clean up swapchain first, it may require glfw to still be alive (not sure)
	vkDestroySwapchainKHR(device, swapchain, nullptr);

	// Clean up GLFW:
	if (window != nullptr)
	{
		glfwDestroyWindow(window);
	}
	glfwTerminate();
	
	// Clean up Vulkan:
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}