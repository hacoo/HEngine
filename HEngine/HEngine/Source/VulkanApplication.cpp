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
	std::cout << "Initializing Vulkan..." << std::endl;
	initVulkanInstance();
	setupDebugCallback();
	initSurface();
	pickPhysicalDevice();
	initQueuesAndDevice();
	initSwapchain();
	initImageViews();
	createRenderPass();
	initDescriptorSetLayout();
	initGraphicsPipeline();
	initFramebuffers();
	initCommandPools();
	initVertexBuffers();
	initIndexBuffers();
	initUniformBuffer();
	initDescriptorPool();
	initDescriptorSet();
	initCommandBuffers();
	initSynchro();
	std::cout << std::endl << "Vulkan initialized OK " << std::endl;	

	fillVertexBuffer();
	fillIndexBuffer();

	std::cout << "Graphics queue index: " << queueIndices.graphics << std::endl;
	std::cout << "Present queue index: " << queueIndices.present << std::endl;
	std::cout << "Transfer queue index: " << queueIndices.transfer << std::endl;

	mainLoop();

	cleanup();
}

void VulkanApplication::initWindow()
{
	// Start GLFW
	glfwInit();

	// Specify no client API, since we are using Vulkan (otherwise it will try to use OpenGL or something)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Don't show window until vulkan boots
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	// Turn off resizing for now
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// First nullptr would specify which monitor. Second is OpenGL-specific.
	window = glfwCreateWindow(width, height, "Hello, Triangle!", nullptr, nullptr);

	// Register size-change callback
	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, VulkanApplication::onWindowResized);
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
	std::set<int> uniqueQueueFamilies = { 
queueIndices.graphics,
queueIndices.present,
queueIndices.transfer 
	};

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
	vkGetDeviceQueue(device, queueIndices.transfer, 0, &transferQueue);
}

void VulkanApplication::initSwapchain()
{
	// Set up swap chain
	SwapChainSupportInfo swapChainInfo;
	swapChainInfo.initialize(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainInfo.formats);
	VkPresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainInfo.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainInfo.capabilities);

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
		
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapchainViews[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VulkanApplication::createRenderPass()
{
	// Set up color buffer -- just one, since we aren't multisampling
	VkAttachmentDescription colorAttachment = { };
	colorAttachment.format = swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear to black before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Rendered contents stay in memory

	// Not currently using stencil buffer
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	
	// Indicate that final layout will be used in swapchain
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; 

	VkAttachmentReference colorAttachmentRefInfo = { };
	colorAttachmentRefInfo.attachment = 0;
	colorAttachmentRefInfo.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRefInfo;

	VkSubpassDependency dependency = { };
	// Implicit subpass -- before or after render pass
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// Index of target subpass
	dependency.dstSubpass = 0; 
	// operation to wait on, stage where it occurs:
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	// operations that wait on this:
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = { };
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Coult not create render pass");
	}
}

