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

#include "render_types.h"
#include "vulkan_util.h"

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

class VulkanApplication{
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
			transfer(-1),
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
			if (transfer < 0)
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
				if (queueFamilies[i].queueCount > 0)
				{
					// check graphics
					VkQueueFlags graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
					if (graphicsSupport)
					{
						graphics = i;
						break;
					}
				}	
			}

			for (int i = 0; i < queueFamilies.size(); ++i)
			{
				if (queueFamilies[i].queueCount > 0)
				{
					// check present
					VkBool32 presentSupport;
					vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
					
					if (presentSupport)
					{
						present = i;
					}
				}
			}

			// Try to find an unused queue for transfer ops
			for (int i = 0; i < queueFamilies.size(); ++i)
			{
				if (queueFamilies[i].queueCount > 0)
				{
					// check transfer
					// NOTE: any queue that supports graphics or compute also support transfer.
					// However, they are NOT required to set the transfer bit also!!
					VkQueueFlags computeSupport = queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
					VkQueueFlags transferSupport = queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
					VkQueueFlags graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
					if ((graphicsSupport || computeSupport || transferSupport) &&
						graphics != i)
					{
						transfer = i;
						break;
					}
				}
			}

			// If we couldn't find a separate queue for transfer, use the graphics queue
			if (transfer < 0)
			{
				transfer = graphics;
			}

			return isValid();
		}

	public:
		// Indices of queues used in this application
		int graphics;
		int present;
		int transfer;
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

	// for graphics operations
	VkQueue graphicsQueue;
	// for presenting stuff on the screen
	VkQueue presentQueue;
	// for schlepping memory around
	VkQueue transferQueue;

	// Shader modules
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;

	// Pipeline
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	// Vertex buffer
	const std::vector<Vertex2D> vertices = 
	{
{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	// Vertex index buffer
	const std::vector<size_t> indices = 
	{
		0, 1, 2, 2, 3, 0
	};

	// Vertex buffer hold actual vertices to draw
	size_t vertexBufferSize;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// Staging buffer for vertex data; can be mapped by CPU.
	// Vertex data is then transfered to the vertex buffer.
	// This is because CPU-mappable memory is often not the fastest for rendering.
	size_t vertexStagingBufferSize;
	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingMemory;
	
	// Framebuffers
	std::vector <VkFramebuffer> swapchainFramebuffers;

	// Command pool / buffers
	VkCommandPool graphicsCommandPool;
	VkCommandPool transferCommandPool;
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

	// Clean up previously-created swapchain
	void cleanupSwapchain();

	// Create or re-create the swapchain. Will call everything that depends on the swapchain.
	void recreateSwapchain();

	// Set up image views to use in swapchain
	void initImageViews();

	// Create render passes - how color and depth buffers are used
	void createRenderPass();

	// Configure pipeline	
	void initGraphicsPipeline();

	// Set up framebuffers
	void initFramebuffers();

	// Set up command pool
	void initCommandPools();

	// Set up vertex buffers
	void initVertexBuffers();

	// Set up command buffers
	void initCommandBuffers();

	// Set up semaphores / other synchro stuff
	void initSynchro();

	// Fill the vertex buffer once created
	void fillVertexBuffer();

	// Choose the best available type of GPU memory
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

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

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
 
	// Copy GPU memory around
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// Callbacks:

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

	static void onWindowResized(GLFWwindow* window, int width, int height);

	void mainLoop();

	void drawFrame();

	void cleanup();
};