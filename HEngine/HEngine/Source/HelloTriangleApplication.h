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

	HelloTriangleApplication() { }

	~HelloTriangleApplication() { }

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

private: // data

	GLFWwindow* window = nullptr;
	uint32_t width = 800;
	uint32_t height = 600;

	// Vulkan stuff:
	VkInstance instance;
	VkDebugReportCallbackEXT callback;

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
		VkInstanceCreateInfo createInfo    = { };
		createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo        = &appInfo;
		createInfo.enabledLayerCount       = 0;
		createInfo.enabledExtensionCount   = requiredExtensionCount;
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		// Set up validation layers
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
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
		}
		std::cout << std::endl;

		// Create the instance
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

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
		// Clean up Vulkan:
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
		vkDestroyInstance(instance, nullptr);

		// Clean up GLFW:
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};