void VulkanApplication::initGraphicsPipeline()
{
	auto vertShaderCode = readFileBytes("Source/Shaders/vert.spv");
	auto fragShaderCode = readFileBytes("Source/Shaders/frag.spv");

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
	auto vertexBindingDescription = VulkanUtil::getBindingDescription<Vertex2D>();
	auto vertexAttributeDescriptions = VulkanUtil::getAttributeDescriptions<Vertex2D>();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { };
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

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
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = 0;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// Create the actual pipeline:	
	VkGraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisampleInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlendingInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0; // index of subpass for this pipeline

	// Base pipeline handle allows 'inheritance' from a parent pipeline -- switching
	// between similar pipelines, with the same parent, is faster
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	// pipelineInfo.flags |= VK_PIPELINE_CREATE_DERIVATIVE_BIT; // turn on to use inheritance

	if (vkCreateGraphicsPipelines(device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&graphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
}

void VulkanApplication::initFramebuffers()
{
	swapchainFramebuffers.resize(swapchainViews.size());

	for (size_t i = 0; i < swapchainViews.size(); ++i)
	{
		VkImageView attachments[] = { swapchainViews[i] };

		VkFramebufferCreateInfo framebufferInfo = { };
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer");
		}
	}	
}

void VulkanApplication::initCommandPools()
{
	// Create graphics command pool
	VkCommandPoolCreateInfo graphicsPoolInfo = { };
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.queueFamilyIndex = queueIndices.graphics;
	graphicsPoolInfo.flags = 0;
	
	// Possible flags:
	// Flag that this pool is rerecorded with new commands often
	// poolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	// Allow command buffers to be rerecorded individually
	// poolInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	if (vkCreateCommandPool(device, &graphicsPoolInfo, nullptr, &graphicsCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create graphics command pool");
	}

	VkCommandPoolCreateInfo transferPoolInfo = { };
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.queueFamilyIndex = queueIndices.transfer;
	// Flag as transient, since the command buffer is recorded each time we transfer
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	if (vkCreateCommandPool(device, &transferPoolInfo, nullptr, &transferCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not create transfer command pool");
	}
}

void VulkanApplication::initVertexBuffers()
{
	// Allow both graphics and transfer queues to access this buffer.
	// If we used the graphics queue for both ops, you'd use exclusive mode.
	std::vector<uint32_t> vertexQueues = {
static_cast<uint32_t>(queueIndices.graphics),
static_cast<uint32_t>(queueIndices.transfer)
	};
	vertexBufferSize = sizeof(vertices[0]) * vertices.size();

	if (!createVkBuffer(vertexBuffer,
		vertexBufferMemory,
		device,
		physicalDevice,
		vertexBufferSize,
		vertexQueues,
		VK_SHARING_MODE_CONCURRENT,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		throw std::runtime_error("failed to create vertex buffer");
	}

	std::vector<uint32_t> vertexStagingQueues = {
static_cast<uint32_t>(queueIndices.transfer)
	};
	vertexStagingBufferSize = sizeof(vertices[0]) * vertices.size();

	if (!createVkBuffer(vertexStagingBuffer,
		vertexStagingMemory,
		device,
		physicalDevice,
		vertexStagingBufferSize,
		vertexStagingQueues,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		throw std::runtime_error("failed to create vertex staging buffer");
	}
}

void VulkanApplication::initIndexBuffers()
{		
	// Allow both graphics and transfer queues to access this buffer.
	// If we used the graphics queue for both ops, you'd use exclusive mode.
	std::vector<uint32_t> indexQueues = {
static_cast<uint32_t>(queueIndices.graphics),
static_cast<uint32_t>(queueIndices.transfer)
	};
	indexBufferSize = sizeof(indices[0]) * indices.size();

	if (!createVkBuffer(indexBuffer,
		indexBufferMemory,
		device,
		physicalDevice,
		indexBufferSize,
		indexQueues,
		VK_SHARING_MODE_CONCURRENT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		throw std::runtime_error("failed to create index buffer");
	}

	std::vector<uint32_t> indexStagingQueues = {
static_cast<uint32_t>(queueIndices.transfer)
	};
	indexStagingBufferSize = sizeof(indices[0]) * indices.size();

	if (!createVkBuffer(indexStagingBuffer,
		indexStagingMemory,
		device,
		physicalDevice,
		indexStagingBufferSize,
		indexStagingQueues,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		throw std::runtime_error("failed to create index staging buffer");
	}
}

void VulkanApplication::fillIndexBuffer()
{
	void* data;

	// Transfer to staging memory first:
	vkMapMemory(device, indexStagingMemory, 0, 
		static_cast<VkDeviceSize>(indexStagingBufferSize), 0, &data);

	memcpy(data, indices.data(), indexStagingBufferSize);

	// Note: For some type of memory, you would need to call vkFlushMappedMemoryRanges after writing,
	// to ensure the write makes it to the device. This isn't necessery because we specified the 
	// COHERENT bit. 
	//
	// Similarly, if reading memory, you would need to call vkInvalidateMappedMemoryRanges before
	// reading, if the memory is not coherent.	

	vkUnmapMemory(device, indexStagingMemory);

	// Copy into device memory
	copyBuffer(indexStagingBuffer, indexBuffer, indexStagingBufferSize);
}

void VulkanApplication::fillVertexBuffer()
{
	void* data;

	// Transfer to staging memory first:
	vkMapMemory(device, vertexStagingMemory, 0, static_cast<VkDeviceSize>(vertexStagingBufferSize), 0, &data);

	memcpy(data, vertices.data(), vertexStagingBufferSize);

	// Note: For some type of memory, you would need to call vkFlushMappedMemoryRanges after writing,
	// to ensure the write makes it to the device. This isn't necessery because we specified the 
	// COHERENT bit. 
	//
	// Similarly, if reading memory, you would need to call vkInvalidateMappedMemoryRanges before
	// reading, if the memory is not coherent.	

	vkUnmapMemory(device, vertexStagingMemory);

	// Copy into device memory
	copyBuffer(vertexStagingBuffer, vertexBuffer, vertexStagingBufferSize);
}

void VulkanApplication::initCommandBuffers()
{
	commandBuffers.resize(swapchainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Could not allocate command buffers");
	}

	for (size_t i = 0; i < commandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo beginInfo = { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		// Indicates that CB may be used while pending execution - i.e., submitting draw commands
		// while previous frame is still drawing		
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		// Reset the buffer
		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = { };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// VULKAN COMMANDS
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 
			0, 1, &descriptorSet, 0, nullptr);

		// Draw contents of vertex buffer
		// vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		
		// Draw using index buffer
		vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);
		// END VULKAN COMMANDS

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not record command buffer");
		}
	}
}

void VulkanApplication::initSynchro()
{
	// Create semaphores:
	VkSemaphoreCreateInfo semInfo = { };
	semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semInfo, nullptr, &imageAvailableSem) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semInfo, nullptr, &renderFinishedSem) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create semaphores");
	}
}

