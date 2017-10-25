#pragma once

// Defines vulkan application, 

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW\glfw3native.h>

// Requested debug flags
const VkDebugReportFlagsEXT debugFlags = // VK_DEBUG_REPORT_DEBUG_BIT_EXT |
VK_DEBUG_REPORT_WARNING_BIT_EXT |
VK_DEBUG_REPORT_ERROR_BIT_EXT;


// Requested validation layers
const std::vector<const char*> validationLayers =
{
"VK_LAYER_LUNARG_standard_validation"
};

// Required device extensions
const std::vector<const char*> deviceExtensions =
{
VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
			present(-1),
			queueFamilyCount(0)
		{ }
		
		// Checks if all required indices have been filled in
		bool isValid()
		{
			if (graphics < 0)
			{
				return false;
			}
			if (present < 0)
			{
				return false;
			}
			
			return true;
		}

		// Attempt to fill in this struct using device.
		// If any required queues aren't found, returns false.		
		bool initialize(VkPhysicalDevice& device, VkSurfaceKHR& surface)
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

				// check present
				VkBool32 presentSupport;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (queueFamilies[i].queueCount > 0 && presentSupport)
				{
					present = i;
				}
			}

			return isValid();
		}

	public:
		// Indices of queues used in this application
		int graphics;
		int present;
		uint32_t queueFamilyCount;
	};

	struct SwapChainSupportInfo
	{
	public:
		
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector <VkSurfaceFormatKHR> formats;
		std::vector <VkPresentModeKHR> presentModes;

	public:
		bool initialize(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0) 
			{
				formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) 
			{
				presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
			}

			return true;
		}
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
	VkSurfaceKHR surface;

	// swapchain
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainViews;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;

	// queues:
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// Shader modules
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	// Pipeline
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// Framebuffers
	std::vector <VkFramebuffer> swapchainFramebuffers;

	// Command pool / buffers
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// Rendering synchronoization 
	VkSemaphore imageAvailableSem;
	VkSemaphore renderFinishedSem;


private: // methods

	// Setup functions

	// Create glfw window
	void initWindow();
		
	// Check validation layer support and initialize the Vulkan instance
	void initVulkanInstance();

	// Set up the debug callback
	void setupDebugCallback();
			
	// Set up window surface to draw on (platform dependent)
	void initSurface();

	// Decides which physical device to use. Doesn't set up logical device yet.
	void pickPhysicalDevice();

	// Set up logical device and queues. Creates the device.
	void initQueuesAndDevice();

	// set up the swapchain
	void initSwapchain();

	// Set up image views to use in swapchain
	void initImageViews();

	// Create render passes - how color and depth buffers are used
	void createRenderPass();

	// Configure pipeline	
	void initGraphicsPipeline();

	// Set up framebuffers
	void initFramebuffers();

	// Set up command pool
	void initCommandPool();

	// Set up command buffers
	void initCommandBuffers();

	// Set up semaphores / other synchro stuff
	void initSynchro();

	// create shader module from raw bytecode
	VkShaderModule createShaderModule(const std::vector<char>& bytecode, VkDevice& device);

	static std::vector<char> readFileBytes(const std::string& filename);

	uint32_t calcSuitabilityScore(const VkPhysicalDevice& device, const VkSurfaceKHR& surface) const;
	
	std::string getDeviceDescriptionString(const VkPhysicalDevice& device, 
		const VkSurfaceKHR& surface) const;
	
	// Helper function, looks up address of vkCreateDebugReportCallbackEXT as this is not automatically loaded
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);

	// Similar helper for destroy
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	std::vector<const char*> getRequiredExtensions();

	// Check if all requested validation layers are present. If not, returns false.
	bool checkValidationLayerSupport() const;

	bool checkDeviceExtensionSupport(const VkPhysicalDevice& device) const;

	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	// Choose the swap chain presentation mode. Currently using mailbox so we can use tripple buffering.
	static VkPresentModeKHR chooseSwapChainPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);

	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

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

	void drawFrame();

	void cleanup();
};