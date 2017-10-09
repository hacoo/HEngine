#pragma once

#include <functional>
#include <vulkan\vulkan.h>

// RAII wrapper class for Vulkan objects. Vulkan is a C API, so there are no destructors.
// By wrapping Vulkan objects in instances of this class, we can ensure proper destruction.
template <typename T>
class VulkanDestructWrapper
{

public:

	// Default constructor, deleter does nothing
	VulkanDestructWrapper() 
		:
		VulkanDestructWrapper([](T, VkAllocationCallbacks*) {}) { }

	//	VulkanDestructWrapper(const VulkanDestructWrapper<VkInstance>& instance, std::function<void(VkInstance, T, VkAllocationCallbacks*> deletefn))


private:



};