void VulkanApplication::initUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	std::vector<uint32_t> queues = {
static_cast<uint32_t>(queueIndices.graphics)
	};
	uinformBufferSize = sizeof(UniformBufferObject);

	// Host will write to uniform buffer directly, as we expect it to change vert often
	bool result = createVkBuffer(uniformBuffer,
		uniformBufferMemory,
		device,
		physicalDevice,
		uinformBufferSize,
		queues,
		VK_SHARING_MODE_EXCLUSIVE,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (!result)
	{
		throw std::runtime_error("failed to create uniform buffer");
	}
}

void VulkanApplication::initDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = { };

	// 'binding' corresponds to index used in shader
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = { };
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to created descriptor set layout");
	}
}

void VulkanApplication::initDescriptorPool()
{
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = { };
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor pool");
	}
}

void VulkanApplication::initDescriptorSet()
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	// descriptor set is automatically cleand up with descriptor pool
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to created descriptor set");
	}

	// Configure descriptors contained in set:
	
	VkDescriptorBufferInfo bufferInfo = { };
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);

	VkWriteDescriptorSet descWrite = { };
	descWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descWrite.dstSet = descriptorSet;
	descWrite.dstBinding = 0;
	descWrite.dstArrayElement = 0;
	descWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descWrite.descriptorCount = 1;
	descWrite.pBufferInfo = &bufferInfo;
	descWrite.pImageInfo = nullptr;
	descWrite.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 1, &descWrite, 0, nullptr);
}

void VulkanApplication::recreateSwapchain()
{
	vkDeviceWaitIdle(device);

	cleanupSwapchain();

	// Call everything that depends on swapchain or image size
	initSwapchain();
	initImageViews();
	createRenderPass();
	initGraphicsPipeline();
	initFramebuffers();
	initCommandBuffers();
}

void VulkanApplication::updateUniformBuffer()
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = { };	
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// View is above origin at 45 degree angle
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// 45 degree vertical FOV, aspect ratio as per window size, near plane at 1.0, far at 10.0
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 1.0f, 10.0f);

	// In openGL, y-coordinate of clip is inverted. GLM expects this
	ubo.proj[1][1] *= -1;

	// GLM vectors can be copied directly; their format is compatible with shader inputs
	void* data;
	vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device, uniformBufferMemory);
}

uint32_t VulkanApplication::findMemoryType(uint32_t typeFilter,
	VkMemoryPropertyFlags properties, 
	VkPhysicalDevice& physicalDevice)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if (typeFilter & (1 << i) && 
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	
	throw std::runtime_error("no GPU memory type matching the requested filter was found");
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

VkExtent2D VulkanApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// swap extent is resolution of swap chain images -- basically, resolution of window
	
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	// if currentExtent.width is maxed, we need to set extents by hand:
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	VkExtent2D extent = { (uint32_t) width, (uint32_t) height };

	extent.width = std::max(capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, extent.width));

	extent.height = std::max(capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, extent.height));

	return extent;
}

void VulkanApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// Create a temporary command buffer for the copy op:
	VkCommandBufferAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = { };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = { };
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	// Submit on transfer queue:
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	
	// Wait for the transfer to complete. If there were multiple transfers, it would
	// be a good idea to use a fence and wait on all of them; this allows more 
	// driver optimization	
	vkQueueWaitIdle(graphicsQueue);
}

bool VulkanApplication::createVkBuffer(VkBuffer& buffer,
	VkDeviceMemory& memory,
	VkDevice& device,
	VkPhysicalDevice& physicalDevice,
	size_t size,
	std::vector<uint32_t>& queueIndices,
	VkSharingMode sharingMode,
	VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags memFlags)
{	
	VkBufferCreateInfo bufInfo = { };
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.size = size;
	bufInfo.usage = usageFlags;
	bufInfo.sharingMode = sharingMode;
	bufInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size());
	bufInfo.pQueueFamilyIndices = queueIndices.data();

	if (vkCreateBuffer(device, &bufInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		return false;
	}

	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(device, buffer, &memReqs);

	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, memFlags, physicalDevice);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
	{
		return false;
	}

	vkBindBufferMemory(device, buffer, memory, 0);
	return true;
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

