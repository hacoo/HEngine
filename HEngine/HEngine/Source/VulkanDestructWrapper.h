#pragma once

// RAII wrapper class for Vulkan objects. Vulkan is a C API, so there are no destructors.
// By wrapping Vulkan objects in instances of this class, we can ensure proper destruction.