#include <VulkanApplication.h>
#include <set>
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <fstream>

void VulkanApplication::run()
{
	// Create glfw window
	initWindow();

	// Vulkan setup
	initVulkanInstance();
	setupDebugCallback();
	initSurface();
	pickPhysicalDevice();
	initQueuesAndDevice();
	initSwapchain();
	initGraphicsPipeline();
	
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

void VulkanApplication::initVulkanInstance()
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
}

void VulkanApplication::setupDebugCallback()
{
	// Set up debug callback
	if (enableValidationLayers)
	{
		VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = { };
		callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		callbackCreateInfo.flags = debugFlags;
		callbackCreateInfo.pfnCallback = debugCallback;
		VkResult result = CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Vulkan initialization failed - could not register debug callback");
		}
	}	
}

void VulkanApplication::initSurface()
{
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
}

void VulkanApplication::pickPhysicalDevice()
{
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
}

void VulkanApplication::initQueuesAndDevice()
{
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

	// Initialize the device
	if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create Vulkan logical device");
	}

	// Get final queue handles from device
	// Third argument is offset if storing in array
	vkGetDeviceQueue(device, queueIndices.graphics, 0, &graphicsQueue);	 
	vkGetDeviceQueue(device, queueIndices.present, 0, &presentQueue);
}

void VulkanApplication::initSwapchain()
{
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

void VulkanApplication::initImageViews()
{
	swapchainViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainViews.size(); ++i)
	{
		VkImageViewCreateInfo createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i];

		// How this image view is interpreted -- 1D texture, 2D texture, or 3D texture / cube map
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapchainFormat;

		// You can swizzle image bits around if you want
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapchainViews[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VulkanApplication::initGraphicsPipeline()
{
	auto vertShaderCode = readFileBytes("Source/Shaders/vert.spv");
	auto fragShaderCode = readFileBytes("Source/Shaders/frag.spv");

	std::cout << "Loaded shader bytecode" << std::endl;

	vertShaderModule = createShaderModule(vertShaderCode, device);
	fragShaderModule = createShaderModule(fragShaderCode, device);

	// Modules are just bytecode wrappers -- ShaderStageInfo is pipeline context
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = { };
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; // function to invoke -- for multiple in one file
	// Important note -- shaderStageInfo contains pSpecializationInfo, which is for specifying constants
	// It's more efficient if these can be specified when setting up the pipeline	

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = { };
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Describes format of vertex data, attributes passed to vert shader:
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { };
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	// What kind of geometry to draw from vertices, also, should primitive resart be enabled
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = false;

	// viewport -- region of framebuffer where output is rendered
	VkViewport viewport = { };
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapchainExtent.width;
	viewport.height = (float)swapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 0.0f;

	// Scissor - cut out part of the viewport
	VkRect2D scissor = { };
	scissor.offset = { 0,0 };
	scissor.extent = swapchainExtent;

	// Combine viewport, scissor
	VkPipelineViewportStateCreateInfo viewportStateInfo = { };
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	// Set up rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = { };
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	// Depth clamp -- if true, fragments outside near / far plane are clamped instead of culling
	rasterizerInfo.depthClampEnable = VK_FALSE;

	// if true, the rasterizer is turned off completely!
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
	// rasterizerInfo.polygonMode = VK_POLYGON_MODE_LINE;
	// rasterizerInfo.polygonMode = VK_POLYGON_MODE_POINTS;
	rasterizerInfo.lineWidth = 1.0f;

	// Cull back-facing triangles
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

	// Depth bias -- usually used to resolve z-layering issues on coplanar geometry, e.g., shadows
	// I.e., you'd give everything in the shadow buffer a small bias
	rasterizerInfo.depthBiasEnable = VK_FALSE;
	rasterizerInfo.depthBiasConstantFactor = 0.0f;
	rasterizerInfo.depthBiasClamp = 0.0f;
	rasterizerInfo.depthBiasSlopeFactor = 0.0f;

	// Set up multisampling -- turned off for now
	VkPipelineMultisampleStateCreateInfo multisampleInfo = { };
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.sampleShadingEnable = VK_FALSE;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleInfo.minSampleShading = 1.0f;
	multisampleInfo.pSampleMask = nullptr;
	multisampleInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleInfo.alphaToOneEnable = VK_FALSE;

	// Depth and stencil buffers -- not used now, but would be set up here

	// Set up color blending - i.e, what happens if there's already a color in the framebuffer
	// ColorBlendAttachmentState is per framebuffer.
	// Here, we set up alpha blending:
	VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo = { };
	colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentInfo.blendEnable = VK_TRUE;
	colorBlendAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;

	// Reference blend attachment structure for all framebuffers, set up global stuff, constants
	VkPipelineColorBlendStateCreateInfo colorBlendingInfo = { };
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendAttachmentInfo;
	colorBlendingInfo.blendConstants[0] = 0.0f;
  	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	// A few pipeline things CAN be changed dynamically - e.g., viewport size, line width...
	VkDynamicState dynamicStates[] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};
	
	VkPipelineDynamicStateCreateInfo dynamicStateInfo = { };
	dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateInfo.dynamicStateCount = 2;
	dynamicStateInfo.pDynamicStates = dynamicStates;

	// Set up pipeline layout. Can also set up 'push constants', dynamic constants sent to shaders.
	// These aren't used yet.	
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { };
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	


}

VkShaderModule VulkanApplication::createShaderModule(const std::vector<char>& bytecode, 
	VkDevice& device)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytecode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
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
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	// Clean up swapchain first, it may require glfw to still be alive (not sure)
	for (auto& view : swapchainViews)
	{
		vkDestroyImageView(device, view, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);

	// Clean up GLFW:
	if (window != nullptr)
	{
		glfwDestroyWindow(window);
	}
	glfwTerminate();
	
	// Clean up shaders
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);	

	// Clean up device / instance:
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
}

std::vector<char> VulkanApplication::readFileBytes(const std::string& filename)
{
	// Open file starting at the end, in binary
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
	if (!file.is_open())
	{
		throw std::runtime_error("Could not open file");
	}

	// At end, use this to determine file size
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}