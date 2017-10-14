#pragma once

#include <vulkan\vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

// Requested debug flags
const VkDebugReportFlagsEXT debugFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT
| VK_DEBUG_REPORT_WARNING_BIT_EXT
| VK_DEBUG_REPORT_DEBUG_BIT_EXT;


// Requested validation layers
const std::vector<const char*> validationLayers =
{
"VK_LAYER_LUNARG_standard_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Test application from the Vulkan tutorial
class HelloTriangleApplication
{
public: // data
	
public: // methods

	HelloTriangleApplication()
		:
		physicalDevice(VK_NULL_HANDLE)
	{ }

	~HelloTriangleApplication() 
	{ }

	// Copy / moves disallowed
	HelloTriangleApplication(const HelloTriangleApplication& other)          = delete;
	HelloTriangleApplication(HelloTriangleApplication&& other)               = delete;
	HelloTriangleApplication& operator=(const HelloTriangleApplication& rhs) = delete;
	HelloTriangleApplication& operator=(HelloTriangleApplication&& rhs)      = delete;
	
	void run()
	{
		initWindow();

		initVulkan();

		mainLoop();

		cleanup();
	}

public: // Classes

	// Records the queue family indices for this machine
	struct QueueFamilyIndices
	{
	public:
		QueueFamilyIndices()
			:
			graphics(-1),
			queueFamilyCount(0)
		{ }
		
		// Checks if all required indices have been filled in
		bool isValid()
		{
			if (graphics < 0)
			{
				return false;
			}
			
			return true;
		}

		// Attempt to fill in this struct using device.
		// If any required queues aren't found, returns false.		
		bool initialize(VkPhysicalDevice& device)
		{
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			for (int i = 0; i < queueFamilies.size(); ++i)
			{
				// check graphics
				if (queueFamilies[i].queueCount > 0 &&
					queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphics = i;
				}
			}

			return isValid();
		}

		// Indices of queues used in this application
		int graphics;
		uint32_t queueFamilyCount;
	};

private: // data

	GLFWwindow* window = nullptr;
	uint32_t width = 800;
	uint32_t height = 600;

	// Vulkan stuff:
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilyIndices queueIndices;

	// queues:
	VkQueue graphicsQueue;

private: // methods
	
	void initWindow()
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

	// Create Vulkan instance and return a pointer to it. Throws std::runtime_error 
	// on failure.
	void initVulkan() 
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
			std::cout << getDeviceDescriptionString(device) << std::endl;
			uint32_t score = calcSuitabilityScore(device);
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
		std::cout << getDeviceDescriptionString(physicalDevice) << std::endl;

		// Get queue indices for the selected device
		if (!queueIndices.initialize(physicalDevice))
		{
			throw std::runtime_error("Could not find all required queue families");
		}

		// Create the logical device
		
		// Create graphics queue. We only need 1. Most drivers support very few queues currently.
		VkDeviceQueueCreateInfo queueCreateInfo = { };
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueIndices.graphics;
		queueCreateInfo.queueCount = 1;

		// Required even if only one queue
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Device features we want to use. For now, everything is turned off.
		VkPhysicalDeviceFeatures deviceFeatures = { };
		
		VkDeviceCreateInfo deviceCreateInfo = { };
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		
		// Enable device-specific extensions (none for now)
		deviceCreateInfo.enabledExtensionCount = 0;
		deviceCreateInfo.ppEnabledExtensionNames = nullptr;

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

		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("Could not create Vulkan logical device");
		}

		// Get final queue handles from device
		// Third argument is offset if storing in array
		vkGetDeviceQueue(device, queueIndices.graphics, 0, &graphicsQueue);
	 
	}

	uint32_t calcSuitabilityScore(const VkPhysicalDevice& device) const
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

		uint32_t score = 0;

		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 20000;
		}
	   
		score += deviceProperties.limits.maxImageDimension2D;
		return score;
	}
	
	// Displays 
	std::string getDeviceDescriptionString(const VkPhysicalDevice& device)
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
		sstream << "  Suitability score: " << calcSuitabilityScore(device) << std::endl;

		return sstream.str();
	}
	

	// Helper function, looks up address of vkCreateDebugReportCallbackEXT as this is not automatically loaded
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) 
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

	// Similar helper for destroy
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (func != nullptr) 
		{
			func(instance, callback, pAllocator);
		}
	}

	std::vector<const char*> getRequiredExtensions()
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

	// Check if all requested validation layers are present. If not, returns false.
	bool checkValidationLayerSupport()
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

	// Callback for validation layers
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
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


	void mainLoop()
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
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
};