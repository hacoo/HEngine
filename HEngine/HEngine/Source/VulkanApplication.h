#pragma once

// Defines vulkan application, 

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

class VulkanApplication
{
public: // methods

	VulkanApplication()
		:
		window(nullptr),
		width(800),
		height(600),
		physicalDevice(VK_NULL_HANDLE)
	{ }

	~VulkanApplication()
	{ }

	// Copy / moves disallowed
	VulkanApplication(const VulkanApplication& other)          = delete;
	VulkanApplication(VulkanApplication&& other)               = delete;
	VulkanApplication& operator=(const VulkanApplication& rhs) = delete;
	VulkanApplication& operator=(VulkanApplication&& rhs)      = delete;

	void run();

public: // classes
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
	GLFWwindow* window;
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

	void initWindow();
	
	// Create Vulkan instance and return a pointer to it. Throws std::runtime_error 
	// on failure.
	void initVulkan();
	
	uint32_t calcSuitabilityScore(const VkPhysicalDevice& device) const;
	
	std::string getDeviceDescriptionString(const VkPhysicalDevice& device) const;
	
	// Helper function, looks up address of vkCreateDebugReportCallbackEXT as this is not automatically loaded
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	// Similar helper for destroy
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	std::vector<const char*> getRequiredExtensions();

	// Check if all requested validation layers are present. If not, returns false.
	bool checkValidationLayerSupport();

	// Callback for validation layers
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);

	void mainLoop();

	void cleanup();
};