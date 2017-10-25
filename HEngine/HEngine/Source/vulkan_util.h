// Helper / utility functions for Vulkan rendering

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <array>

#include "render_types.h"

namespace VulkanUtil
{
	
	template <typename Vertex2D>
	VkVertexInputBindingDescription VulkanUtil::getBindingDescription()
	{
		// We have only one array of vertex data, hence, only one binding
		VkVertexInputBindingDescription bindingDescription = { };
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		
		return bindingDescription;
	}
	
	template <typename Vertex2D>
	std::array<VkVertexInputAttributeDescription, 2> VulkanUtil::getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> descriptions = { };
		
		// First parameter: position, at location 0
		descriptions[0].binding = 0; // which binding to use (see getBindingDescription)
		descriptions[0].location = 0;
		// corresponds to vec2:
		descriptions[0].format = VK_FORMAT_R32G32_SFLOAT; 
		descriptions[0].offset = offsetof(Vertex2D, pos);

		// Second parameter: color, at location 1
		descriptions[1].binding = 0;
		descriptions[1].location = 1;
		descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[1].offset = offsetof(Vertex2D, color);

		return descriptions;
	}
};