void VulkanApplication::onWindowResized(GLFWwindow* window, int width, int height)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	VulkanApplication* app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->recreateSwapchain();
}

void VulkanApplication::mainLoop()
{
	// Don't show window until the first frame is drawn (otherwise you get a very bright white window)
	updateUniformBuffer();
	drawFrame();
	glfwShowWindow(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Tick();
		updateUniformBuffer();
		drawFrame();
	}

	vkDeviceWaitIdle(device);
}

void VulkanApplication::drawFrame()
{
	// Does the following:
	// 1. Get image from swap chain
	// 2. Execute command buffer with that image attached
	// 3. Return image to swap chain

	uint32_t imageIndex = 0xDEADBEEF;
	uint64_t timeout = std::numeric_limits<uint64_t>::max();

	// Get next image from swapchain, signal imageAvailableSem when done. Records into imageIndex
	VkResult result = vkAcquireNextImageKHR(device,
		swapchain,
		timeout,
		imageAvailableSem,
		VK_NULL_HANDLE,
		&imageIndex);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// Something changed, and the swapchain is no longer compatible -- recreate it
		recreateSwapchain();
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("could not get next swapchain image");
	}

	// Wait for queue to go idle from drawing LAST frame -- this way game tick,
	// etc, can continue while previous frame renders
	result = vkQueueWaitIdle(presentQueue);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// At this point, recreate also if swapchain is suboptimal,
		recreateSwapchain();
	} 
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image");
	}
	
	// Submit draw commands:
	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Semaphores to wait on - command buffer will not execute until these signal
	VkSemaphore waitSems[] = { imageAvailableSem };

	// Stages to wait at. We want to wait before writing colors to the image (but other stuff can get started -- 
	// e.g., vertex shaders, which don't need an image to write on yet. Note that each mask corresponds 
	// to a semaphore at the same index.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSems;
	submitInfo.pWaitDstStageMask = waitStages;
	
	// Now register the command buffer we want to execute. This should correspond
	// to a swapchain index	
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	// Semaphore to signal once the command buffer is done executing
	VkSemaphore signalSemaphores[] = { renderFinishedSem };

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit to graphics queue");
	}

	// Present image to the screen
	VkPresentInfoKHR presentInfo = { };
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	// Wait for image to finish rendering before presenation:
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	// Usually, there is only one swapchain!
	VkSwapchainKHR swapchains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;

	// Individual results for each swapchain - not needed, since there is only one
	presentInfo.pResults = nullptr;
	
	vkQueuePresentKHR(presentQueue, &presentInfo);

}

void VulkanApplication::cleanupSwapchain()
{
	for (auto& fb : swapchainFramebuffers)
	{
		vkDestroyFramebuffer(device, fb, nullptr);
	}

	vkFreeCommandBuffers(device, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	// Clean up shaders
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyRenderPass(device, renderPass, nullptr);

	// Clean up swapchain first, it may require glfw to still be alive (not sure)
	for (auto& view : swapchainViews)
	{
		vkDestroyImageView(device, view, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void VulkanApplication::cleanup()
{
	std::cout << "Beginning Vulkan teardown... " << std::endl;

	cleanupSwapchain();

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	// Clean up buffers
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);

	vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
	vkFreeMemory(device, vertexStagingMemory, nullptr);

	vkDestroyBuffer(device, indexBuffer, nullptr);
	vkFreeMemory(device, indexBufferMemory, nullptr);

	vkDestroyBuffer(device, indexStagingBuffer, nullptr);
	vkFreeMemory(device, indexStagingMemory, nullptr);

	vkDestroyBuffer(device, uniformBuffer, nullptr);
	vkFreeMemory(device, uniformBufferMemory, nullptr);
		
	// Clean up synchro stuff
	vkDestroySemaphore(device, imageAvailableSem, nullptr);
	vkDestroySemaphore(device, renderFinishedSem, nullptr);

	vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
	
	// Clean up device / instance:
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	vkDestroyInstance(instance, nullptr);

	std::cout << "Vulkan cleaned up OK" << std::endl;	

	// Clean up GLFW:
	if (window != nullptr)
	{
		glfwDestroyWindow(window);
	}
	glfwTerminate();
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