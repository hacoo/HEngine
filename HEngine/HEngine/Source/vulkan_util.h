// Helper / utility functions for Vulkan rendering

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>
#include <array>

#include "render_types.h"

namespace VulkanUtil
{
	template<typename Vertex2D>
	VkVertexInputBindingDescription getBindingDescription();

	template<typename Vertex2D>
	std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};