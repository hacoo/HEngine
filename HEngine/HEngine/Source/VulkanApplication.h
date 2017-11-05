#pragma once

// Defines vulkan application, 

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <string>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW\glfw3native.h>

#define GLM_FORCE_RADIANS
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>


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
		windowWidth(800),
		windowHeight(600),
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

protected: // classes

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

	// Passes uniform paramters to shaders
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

protected: // data

	GLFWwindow* window;
	uint32_t windowWidth = 800;
	uint32_t windowHeight = 600;

	// Vulkan stuff:
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	QueueFamilyIndices queueIndices;
	VkSurfaceKHR surface;

	// UBO descriptors
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;

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
	VkDescriptorSetLayout descriptorSetLayout;
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


	// Vertex buffers hold actual vertices to draw
	size_t vertexBufferSize;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// Staging buffer for vertex data; can be mapped by CPU.
	// Vertex data is then transfered to the vertex buffer.
	// This is because CPU-mappable memory is often not the fastest for rendering.
	size_t vertexStagingBufferSize;
	VkBuffer vertexStagingBuffer;
	VkDeviceMemory vertexStagingMemory;

	// Holds shader uniform params
	size_t uinformBufferSize;
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	// Vertex index buffer 
	const std::vector<uint32_t> indices = 
	{
		0, 1, 2, 2, 3, 0
	};

	// Index buffer holds indices of vertices used in triangles
	size_t indexBufferSize;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	size_t indexStagingBufferSize;
	VkBuffer indexStagingBuffer;
	VkDeviceMemory indexStagingMemory;

	// Texture stuff:
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

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

	// Set up index buffers
	void initIndexBuffers();

	// Set up command buffers
	void initCommandBuffers();

	// Set up UBO
	void initUniformBuffer();

	// Set up semaphores / other synchro stuff
	void initSynchro();

	// Set up UBO descriptor set
	void initDescriptorSetLayout();

	// Create pool for allocating descriptor sets
	void initDescriptorPool();

	// Create set of descriptors
	void initDescriptorSet();

	// Fill the vertex buffer through staging once created
	void fillVertexBuffer();

	// Fill index buffer through staging once created
	void fillIndexBuffer();

	void createTextureImage();

	// Update uniform buffer based on application state
	void updateUniformBuffer();

	// Choose the best available type of GPU memory
	static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDevice& physicalDevice);

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
	bool copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	// Create a Vulkan buffer and corresponding memory. Returns true on success, false on failure.
	static bool createVkBuffer(VkBuffer& buffer,
		VkDeviceMemory& memory,
		VkDevice& device,
		VkPhysicalDevice& physicalDevice,
		size_t size,
		std::vector<uint32_t>& queueIndices,
		VkSharingMode sharingMode,
		VkBufferUsageFlags usageFlags,
		VkMemoryPropertyFlags memFlags);

	// Create a vulkan Image texture
	static bool createVkImage(
		VkImage& image,
		VkDeviceMemory& memory,
		VkDevice& device,
		VkPhysicalDevice& physicalDevice,
		size_t width,
		size_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usageFlags,
		VkMemoryPropertyFlags memPropFlags
	);

	// Create a command buffer that is executed once
	bool createSingleTimeCommandBuffer(VkCommandPool& commandPool, VkCommandBuffer& outCommandBuffer);

	// Execute a single-time command buffer once commands are recorded.
	// Execution is synchronous and will wait until the specified queue idles	
	bool executeSingleTimeCommandBuffer(
		VkCommandBuffer commandBuffer,
		VkQueue& queue,
		VkCommandPool& commandPool
	);

	bool transitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkPipelineStageFlags srcStage,
		VkPipelineStageFlags dstStage
	);

	bool copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

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