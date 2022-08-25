#include <vulkan/vulkan.h>

#include "core.h"
#include "bir.h"
#include "res.h"
#include "video_internal.h"
#include "video_vk.h"
#include "window_internal.h"

/* == Vulkan backend implemention ==
 *
 * Requires Vulkan 1.2.
 *
 * Uses the VK_KHR_dynamic_rendering extension. Attachment sync is sub-optimal, that is,
 * it's pretty much forced to be synchronous, much like an OpenGL framebuffer.
 *
 * A buffer of "commands" (Not to be confused with a Vulkan command buffer :D), is used
 * to ensure updates to uniform, index and vertex buffers only get applied to the correct
 * frames, and not to each frame in flight at once, which ~~may~~ will cause visual bugs. */

static void add_memcpy_cmd(struct update_queue* buf, void* target, const void* data, usize size) {
	usize cmd_size = sizeof(struct update_cmd_memcpy) + size;

	usize new_size = cmd_size + buf->count;

	if (new_size > buf->capacity) {
		if (buf->capacity == 0) { buf->capacity = 8; }

		while (buf->capacity < new_size) {
			buf->capacity *= 2;
		}

		buf->bytes = core_realloc(buf->bytes, buf->capacity);
	}

	struct update_cmd_memcpy* cmd = (void*)(buf->bytes + buf->count);
	cmd->cmd.type = update_cmd_memcpy;
	cmd->cmd.size = cmd_size;
	cmd->target = target;
	cmd->size = size;

	memcpy(cmd + 1, data, size);

	buf->count += cmd_size;
}

static const char* device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
};

struct vk_video_context vctx;

struct vk_video_context* get_vk_video_context() {
	return &vctx;
}

struct swapchain_capabilities {
	VkSurfaceCapabilitiesKHR capabilities;
	u32 format_count;       VkSurfaceFormatKHR* formats;
	u32 present_mode_count; VkPresentModeKHR* present_modes;
};

static struct swapchain_capabilities get_swapchain_capabilities(VkPhysicalDevice device) {
	struct swapchain_capabilities r = { 0 };

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, get_window_vk_surface(), &r.capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, get_window_vk_surface(), &r.format_count, null);
	if (r.format_count > 0) {
		r.formats = core_alloc(sizeof(VkSurfaceFormatKHR) * r.format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, get_window_vk_surface(), &r.format_count, r.formats);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, get_window_vk_surface(), &r.present_mode_count, null);
	if (r.present_mode_count > 0) {
		r.present_modes = core_alloc(sizeof(VkPresentModeKHR) * r.present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, get_window_vk_surface(),
				&r.present_mode_count, r.present_modes);
	}

	return r;
}

/* Chooses the first format that uses VK_FORMAT_B8G8R8A8_UNORM. On failure, it just returns the first
 * available format. */
static VkSurfaceFormatKHR choose_swap_surface_format(u32 avail_format_count, VkSurfaceFormatKHR* avail_formats) {
	for (u32 i = 0; i < avail_format_count; i++) {
		if (avail_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
			return avail_formats[i];
		}
	}

	warning("Failed to find a surface that supports VK_FORMAT_B8G8R8A8_UNORM");

	return avail_formats[0];
}

/* This is basically just what kind of VSync to use. */
static VkPresentModeKHR choose_swap_present_mode(u32 avail_present_mode_count, VkPresentModeKHR* avail_present_modes) {
	if (!vctx.enable_vsync) {
		for (u32 i = 0; i < avail_present_mode_count; i++) {
			if (avail_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
				return avail_present_modes[i];
			}
		}

		warning("No support for VK_PRESENT_MODE_IMMEDIATE_KHR; "
			"Vertical sync cannnot be disabled.");

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	for (u32 i = 0; i < avail_present_mode_count; i++) {
		if (avail_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return avail_present_modes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

/* Clamp window size between capabilities.minImageExtent and capabilities.maxImageExtent. */
static VkExtent2D choose_swap_extent(VkSurfaceCapabilitiesKHR* capabilities) {
	v2i size = get_window_size();

	VkExtent2D extent = { (u32)size.x, (u32)size.y };

	if (extent.width > capabilities->maxImageExtent.width) {
		extent.width = capabilities->maxImageExtent.width;
	}
	if (extent.width < capabilities->minImageExtent.width) {
		extent.width = capabilities->minImageExtent.width;
	}
	if (extent.height > capabilities->maxImageExtent.height) {
		extent.height = capabilities->maxImageExtent.height;
	}
	if (extent.height < capabilities->minImageExtent.height) {
		extent.height = capabilities->minImageExtent.height;
	}

	return extent;
}

static VkFormat find_supported_format(VkFormat* candidates, usize candidate_count, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (usize i = 0; i < candidate_count; i++) {
		VkFormat format = candidates[i];

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(vctx.pdevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	error("Failed to find a supported format.");

	return candidates[0];
}

static VkFormat find_depth_format() {
	return find_supported_format((VkFormat[]) {
		VK_FORMAT_D32_SFLOAT
	}, 1, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void deinit_swapchain_capabilities(struct swapchain_capabilities* scc) {
	core_free(scc->formats);
	core_free(scc->present_modes);
}

static struct queue_families get_queue_families(VkPhysicalDevice device) {
	struct queue_families r = { -1, -1 };

	u32 family_count = 0;

	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, null);

	VkQueueFamilyProperties* families = core_alloc(sizeof(VkQueueFamilyProperties) * family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families);

	for (u32 i = 0; i < family_count; i++) {
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			r.graphics_compute = (i32)i;
		}

		VkBool32 supports_presentation = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, get_window_vk_surface(), &supports_presentation);
		if (supports_presentation) {
			r.present = (i32)i;
		}
	}

	core_free(families);

	return r;
}

static bool device_supports_extensions(VkPhysicalDevice device) {
	u32 avail_ext_count;
	vkEnumerateDeviceExtensionProperties(device, null, &avail_ext_count, null);

	VkExtensionProperties* avail_exts = core_alloc(sizeof(VkExtensionProperties) * avail_ext_count);

	vkEnumerateDeviceExtensionProperties(device, null, &avail_ext_count, avail_exts);

	for (u32 i = 0; i < sizeof(device_extensions) / sizeof(*device_extensions); i++) {
		bool found = false;

		for (u32 ii = 0; ii < avail_ext_count; ii++) {
			if (strcmp(device_extensions[i], avail_exts[ii].extensionName) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			error("Failed to find extension: `%s'", device_extensions[i]);
			core_free(avail_exts);
			return false;
		}
	}

	core_free(avail_exts);

	return true;
}

static bool layer_supported(const char* name) {
	u32 avail_count;
	vkEnumerateInstanceLayerProperties(&avail_count, null);

	VkLayerProperties* avail_layers = core_alloc(sizeof(VkLayerProperties) * avail_count);
	vkEnumerateInstanceLayerProperties(&avail_count, avail_layers);

	for (u32 i = 0; i < avail_count; i++) {
		if (strcmp(name, avail_layers[i].layerName) == 0) {
			core_free(avail_layers);
			return true;
		}
	}

	core_free(avail_layers);

	return false;
}


static VkResult create_debug_utils_messenger_ext(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* information,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* messenger) {

	PFN_vkCreateDebugUtilsMessengerEXT f =
		(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	return f != null ? f(instance, information, allocator, messenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT messenger,
	const VkAllocationCallbacks* allocator) {

	PFN_vkDestroyDebugUtilsMessengerEXT f =
		(PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	
	if (f) {
		f(instance, messenger, allocator);
	}
}

static VKAPI_ATTR VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* udata) {

	if (severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				info("%s", data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				warning("%s", data->pMessage);
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				error("%s", data->pMessage);
				break;
		}
	}

	return false;
}

static VkPhysicalDevice first_suitable_device() {
	/* Choose the physical device. */
	u32 device_count = 0;
	vkEnumeratePhysicalDevices(vctx.instance, &device_count, null);

	if (device_count == 0) {
		abort_with("Couldn't find any Vulkan-capable graphics hardware.");
	}

	VkPhysicalDevice* devices = core_alloc(sizeof(VkPhysicalDevice) * device_count);
	vkEnumeratePhysicalDevices(vctx.instance, &device_count, devices);

	for (u32 i = 0; i < device_count; i++) {
		VkPhysicalDevice device = devices[i];

		vctx.qfs = get_queue_families(device);

		VkPhysicalDeviceProperties props;
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceProperties(device, &props);
		vkGetPhysicalDeviceFeatures(device, &features);

		bool swapchain_good = false;
		bool extensions_good = device_supports_extensions(device);
		if (extensions_good) {
			struct swapchain_capabilities scc = get_swapchain_capabilities(device);
			swapchain_good = scc.format_count > 0 && scc.present_mode_count > 0;
			deinit_swapchain_capabilities(&scc);
		}

		if (
			(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ||
			 props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
			 extensions_good && swapchain_good &&
			 vctx.qfs.graphics_compute >= 0 && vctx.qfs.present >= 0) {
			info("Selected device: %s.", props.deviceName);
			vctx.min_uniform_buffer_offset_alignment = props.limits.minUniformBufferOffsetAlignment;
			core_free(devices);
			return device;
		}
	}

	core_free(devices);

	return VK_NULL_HANDLE;
}

static usize pad_ub_size(usize size) {
	if (vctx.min_uniform_buffer_offset_alignment > 0) {
		size = (size + vctx.min_uniform_buffer_offset_alignment - 1) & ~(vctx.min_uniform_buffer_offset_alignment - 1);
	}

	return size;
}

static VkCommandBuffer begin_temp_command_buffer(VkCommandPool pool) {
	VkCommandBuffer buffer;

	vkAllocateCommandBuffers(vctx.device, &(VkCommandBufferAllocateInfo) {	
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = pool,
		.commandBufferCount = 1
	}, &buffer);

	vkBeginCommandBuffer(buffer, &(VkCommandBufferBeginInfo) {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	});

	return buffer;
}

static void end_temp_command_buffer(VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue) {
	vkEndCommandBuffer(buffer);

	vkQueueSubmit(vctx.graphics_compute_queue, 1, &(VkSubmitInfo) {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &buffer
	}, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(vctx.device, pool, 1, &buffer);
}

static void change_image_layout(VkImage image, VkFormat format, VkImageLayout src_layout, VkImageLayout dst_layout, bool is_depth) {
	VkCommandBuffer command_buffer = begin_temp_command_buffer(vctx.command_pool);

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = src_layout,
		.newLayout = dst_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	VkPipelineStageFlags src_stage, dst_stage;

	if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (src_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dst_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dst_stage = src_stage;
	} else if (src_layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		if (is_depth) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		abort_with("Bad layout transition. Use a custom call to vkCmdPipelineBarrier instead.");
	}

	vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, null, 0, null, 1, &barrier);

	end_temp_command_buffer(command_buffer, vctx.command_pool, vctx.graphics_compute_queue);
}

static VkImageView new_image_view(VkImage image, VkFormat format, VkImageAspectFlags flags) {
	VkImageView view = VK_NULL_HANDLE;
	if (vkCreateImageView(vctx.device, &(VkImageViewCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange.aspectMask = flags,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount = 1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1
		}, null, &view) != VK_SUCCESS) {
		abort_with("Failed to create image view.");
	}

	return view;
}

static void new_image(v2i size, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props,
	VkImage* image, VmaAllocation* image_memory, VkImageLayout layout, bool is_depth) {

	if (vmaCreateImage(vctx.allocator, &(VkImageCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.extent = {
				.width = (u32)size.x,
				.height = (u32)size.y,
				.depth = 1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.format = format,
			.tiling = tiling,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.usage = usage,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		}, &(VmaAllocationCreateInfo) {
			.usage = VMA_MEMORY_USAGE_AUTO,
			.requiredFlags = props
		}, image, image_memory, null) != VK_SUCCESS) {
		abort_with("Failed to create image.");
	}

	if (layout != VK_IMAGE_LAYOUT_UNDEFINED) {
		change_image_layout(*image, format, VK_IMAGE_LAYOUT_UNDEFINED, layout, is_depth);
	}
}

static void new_depth_resources(VkImage* image, VkImageView* view, VmaAllocation* memory, v2i size, bool can_sample, bool can_storage) {
	VkFormat depth_format = find_depth_format();

	i32 usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (can_sample) {
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	}

	if (can_storage) {
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	VkImageLayout layout = can_sample ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;

	new_image(size, depth_format, VK_IMAGE_TILING_OPTIMAL, usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory, layout, true);

	*view = new_image_view(*image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

static void init_swapchain(VkSwapchainKHR);
static void deinit_swapchain();
static void recreate();

void video_vk_init(const struct video_config* config) {
	memset(&vctx, 0, sizeof vctx);

	vector(const char*) extensions = null;
	window_get_vk_extensions(&extensions);

	vector(const char*) layers = null;

	bool enable_validation = config->enable_validation;
	vctx.enable_vsync = config->enable_vsync;

	if (enable_validation) {
		if (!layer_supported("VK_LAYER_KHRONOS_validation")) {
			warning("No support for Vulkan validation layers. Make sure the Vulkan SDK is installed properly.");
			enable_validation = false;
			goto no_validation;
		}

		vector_push(extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		vector_push(layers, "VK_LAYER_KHRONOS_validation");
	}

no_validation:

	if (vkCreateInstance(&(VkInstanceCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &(VkApplicationInfo) {
				.apiVersion = VK_API_VERSION_1_2,
				.pApplicationName = "Corrosion Application."
			},
			.enabledExtensionCount = (u32)vector_count(extensions),
			.ppEnabledExtensionNames = extensions,
			.enabledLayerCount = (u32)vector_count(layers),
			.ppEnabledLayerNames = layers
		}, null, &vctx.instance) != VK_SUCCESS) {
		abort_with("Failed to create Vulkan instance.");
	}

	free_vector(layers);
	free_vector(extensions);

	info("Vulkan instance created.");

	window_create_vk_surface(vctx.instance);

	if (enable_validation) {
		if (create_debug_utils_messenger_ext(vctx.instance, &(VkDebugUtilsMessengerCreateInfoEXT) {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debug_callback
		}, null, &vctx.messenger) != VK_SUCCESS) {
			abort_with("Failed to create debug messenger.");
		}
	}

	vctx.pdevice = first_suitable_device();
	if (vctx.pdevice == VK_NULL_HANDLE) {
		error("Vulkan-capable hardware exists, but it does not support the necessary features.");
		abort_with("Failed to find a suitable graphics device.");
	}

	vector(VkDeviceQueueCreateInfo) queue_infos = null;
	vector(i32) unique_queue_families = null;

	vector_push(unique_queue_families, vctx.qfs.graphics_compute);
	if (vctx.qfs.present != vctx.qfs.graphics_compute) {
		vector_push(unique_queue_families, vctx.qfs.present);
	}

	f32 queue_priority = 0.0f;
	for (usize i = 0; i < vector_count(unique_queue_families); i++) {
		VkDeviceQueueCreateInfo queue_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = (u32)unique_queue_families[i],
			.queueCount = 1,
			.pQueuePriorities = &queue_priority,
		};

		vector_push(queue_infos, queue_info);
	}

	if (vkCreateDevice(vctx.pdevice, &(VkDeviceCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pQueueCreateInfos = queue_infos,
			.queueCreateInfoCount = (u32)vector_count(queue_infos),
			.pEnabledFeatures = &(VkPhysicalDeviceFeatures) {
				0
			},
			.enabledExtensionCount = sizeof(device_extensions) / sizeof(*device_extensions),
			.ppEnabledExtensionNames = device_extensions,
			.pNext = &(VkPhysicalDeviceDynamicRenderingFeaturesKHR) {
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
				.dynamicRendering = VK_TRUE
			}
		}, null, &vctx.device) != VK_SUCCESS) {
		abort_with("Failed to create a Vulkan device.");
	}

	vkGetDeviceQueue(vctx.device, (u32)vctx.qfs.graphics_compute, 0, &vctx.graphics_compute_queue);
	vkGetDeviceQueue(vctx.device, (u32)vctx.qfs.present,          0, &vctx.present_queue);

	free_vector(queue_infos);
	free_vector(unique_queue_families);

	/* Load extensions */
	vctx.vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetDeviceProcAddr(vctx.device, "vkCmdBeginRenderingKHR");
	vctx.vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetDeviceProcAddr(vctx.device, "vkCmdEndRenderingKHR");

	/* Create the allocator. */
	vmaCreateAllocator(&(VmaAllocatorCreateInfo) {
		.vulkanApiVersion = VK_API_VERSION_1_2,
		.physicalDevice = vctx.pdevice,
		.device = vctx.device,
		.instance = vctx.instance,
		.pVulkanFunctions = &(VmaVulkanFunctions) {
			.vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = &vkGetDeviceProcAddr
		}
	}, &vctx.allocator);

	init_swapchain(VK_NULL_HANDLE);

	/* Create the command pools. */
	if (vkCreateCommandPool(vctx.device, &(VkCommandPoolCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = (u32)vctx.qfs.graphics_compute
		}, null, &vctx.command_pool) != VK_SUCCESS) {
		abort_with("Failed to create command pool.");
	}

	/* Create the command buffers. */
	if (vkAllocateCommandBuffers(vctx.device, &(VkCommandBufferAllocateInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = vctx.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = max_frames_in_flight
		}, vctx.command_buffers) != VK_SUCCESS) {
		abort_with("Failed to allocate command buffers.");
	}

	/* Create sync objects. */
	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (u32 i = 0; i < max_frames_in_flight; i++) {
		if (
			vkCreateSemaphore(vctx.device, &semaphore_info, null, &vctx.image_avail_semaphores[i])    != VK_SUCCESS ||
			vkCreateSemaphore(vctx.device, &semaphore_info, null, &vctx.render_finish_semaphores[i])  != VK_SUCCESS ||
			vkCreateFence(vctx.device, &fence_info, null, &vctx.in_flight_fences[i])) {
			abort_with("Failed to create synchronisation objects.");
		}
	}

	vctx.default_fb = video.new_framebuffer(framebuffer_flags_default | framebuffer_flags_fit, get_window_size(),
		(struct framebuffer_attachment_desc[]) {
			{
				.type = framebuffer_attachment_depth,
				.format = framebuffer_format_depth,
			},
			{
				.type = framebuffer_attachment_colour,
				.format = framebuffer_format_rgba8i,
				.clear_colour = config->clear_colour
			},
		}, 2);
}

void video_vk_deinit() {
	vkDeviceWaitIdle(vctx.device);

	for (usize i = 0; i < vector_count(vctx.free_queue); i++) {
		struct free_queue_item* item = vctx.free_queue + i;

		switch (item->type) {
			case video_vk_object_texture:
				video_vk_free_texture(item->as.texture);
				break;
			case video_vk_object_pipeline:
				video_vk_free_pipeline(item->as.pipeline);
				break;
			case video_vk_object_framebuffer:
				video_vk_free_framebuffer(item->as.framebuffer);
				break;
			default: break;
		}
	}

	vector_clear(vctx.free_queue);

	free_vector(vctx.free_queue);

	video_vk_free_framebuffer(vctx.default_fb);

	for (u32 i = 0; i < max_frames_in_flight; i++) {
		vkDestroySemaphore(vctx.device, vctx.image_avail_semaphores[i], null);
		vkDestroySemaphore(vctx.device, vctx.render_finish_semaphores[i], null);
		vkDestroyFence(vctx.device, vctx.in_flight_fences[i], null);
		
		if (vctx.update_queues[i].capacity > 0) {
			core_free(vctx.update_queues[i].bytes);
		}
	}

	vkDestroyCommandPool(vctx.device, vctx.command_pool, null);

	deinit_swapchain(true);

	window_destroy_vk_surface(vctx.instance);

	vmaDestroyAllocator(vctx.allocator);

	vkDestroyDevice(vctx.device, null);

	destroy_debug_utils_messenger_ext(vctx.instance, vctx.messenger, null);

	vkDestroyInstance(vctx.instance, null);
}

void video_vk_begin() {
frame_begin:

	vctx.draw_call_count = 0;

	vctx.in_frame = true;

	vkWaitForFences(vctx.device, 1, &vctx.in_flight_fences[vctx.current_frame], VK_TRUE, UINT64_MAX);

	VkResult r;
	if (!vctx.want_recreate) {
		r = vkAcquireNextImageKHR(vctx.device, vctx.swapchain, UINT64_MAX, vctx.image_avail_semaphores[vctx.current_frame],
			VK_NULL_HANDLE, &vctx.image_id);
	}
	
	if (vctx.want_recreate || r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
		vctx.want_recreate = false;
		recreate();
		goto frame_begin;
	} else if (r != VK_SUCCESS) {
		abort_with("Failed to acquire swapchain image.");
	}

	vkResetFences(vctx.device, 1, &vctx.in_flight_fences[vctx.current_frame]);

	vkResetCommandBuffer(vctx.command_buffers[vctx.current_frame], 0);

	if (vkBeginCommandBuffer(vctx.command_buffers[vctx.current_frame], &(VkCommandBufferBeginInfo) {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		}) != VK_SUCCESS) {
		abort_with("Failed to begin the command buffer");
	}
}

void video_vk_end() {
	if (vkEndCommandBuffer(vctx.command_buffers[vctx.current_frame]) != VK_SUCCESS) {
		abort_with("Failed to end the command buffer.");
	}

	/* Update everything in the update queue and flush it. */
	struct update_queue* update_queue = vctx.update_queues + vctx.current_frame;
	struct update_cmd* cmd = (struct update_cmd*)(update_queue->bytes);
	struct update_cmd* end = (struct update_cmd*)(update_queue->bytes + update_queue->count);

	while (cmd != end) {
		switch (cmd->type) {
			case update_cmd_memcpy: {
				struct update_cmd_memcpy* ucmd = (struct update_cmd_memcpy*)cmd;
				memcpy(ucmd->target, ucmd + 1, ucmd->size);
			} break;
		}

		cmd = (struct update_cmd*)(((u8*)cmd) + cmd->size);
	}

	update_queue->count = 0;

	/* Submit the draw command buffer. It waits on the compute
	 * command buffer to complete. */
	if (vkQueueSubmit(vctx.graphics_compute_queue, 1, &(VkSubmitInfo) {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = (VkSemaphore[]) {
				vctx.image_avail_semaphores[vctx.current_frame],
			},
			.pWaitDstStageMask = (VkPipelineStageFlags[]) {
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			},
			.commandBufferCount = 1,
			.pCommandBuffers = (VkCommandBuffer[]) {
				vctx.command_buffers[vctx.current_frame]
			},
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = (VkSemaphore[]) {
				vctx.render_finish_semaphores[vctx.current_frame]
			}
		}, vctx.in_flight_fences[vctx.current_frame]) != VK_SUCCESS) {
		abort_with("Failed to submit draw command buffer.");
	}

	vkQueuePresentKHR(vctx.present_queue, &(VkPresentInfoKHR) {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = (VkSemaphore[]) {
			vctx.render_finish_semaphores[vctx.current_frame]
		},
		.swapchainCount = 1,
		.pSwapchains = (VkSwapchainKHR[]) {
			vctx.swapchain
		},
		.pImageIndices = &vctx.image_id,
	});

	vctx.in_frame = false;

	for (usize i = 0; i < vector_count(vctx.free_queue); i++) {
		struct free_queue_item* item = vctx.free_queue + i;

		switch (item->type) {
			case video_vk_object_texture:
				video_vk_free_texture(item->as.texture);
				break;
			case video_vk_object_pipeline:
				video_vk_free_pipeline(item->as.pipeline);
				break;
			case video_vk_object_framebuffer:
				video_vk_free_framebuffer(item->as.framebuffer);
				break;
			default: break;
		}
	}

	vector_clear(vctx.free_queue);

	vctx.prev_frame = vctx.current_frame;
	vctx.current_frame = (vctx.current_frame + 1) % max_frames_in_flight;
}

void video_vk_want_recreate() {
	vctx.want_recreate = true;
}

static void init_swapchain(VkSwapchainKHR old_swapchain) {
	vkDeviceWaitIdle(vctx.device);

	struct swapchain_capabilities scc = get_swapchain_capabilities(vctx.pdevice);
	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(scc.format_count, scc.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(scc.present_mode_count, scc.present_modes);
	VkExtent2D extent = choose_swap_extent(&scc.capabilities);

	vctx.swapchain_format = surface_format.format;
	vctx.swapchain_extent = extent;


	/* Acquire one more image than the minimum if possible so that
	 * we don't end up waiting for the driver to give us another image. */
	u32 image_count = scc.capabilities.minImageCount;
	if (image_count < scc.capabilities.maxImageCount) {
			image_count++;
	}

	VkSwapchainCreateInfoKHR swap_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = get_window_vk_surface(),
		.minImageCount = image_count,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = scc.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = old_swapchain
	};

	u32 queue_family_indices[2] = { vctx.qfs.graphics_compute, vctx.qfs.present };

	if (vctx.qfs.graphics_compute != vctx.qfs.present) {	
		swap_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swap_info.queueFamilyIndexCount = 2;
		swap_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swap_info.queueFamilyIndexCount = 0;
		swap_info.pQueueFamilyIndices = null;
	}

	if (vkCreateSwapchainKHR(vctx.device, &swap_info, null, &vctx.swapchain) != VK_SUCCESS) {
		abort_with("Failed to create swapchain.");
	}

	deinit_swapchain_capabilities(&scc);

	/* Acquire handles to the swapchain images. */
	vkGetSwapchainImagesKHR(vctx.device, vctx.swapchain, &vctx.swapchain_image_count, null);
	vctx.swapchain_images = core_alloc(sizeof(VkImage) * vctx.swapchain_image_count);
	vkGetSwapchainImagesKHR(vctx.device, vctx.swapchain, &vctx.swapchain_image_count, vctx.swapchain_images);

	vctx.swapchain_image_views = core_alloc(sizeof(VkImage) * vctx.swapchain_image_count);

	/* Create image views. */
	for (u32 i = 0; i < vctx.swapchain_image_count; i++) {
		vctx.swapchain_image_views[i] = new_image_view(vctx.swapchain_images[i], vctx.swapchain_format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

static void deinit_swapchain() {
	for (u32 i = 0; i < vctx.swapchain_image_count; i++) {
		vkDestroyImageView(vctx.device, vctx.swapchain_image_views[i], null);
	}

	vkDestroySwapchainKHR(vctx.device, vctx.swapchain, null);

	core_free(vctx.swapchain_images);
	core_free(vctx.swapchain_image_views);
}

static void recreate() {
	vkDeviceWaitIdle(vctx.device);

	VkImage* old_swapchain_images = vctx.swapchain_images;
	VkImageView* old_swapchain_image_views = vctx.swapchain_image_views;
	VkSwapchainKHR old_swapchain = vctx.swapchain;

	init_swapchain(vctx.swapchain);

	for (u32 i = 0; i < vctx.swapchain_image_count; i++) {
		vkDestroyImageView(vctx.device, old_swapchain_image_views[i], null);
	}

	vkDestroySwapchainKHR(vctx.device, old_swapchain, null);

	core_free(old_swapchain_images);
	core_free(old_swapchain_image_views);

	struct video_vk_framebuffer* framebuffer = vctx.framebuffers.head;
	while (framebuffer) {
		if (framebuffer->flags & framebuffer_flags_fit) {
			video_vk_resize_framebuffer((struct framebuffer*)framebuffer, get_window_size());
		}

		framebuffer = framebuffer->next;
	}

	struct video_vk_pipeline* pipeline = vctx.pipelines.head;
	while (pipeline) {
		video_vk_recreate_pipeline((struct pipeline*)pipeline);

		pipeline = pipeline->next;
	}
}

static VkFormat fb_attachment_format(u32 format) {
	return
		format == framebuffer_format_depth   ? find_depth_format() :
		format == framebuffer_format_rgba8i  ? VK_FORMAT_R8G8B8A8_UNORM :
		format == framebuffer_format_rgba16f ? VK_FORMAT_R16G16B16A16_SFLOAT :
		format == framebuffer_format_rgba32f ? VK_FORMAT_R32G32B32A32_SFLOAT :
		VK_FORMAT_R8G8B8A8_UNORM;
}

static void init_vk_framebuffer(struct video_vk_framebuffer* fb,
	u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {

	fb->size = size;
	memset(&fb->attachment_map, 0, sizeof fb->attachment_map);

	fb->is_headless = ~flags & framebuffer_flags_default;

	if (!fb->is_headless) {
		fb->size.x = vctx.swapchain_extent.width;
		fb->size.y = vctx.swapchain_extent.height;
		size = fb->size;
	}

	fb->use_depth = false;
	fb->colour_count = 0;

	usize depth_index = (usize)-1;
	for (usize i = 0; i < attachment_count; i++) {
		if (depth_index > attachment_count && attachments[i].type == framebuffer_attachment_depth) {
			depth_index = i;
			fb->use_depth = true;
			fb->depth_format = find_depth_format(vctx.device);
		}

		if (attachments[i].type == framebuffer_attachment_colour) {
			fb->colour_count++;
		}
	}

	fb->colour_infos = core_calloc(fb->colour_count, sizeof *fb->colour_infos);
	fb->colour_formats = core_calloc(fb->colour_count, sizeof *fb->colour_formats);

	usize colour_index = 0;
	for (usize i = 0; i < attachment_count; i++) {
		const struct framebuffer_attachment_desc* attachment_desc = &attachments[i];

		if (attachment_desc->type == framebuffer_attachment_colour) {
			usize idx = colour_index++;

			if (fb->is_headless) {
				fb->colours[idx].format = fb_attachment_format(attachment_desc->format);
				fb->colour_formats[idx] = fb_attachment_format(attachment_desc->format);
			} else {
				fb->colours[idx].format = vctx.swapchain_format;
				fb->colour_formats[idx] = vctx.swapchain_format;
			}

			fb->colours[idx].clear_colour = attachment_desc->clear_colour;

			table_set(fb->attachment_map, i, &fb->colours[idx]);
		} else if (attachment_desc->type == framebuffer_attachment_depth) {
			if (fb->is_headless) {
				table_set(fb->attachment_map, i, &fb->depth);
			}
		}
	}

	if (fb->is_headless) {
		/* Only one sampler is needed for all of the attachments. */
		if (vkCreateSampler(vctx.device, &(VkSamplerCreateInfo) {
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.magFilter = VK_FILTER_NEAREST,
				.minFilter = VK_FILTER_NEAREST,
				.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
				.anisotropyEnable = VK_FALSE,
				.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
				.unnormalizedCoordinates = VK_FALSE,
				.compareEnable = VK_FALSE,
				.compareOp = VK_COMPARE_OP_ALWAYS,
				.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
			}, null, &fb->sampler) != VK_SUCCESS) {
			abort_with("Failed to create texture sampler.");
		}

		/* Create images for the colour attachments. */
		for (usize i = 0; i < fb->colour_count; i++) {
			struct video_vk_framebuffer_attachment* attachment = &fb->colours[i];

			u32 usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			if (fb->flags & framebuffer_flags_attachments_are_storage) {
				usage |= VK_IMAGE_USAGE_STORAGE_BIT;
			}

			new_image(fb->size, attachment->format, VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&attachment->texture->image, &attachment->texture->memory,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);
			attachment->texture->view = new_image_view(attachment->texture->image,
				attachment->format, VK_IMAGE_ASPECT_COLOR_BIT);

			attachment->texture->size = fb->size;

			/* Give the texture a reference to the sampler so that the
			 * generic "texture" pipeline resource can be used to sample
			 * the attachment from a shader. */
			attachment->texture->sampler = fb->sampler;
		}
	}

	/* Create the image for the depth attachment if required. */
	if (fb->use_depth) {
		VkFormat fmt = find_depth_format();

		fb->depth.type = framebuffer_attachment_depth;

		fb->depth.texture->size = fb->size;
		
		if (fb->is_headless) {
			fb->depth.texture->sampler = fb->sampler;
		}

		new_depth_resources(&fb->depth.texture->image, &fb->depth.texture->view,
			&fb->depth.texture->memory, fb->size, true, fb->flags & framebuffer_flags_attachments_are_storage);
	}
}

static void deinit_vk_framebuffer(struct video_vk_framebuffer* fb) {
	vkDeviceWaitIdle(vctx.device);

	if (fb->is_headless) {
		for (usize i = 0; i < fb->colour_count; i++) {
			vkDestroyImageView(vctx.device, fb->colours[i].texture->view, null);
			vmaDestroyImage(vctx.allocator, fb->colours[i].texture->image, fb->colours[i].texture->memory);
		}

		vkDestroySampler(vctx.device, fb->sampler, null);
	}

	if (fb->use_depth) {
		vkDestroyImageView(vctx.device, fb->depth.texture->view, null);
		vmaDestroyImage(vctx.allocator, fb->depth.texture->image, fb->depth.texture->memory);
	}

	core_free(fb->colour_infos);
	core_free(fb->colour_formats);

	free_table(fb->attachment_map);
}

struct framebuffer* video_vk_new_framebuffer(u32 flags, v2i size, const struct framebuffer_attachment_desc* attachments, usize attachment_count) {
	struct video_vk_framebuffer* fb = core_calloc(1, sizeof(struct video_vk_framebuffer));

	fb->attachment_descs = core_alloc(attachment_count * sizeof(struct framebuffer_attachment_desc));
	memcpy(fb->attachment_descs, attachments, attachment_count * sizeof(struct framebuffer_attachment_desc));
	fb->attachment_count = attachment_count;

	usize colour_count = 0;
	bool has_depth = false;
	for (usize i = 0; i < attachment_count; i++) {
		if (attachments[i].type == framebuffer_attachment_colour) {
			colour_count++;
		} else if (attachments[i].type == framebuffer_attachment_depth) {
			has_depth = true;
		}
	}

	fb->colours = core_calloc(colour_count, sizeof *fb->colours);

	for (usize i = 0; i < colour_count; i++) {
		fb->colours[i].texture = core_calloc(1, sizeof(struct video_vk_texture));
	}

	if (has_depth) {
		fb->depth.texture = core_calloc(1, sizeof *fb->depth.texture);
	}

	fb->flags = flags;

	init_vk_framebuffer(fb, flags, size, attachments, attachment_count);

	list_push(vctx.framebuffers, fb);

	return (struct framebuffer*)fb;
}

void video_vk_free_framebuffer(struct framebuffer* framebuffer) {
	if (vctx.in_frame) {
		vector_push(vctx.free_queue, ((struct free_queue_item) {
			.type = video_vk_object_framebuffer,
			.as.framebuffer = framebuffer
		}));
		return;
	}

	struct video_vk_framebuffer* fb = (struct video_vk_framebuffer*)framebuffer;

	deinit_vk_framebuffer(fb);

	for (usize i = 0; i < fb->colour_count; i++) {
		core_free(fb->colours[i].texture);
	}

	core_free(fb->colours);

	if (fb->use_depth) {
		core_free(fb->depth.texture);
	}

	list_remove(vctx.framebuffers, fb);

	core_free(fb->attachment_descs);
	core_free(fb);
}

v2i video_vk_get_framebuffer_size(const struct framebuffer* framebuffer) {
	const struct video_vk_framebuffer* fb = (const struct video_vk_framebuffer*)framebuffer;
	
	return fb->size;
}

void video_vk_resize_framebuffer(struct framebuffer* framebuffer, v2i new_size) {
	struct video_vk_framebuffer* fb = (struct video_vk_framebuffer*)framebuffer;

	deinit_vk_framebuffer(fb);
	init_vk_framebuffer(fb, fb->flags, new_size, fb->attachment_descs, fb->attachment_count);
}

void video_vk_begin_framebuffer(struct framebuffer* framebuffer) {
	struct video_vk_framebuffer* fb = (struct video_vk_framebuffer*)framebuffer;

	if (fb->is_headless) {
		for (usize* i = table_first(fb->attachment_map); i; i = table_next(fb->attachment_map, *i)) {
			const struct video_vk_framebuffer_attachment* attachment =
				*(struct video_vk_framebuffer_attachment**)table_get(fb->attachment_map, *i);

			u32 new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			u32 access     = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			u32 stage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			u32 old_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			if (attachment->type == framebuffer_attachment_depth) {
				new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				access     = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				stage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				old_stage  = stage;
			}

			VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.image = attachment->texture->image,
				.newLayout = new_layout,
				.subresourceRange = { get_vk_frambuffer_attachment_aspect_flags(attachment), 0, 1, 0, 1},
				.srcAccessMask = 0,
				.dstAccessMask = access,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
			};

			vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
				old_stage, stage,
				0, 0, null, 0, null, 1, &barrier);

			attachment->texture->state = texture_state_attachment_write;
		}
	} else {
		const struct video_vk_framebuffer_attachment* attachment = fb->colours;

		vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0, 0, null, 0, null, 1, &(VkImageMemoryBarrier) {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.image = vctx.swapchain_images[vctx.image_id],
				.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.subresourceRange = { get_vk_frambuffer_attachment_aspect_flags(attachment), 0, 1, 0, 1},
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
			});

		if (fb->use_depth) {
			vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				0, 0, null, 0, null, 1, &(VkImageMemoryBarrier) {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.image = fb->depth.texture->image,
					.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
					.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
				});
		}
	}

	for (usize i = 0; i < fb->colour_count; i++) {
		VkRenderingAttachmentInfoKHR* info = &fb->colour_infos[i];

		v4f cc = fb->colours[i].clear_colour;

		*info = (VkRenderingAttachmentInfoKHR) {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue.color = { cc.r, cc.g, cc.b, cc.a }
		};

		if (fb->is_headless) {
			info->imageView = fb->colours[i].texture->view;
		} else {
			info->imageView = vctx.swapchain_image_views[vctx.image_id];
		}
	}

	VkRenderingAttachmentInfoKHR depth_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
		.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue.depthStencil = { 1.0f, 0 }
	};

	if (fb->use_depth) {
		depth_info.imageView = fb->depth.texture->view;
	}

	VkRenderingInfoKHR rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = {
				.width  = (u32)fb->size.x,
				.height = (u32)fb->size.y
			}
		},
		.colorAttachmentCount = (u32)fb->colour_count,
		.pColorAttachments = fb->colour_infos,
		.layerCount = 1
	};

	if (fb->use_depth) {
		rendering_info.pDepthAttachment = &depth_info;
	}

	vctx.vkCmdBeginRenderingKHR(vctx.command_buffers[vctx.current_frame], &rendering_info);
	vkCmdSetViewport(vctx.command_buffers[vctx.current_frame], 0, 1, &(VkViewport) {
		.x = 0,
		.y = fb->size.y,
		.width = fb->size.x,
		.height = -fb->size.y
	});

	vkCmdSetScissor(vctx.command_buffers[vctx.current_frame], 0, 1, &(VkRect2D) {
		.offset = { 0, 0 },
		.extent = {
			.width  = (u32)fb->size.x,
			.height = (u32)fb->size.y
		}
	});
}

void video_vk_end_framebuffer(struct framebuffer* framebuffer) {
	struct video_vk_framebuffer* fb = (struct video_vk_framebuffer*)framebuffer;

	vctx.vkCmdEndRenderingKHR(vctx.command_buffers[vctx.current_frame]);

	if (fb->is_headless) {
		for (usize* i = table_first(fb->attachment_map); i; i = table_next(fb->attachment_map, *i)) {
			const struct video_vk_framebuffer_attachment* attachment =
				*(struct video_vk_framebuffer_attachment**)table_get(fb->attachment_map, *i);

			u32 old_layout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			u32 prev_access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			u32 prev_stage  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			if (attachment->type == framebuffer_attachment_depth) {
				old_layout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				prev_access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				prev_stage  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			VkImageMemoryBarrier barrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.oldLayout = old_layout,
				.image = attachment->texture->image,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.subresourceRange = { get_vk_frambuffer_attachment_aspect_flags(attachment), 0, 1, 0, 1},
				.srcAccessMask = prev_access,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
			};

			vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
				prev_stage, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
				0, 0, null, 0, null, 1, &barrier);

			attachment->texture->state = texture_state_shader_graphics_read;
		}
	} else {
		const struct video_vk_framebuffer_attachment* attachment = fb->colours;

		VkImageMemoryBarrier barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.image = vctx.swapchain_images[vctx.image_id],
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.subresourceRange = { get_vk_frambuffer_attachment_aspect_flags(attachment), 0, 1, 0, 1},
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = 0,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		};

		vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, null, 0, null, 1, &barrier);
	}
}

struct framebuffer* video_vk_get_default_fb() {
	return vctx.default_fb;
}

struct texture* video_vk_get_attachment(struct framebuffer* fb_, u32 index) {
	struct video_vk_framebuffer* fb = (struct video_vk_framebuffer*)fb_;

	struct video_vk_framebuffer_attachment** attachment_ptr = table_get(fb->attachment_map, index);
	struct video_vk_framebuffer_attachment* attachment = *attachment_ptr;

	return (struct texture*)attachment->texture;
}

static void attributes_to_vk_attributes(const struct pipeline_attribute_bindings* bindings,
	VkVertexInputBindingDescription* vk_descs, VkVertexInputAttributeDescription* vk_attribs) {

	memset(vk_descs, 0, sizeof(VkVertexInputBindingDescription) * bindings->count);

	usize attrib_offset = 0;

	for (usize i = 0; i < bindings->count; i++) {
		const struct pipeline_attribute_binding* binding = bindings->bindings + i;
		VkVertexInputBindingDescription* vk_binding = vk_descs + i;

		vk_binding->binding = binding->binding;
		vk_binding->stride = (u32)binding->stride;
		vk_binding->inputRate = binding->rate == pipeline_attribute_rate_per_instance ?
			VK_VERTEX_INPUT_RATE_INSTANCE :
			VK_VERTEX_INPUT_RATE_VERTEX;

		const struct pipeline_attributes* attribs = &binding->attributes;
		for (usize j = 0; j < attribs->count; j++) {
			const struct pipeline_attribute* attrib = attribs->attributes + j;
			VkVertexInputAttributeDescription* vk_attrib = vk_attribs + j + attrib_offset;

			memset(vk_attrib, 0, sizeof(VkVertexInputAttributeDescription));

			vk_attrib->binding = binding->binding;
			vk_attrib->location = attrib->location;
			vk_attrib->offset = (u32)attrib->offset;

			switch (attrib->type) {
				case pipeline_attribute_vec2:
					vk_attrib->format = VK_FORMAT_R32G32_SFLOAT;
					break;
				case pipeline_attribute_vec3:
					vk_attrib->format = VK_FORMAT_R32G32B32_SFLOAT;
					break;
				case pipeline_attribute_vec4:
					vk_attrib->format = VK_FORMAT_R32G32B32A32_SFLOAT;
					break;
				default:
					vk_attrib->format = VK_FORMAT_R32_SFLOAT;
					break;
			}
		}

		attrib_offset += attribs->count;
	}
}

static void new_buffer(usize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
	VmaAllocationCreateFlags flags, VkBuffer* buffer, VmaAllocation* memory) {

	if (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) {
		flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}

	if (vmaCreateBuffer(vctx.allocator, &(VkBufferCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = (VkDeviceSize)size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		},
		&(VmaAllocationCreateInfo) {
			.flags = flags,
			.usage = VMA_MEMORY_USAGE_AUTO,
			.requiredFlags = props
		}, buffer, memory, null) != VK_SUCCESS) {
		abort_with("Failed to create buffer.");
	}
}

static void copy_buffer(VkBuffer dst, VkBuffer src, VkDeviceSize size, VkCommandPool pool, VkQueue queue) {
	VkCommandBuffer command_buffer = begin_temp_command_buffer(pool);

	VkBufferCopy copy = {
		.size = size
	};
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy);

	end_temp_command_buffer(command_buffer, pool, queue);
}

static void copy_buffer_to_image(VkBuffer buffer, VkImage image, v2i size, VkCommandPool pool, VkQueue queue) {
	VkCommandBuffer command_buffer = begin_temp_command_buffer(vctx.command_pool);

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&(VkBufferImageCopy) {
			.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.imageSubresource.layerCount = 1,
			.imageExtent = { (u32)size.x, (u32)size.y, 1 }
		});

	end_temp_command_buffer(command_buffer, pool, queue);
}

static VkDescriptorSetLayout* init_pipeline_descriptors(struct video_vk_pipeline* pipeline, const struct pipeline_descriptor_sets* descriptor_sets) {
	VkDescriptorSetLayout* set_layouts = null;

	/* Count descriptors of different types for descriptor pool creation. */
	pipeline->sampler_count = 0;
	pipeline->uniform_count = 0;
	pipeline->storage_count = 0;
	pipeline->image_storage_count = 0;
	for (usize i = 0; i < descriptor_sets->count; i++) {
		const struct pipeline_descriptor_set* set = descriptor_sets->sets + i;

		for (usize ii = 0; ii < set->count; ii++) {
			const struct pipeline_descriptor* desc = set->descriptors + ii;

			switch (desc->resource.type) {
				case pipeline_resource_uniform_buffer:
					pipeline->uniform_count++;
					break;
				case pipeline_resource_texture:
					pipeline->sampler_count++;
					break;
				case pipeline_resource_texture_storage:
					pipeline->image_storage_count++;
					break;
				case pipeline_resource_storage:
					pipeline->storage_count++;
					break;
				default:
					abort_with("Invalid resource pointer type on descriptor.");
					break;
			}
		}
	}

	/* Create the descriptor pool. */
	VkDescriptorPoolSize pool_sizes[4];
	usize pool_size_count = 0;
	if (pipeline->uniform_count > 0) {
		usize idx = pool_size_count++;

		pool_sizes[idx].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		pool_sizes[idx].descriptorCount = max_frames_in_flight * (u32)pipeline->uniform_count;
	}

	if (pipeline->sampler_count > 0) {
		usize idx = pool_size_count++;

		pool_sizes[idx].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		pool_sizes[idx].descriptorCount = max_frames_in_flight * (u32)pipeline->sampler_count;
	}

	if (pipeline->storage_count > 0) {
		usize idx = pool_size_count++;

		pool_sizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		pool_sizes[idx].descriptorCount = max_frames_in_flight * (u32)pipeline->storage_count;
	}

	if (pipeline->image_storage_count > 0) {
		usize idx = pool_size_count++;

		pool_sizes[idx].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		pool_sizes[idx].descriptorCount = max_frames_in_flight * (u32)pipeline->image_storage_count;
	}

	if (vkCreateDescriptorPool(vctx.device, &(VkDescriptorPoolCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.poolSizeCount = (u32)pool_size_count,
			.pPoolSizes = pool_sizes,
			.maxSets = max_frames_in_flight * (u32)descriptor_sets->count
		}, null, &pipeline->descriptor_pool) != VK_SUCCESS) {
		abort_with("Failed to create descriptor pool.");
	}

	pipeline->desc_sets = core_calloc(descriptor_sets->count, sizeof(struct video_vk_impl_descriptor_set));
	pipeline->uniforms = core_calloc(pipeline->uniform_count, sizeof(struct video_vk_impl_uniform_buffer));
	set_layouts = core_calloc(descriptor_sets->count, sizeof(VkDescriptorSetLayout));

	usize uniform_counter = 0;

	for (usize i = 0; i < descriptor_sets->count; i++) {
		const struct pipeline_descriptor_set* set = descriptor_sets->sets + i;
		struct video_vk_impl_descriptor_set* v_set = pipeline->desc_sets + i;

		table_set(pipeline->set_table, hash_string(set->name), v_set);

		VkDescriptorSetLayoutBinding* layout_bindings = core_calloc(set->count, sizeof(VkDescriptorSetLayoutBinding));

		for (usize ii = 0; ii < set->count; ii++) {
			const struct pipeline_descriptor* desc = set->descriptors + ii;
			VkDescriptorSetLayoutBinding* lb = layout_bindings + ii;

			switch (desc->resource.type) {
				case pipeline_resource_uniform_buffer:
					lb->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					break;
				case pipeline_resource_texture:
					lb->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					break;
				case pipeline_resource_texture_storage:
					lb->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					break;
				case pipeline_resource_storage:
					lb->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					break;
				default:
					abort_with("Invalid descriptor resource pointer type.");
					break;
			}

			lb->binding = desc->binding;
			lb->descriptorCount = 1;
			lb->stageFlags = 
				desc->stage == pipeline_stage_compute ? VK_SHADER_STAGE_COMPUTE_BIT : 
				desc->stage == pipeline_stage_vertex  ? VK_SHADER_STAGE_VERTEX_BIT :
				VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		if (vkCreateDescriptorSetLayout(vctx.device, &(VkDescriptorSetLayoutCreateInfo) {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = (u32)set->count,
				.pBindings = layout_bindings
			}, null, &v_set->layout) != VK_SUCCESS) {
			abort_with("Failed to create descriptor set layout.");
		}

		set_layouts[i] = v_set->layout;

		/* Each descriptor set for each frame in flight uses
		 * the same descriptor set layout. */
		VkDescriptorSetLayout layouts[max_frames_in_flight];
		for (usize ii = 0; ii < max_frames_in_flight; ii++) {
			layouts[ii] = v_set->layout;
		}

		if (vkAllocateDescriptorSets(vctx.device, &(VkDescriptorSetAllocateInfo) {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool = pipeline->descriptor_pool,
				.descriptorSetCount = max_frames_in_flight,
				.pSetLayouts = layouts
			}, v_set->sets) != VK_SUCCESS) {
			abort_with("Failed to allocate descriptor sets.");
		}

		usize image_type_count = 2;
		VkDescriptorImageInfo* image_infos = core_calloc(max_frames_in_flight * image_type_count, sizeof(VkDescriptorImageInfo));
		usize image_info_count = 0;

		usize buffer_type_count = 2;

		VkDescriptorBufferInfo* buffer_infos = core_calloc(max_frames_in_flight * buffer_type_count, sizeof(VkDescriptorBufferInfo));
		usize buffer_info_count = 0;

		/* Write the descriptor set and create uniform buffers. */
		for (usize ii = 0, ui = 0; ii < set->count; ii++) {
			VkWriteDescriptorSet desc_writes[max_frames_in_flight] = { 0 };

			const struct pipeline_descriptor* desc = set->descriptors + ii;

			usize uniform_idx = 0;

			if (desc->resource.type == pipeline_resource_uniform_buffer) {
				uniform_idx = uniform_counter++;

				pipeline->uniforms[uniform_idx].size = desc->resource.uniform.size;
			}

			table_set(v_set->uniforms, hash_string(desc->name), pipeline->uniforms + uniform_idx);

			image_info_count = 0;
			buffer_info_count = 0;

			for (usize j = 0; j < max_frames_in_flight; j++) {
				struct VkWriteDescriptorSet* write = desc_writes + j;

				write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write->dstSet = v_set->sets[j];
				write->dstBinding = desc->binding;
				write->dstArrayElement = 0;
				write->descriptorCount = 1;

				switch (desc->resource.type) {
					case pipeline_resource_uniform_buffer: {
						new_buffer(pad_ub_size(desc->resource.uniform.size),
							VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
							VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							pipeline->uniforms[uniform_idx].buffers + j,
							pipeline->uniforms[uniform_idx].memories + j);

						vmaMapMemory(vctx.allocator,
							pipeline->uniforms[uniform_idx].memories[j],
							&pipeline->uniforms[uniform_idx].datas[j]);

						VkDescriptorBufferInfo* buffer_info = buffer_infos + (buffer_info_count++);
						buffer_info->buffer = pipeline->uniforms[uniform_idx].buffers[j];
						buffer_info->offset = 0;
						buffer_info->range = pad_ub_size(pipeline->uniforms[uniform_idx].size);

						write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						write->pBufferInfo = buffer_info;
					} break;
					case pipeline_resource_texture: {
						const struct video_vk_texture* texture = (const struct video_vk_texture*)desc->resource.texture;

						VkDescriptorImageInfo* image_info = image_infos + (image_info_count++);

						image_info->imageView = texture->view;
						image_info->sampler = texture->sampler;
						image_info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

						write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						write->pImageInfo = image_info;
					} break;
					case pipeline_resource_texture_storage: {
						const struct video_vk_texture* texture = (const struct video_vk_texture*)desc->resource.texture;

						VkDescriptorImageInfo* image_info = image_infos + (image_info_count++);

						image_info->imageView = texture->view;
						image_info->sampler = texture->sampler;
						image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

						write->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
						write->pImageInfo = image_info;
					} break;
					case pipeline_resource_storage: {
						const struct video_vk_storage* storage = (const struct video_vk_storage*)desc->resource.storage;

						VkDescriptorBufferInfo* buffer_info = buffer_infos + (buffer_info_count++);

						buffer_info->buffer = storage->buffer;
						buffer_info->offset = 0;
						buffer_info->range = storage->size;

						write->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						write->pBufferInfo = buffer_info;
					} break;
					default: break;
				}
			}

			vkUpdateDescriptorSets(vctx.device, max_frames_in_flight, desc_writes, 0, null);
		}

		core_free(image_infos);
		core_free(buffer_infos);
		core_free(layout_bindings);
	}

	return set_layouts;
}

static void init_pipeline(struct video_vk_pipeline* pipeline, u32 flags, const struct video_vk_shader* shader,
	const struct video_vk_framebuffer* framebuffer,
	const struct pipeline_attribute_bindings* attrib_bindings, const struct pipeline_descriptor_sets* descriptor_sets,
	const struct pipeline_config* config) {
	pipeline->desc_sets = null;
	pipeline->uniforms  = null;

	memset(&pipeline->set_table, 0, sizeof(pipeline->set_table));

	pipeline->flags = flags;

	pipeline->command_buffers = vctx.command_buffers;

	pipeline->descriptor_set_count = descriptor_sets->count;

	VkPipelineShaderStageCreateInfo stages[] = {
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = shader->vertex,
			.pName  = "main"
		},
		{	
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = shader->fragment,
			.pName = "main"
		}
	};

	usize attrib_count = 0;
	for (usize i = 0; i < attrib_bindings->count; i++) {
		attrib_count += attrib_bindings->bindings[i].attributes.count;
	}

	VkVertexInputBindingDescription* vk_bind_descs = core_alloc(sizeof(VkVertexInputBindingDescription) * attrib_bindings->count);
	VkVertexInputAttributeDescription* vk_attribs = core_alloc(sizeof(VkVertexInputAttributeDescription) * attrib_count);
	attributes_to_vk_attributes(attrib_bindings, vk_bind_descs, vk_attribs);

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = (u32)attrib_bindings->count,
		.pVertexBindingDescriptions = vk_bind_descs,
		.vertexAttributeDescriptionCount = (u32)attrib_count,
		.pVertexAttributeDescriptions = vk_attribs
	};

	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	if (flags & pipeline_flags_draw_lines) {
		topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	} else if (flags & pipeline_flags_draw_line_strip) {
		topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	} else if (flags & pipeline_flags_draw_points) {
		topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = (VkViewport[]) {
			{
				.x = 0.0f,
				.y = (f32)framebuffer->size.y,
				.width  = (f32)framebuffer->size.x,
				.height = -(f32)framebuffer->size.y,
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			}
		},
		.scissorCount = 1,
		.pScissors = (VkRect2D[]) {
			{
				.offset = { 0, 0 },
				.extent = { (u32)framebuffer->size.x, (u32)framebuffer->size.y }
			}
		}
	};

	VkPipelineRasterizationStateCreateInfo rasteriser = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = config->line_width,
		.cullMode =
			(flags & pipeline_flags_cull_back_face)  ? VK_CULL_MODE_BACK_BIT  :
			(flags & pipeline_flags_cull_front_face) ? VK_CULL_MODE_FRONT_BIT :
			VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
	};

	if (~flags & pipeline_flags_depth_test) {
		depth_stencil.depthTestEnable = VK_FALSE;
		depth_stencil.depthWriteEnable = VK_FALSE;
	}

	VkPipelineColorBlendAttachmentState* colour_blend_attachments =
		core_alloc(sizeof(VkPipelineColorBlendAttachmentState) * framebuffer->colour_count);
	
	VkPipelineColorBlendAttachmentState colour_blend_attachment = {
		.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE
	};

	if (flags & pipeline_flags_blend) {
		colour_blend_attachment.blendEnable = VK_TRUE;
		colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colour_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colour_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	for (usize i = 0; i < framebuffer->colour_count; i++) {
		colour_blend_attachments[i] = colour_blend_attachment;
	}

	VkPipelineColorBlendStateCreateInfo colour_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = (u32)framebuffer->colour_count,
		.pAttachments = colour_blend_attachments
	};

	VkDescriptorSetLayout* set_layouts = null;
	if (descriptor_sets->count != 0) {
		set_layouts = init_pipeline_descriptors(pipeline, descriptor_sets);
	}

	if (vkCreatePipelineLayout(vctx.device, &(VkPipelineLayoutCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (u32)descriptor_sets->count,
			.pSetLayouts = set_layouts,
		}, null, &pipeline->layout) != VK_SUCCESS) {
		abort_with("Failed to create pipeline layout.");
	}

	VkDynamicState dynamic_states[2];

	u32 dynamic_state_count = 0;
	if (flags & pipeline_flags_dynamic_scissor) {
		dynamic_states[dynamic_state_count++] = VK_DYNAMIC_STATE_SCISSOR;
	}

	VkPipelineDynamicStateCreateInfo dynamic_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = dynamic_state_count,
		.pDynamicStates = dynamic_states
	};

	if (vkCreateGraphicsPipelines(vctx.device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = stages,
			.pVertexInputState = &vertex_input_info,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport_state,
			.pRasterizationState = &rasteriser,
			.pMultisampleState = &multisampling,
			.pColorBlendState = &colour_blending,
			.pDynamicState = &dynamic_state,
			.pDepthStencilState = &depth_stencil,
			.layout = pipeline->layout,
			.pNext = &(VkPipelineRenderingCreateInfoKHR) {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
				.colorAttachmentCount = framebuffer->colour_count,
				.pColorAttachmentFormats = framebuffer->colour_formats,
				.depthAttachmentFormat = framebuffer->depth_format
			}
		}, null, &pipeline->pipeline) != VK_SUCCESS) {
		abort_with("Failed to create pipeline.");
	}

	core_free(colour_blend_attachments);
	core_free(vk_attribs);
	core_free(vk_bind_descs);

	if (descriptor_sets->count > 0) {
		core_free(set_layouts);
	}
}

static void init_compute_pipeline(struct video_vk_pipeline* pipeline, u32 flags, const struct video_vk_shader* shader,
	struct pipeline_descriptor_sets* descriptor_sets) {

	pipeline->desc_sets = null;
	pipeline->uniforms  = null;

	memset(&pipeline->set_table, 0, sizeof(pipeline->set_table));

	pipeline->flags = flags;

	pipeline->command_buffers = vctx.command_buffers;

	pipeline->descriptor_set_count = descriptor_sets->count;

	VkPipelineShaderStageCreateInfo stage = {
		.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage  = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shader->compute,
		.pName = "main"
	};

	VkDescriptorSetLayout* set_layouts = null;
	if (descriptor_sets->count != 0) {
		set_layouts = init_pipeline_descriptors(pipeline, descriptor_sets);
	}

	if (vkCreatePipelineLayout(vctx.device, &(VkPipelineLayoutCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = (u32)descriptor_sets->count,
			.pSetLayouts = set_layouts,
		}, null, &pipeline->layout) != VK_SUCCESS) {
		abort_with("Failed to create pipeline layout.");
	}

	if (vkCreateComputePipelines(vctx.device, VK_NULL_HANDLE, 1, &(VkComputePipelineCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = stage,
			.layout = pipeline->layout,
		}, null, &pipeline->pipeline) != VK_SUCCESS) {
		abort_with("Failed to create compute pipeline.");
	}

	core_free(set_layouts);
}

static void deinit_pipeline(struct video_vk_pipeline* pipeline) {
	vkDeviceWaitIdle(vctx.device);

	if (pipeline->desc_sets) {
		for (usize i = 0; i < pipeline->descriptor_set_count; i++) {
			vkDestroyDescriptorSetLayout(vctx.device, pipeline->desc_sets[i].layout, null);

			free_table(pipeline->desc_sets[i].uniforms);
		}

		core_free(pipeline->desc_sets);
	}

	if (pipeline->uniforms) {
		for (usize i = 0; i < pipeline->uniform_count; i++) {
			for (usize j = 0; j < max_frames_in_flight; j++) {
				vmaUnmapMemory(vctx.allocator, pipeline->uniforms[i].memories[j]);
				vmaDestroyBuffer(vctx.allocator,
					pipeline->uniforms[i].buffers[j],
					pipeline->uniforms[i].memories[j]);
			}
		}

		core_free(pipeline->uniforms);
	}

	if (pipeline->descriptor_set_count > 0) {
		vkDestroyDescriptorPool(vctx.device, pipeline->descriptor_pool, null);
	}

	free_table(pipeline->set_table);

	vkDestroyPipelineLayout(vctx.device, pipeline->layout, null);
	vkDestroyPipeline(vctx.device, pipeline->pipeline, null);
}

static void copy_pipeline_descriptor(struct pipeline_descriptor* dst, const struct pipeline_descriptor* src) {
	dst->name     = copy_string(src->name);
	dst->binding  = src->binding;
	dst->stage    = src->stage;
	dst->resource = src->resource;
}

static void copy_pipeline_descriptor_set(struct pipeline_descriptor_set* dst, const struct pipeline_descriptor_set* src) {
	dst->name        = copy_string(src->name);
	dst->count       = src->count;
	dst->descriptors = null;
	vector(struct pipeline_descriptor) descs_v = null;
	vector_allocate(descs_v, src->count);

	for (usize ii = 0; ii < src->count; ii++) {
		struct pipeline_descriptor desc = { 0 };
		const struct pipeline_descriptor* other_desc = src->descriptors + ii;

		copy_pipeline_descriptor(&desc, other_desc);

		vector_push(descs_v, desc);
	}

	dst->descriptors = descs_v;
}

struct pipeline* video_vk_new_pipeline(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets) {
	struct pipeline_config default_config = default_pipeline_config();

	return video_vk_new_pipeline_ex(flags, shader, framebuffer, attrib_bindings, descriptor_sets, &default_config);
}

struct pipeline* video_vk_new_pipeline_ex(u32 flags, const struct shader* shader, const struct framebuffer* framebuffer,
	struct pipeline_attribute_bindings attrib_bindings, struct pipeline_descriptor_sets descriptor_sets, const struct pipeline_config* config) {
	struct video_vk_pipeline* pipeline = core_calloc(1, sizeof(struct video_vk_pipeline));

	pipeline->config = *config;

	pipeline->flags = flags;
	pipeline->shader = shader;
	pipeline->framebuffer = framebuffer;

	if (descriptor_sets.count > 0) {
		vector_allocate(pipeline->descriptor_sets, descriptor_sets.count);

		for (usize i = 0; i < descriptor_sets.count; i++) {
			struct pipeline_descriptor_set set = { 0 };
			const struct pipeline_descriptor_set* other = descriptor_sets.sets + i;

			copy_pipeline_descriptor_set(&set, other);

			vector_push(pipeline->descriptor_sets, set);
		}
	}

	if (~flags & pipeline_flags_compute) {
		pipeline->bindings.count = attrib_bindings.count;
		pipeline->bindings.bindings = core_calloc(attrib_bindings.count, sizeof *pipeline->bindings.bindings);

		for (usize i = 0; i < pipeline->bindings.count; i++) {
			struct pipeline_attribute_binding* dst = (void*)(pipeline->bindings.bindings + i);
			const struct pipeline_attribute_binding* src = attrib_bindings.bindings + i;

			dst->rate    = src->rate;
			dst->stride  = src->stride;
			dst->binding = src->binding;
			dst->attributes.count = src->attributes.count;
			dst->attributes.attributes = core_calloc(dst->attributes.count, sizeof(struct pipeline_attribute));

			for (usize j = 0; j < dst->attributes.count; j++) {
				struct pipeline_attribute* attrib = (void*)(dst->attributes.attributes + j);
				const struct pipeline_attribute* other = src->attributes.attributes + j;

				attrib->name     = null;
				attrib->location = other->location;
				attrib->offset   = other->offset;
				attrib->type     = other->type;
			}
		}
	}

	list_push(vctx.pipelines, pipeline);

	if (flags & pipeline_flags_compute) {
		init_compute_pipeline(pipeline, flags, (const struct video_vk_shader*)shader, &descriptor_sets);
	} else {
		init_pipeline(pipeline, flags, (const struct video_vk_shader*)shader,
			(const struct video_vk_framebuffer*)framebuffer, &attrib_bindings, &descriptor_sets, config);
	}

	return (struct pipeline*)pipeline;
}

struct pipeline* video_vk_new_compute_pipeline(u32 flags, const struct shader* shader, struct pipeline_descriptor_sets descriptor_sets) {
	return video_vk_new_pipeline(flags | pipeline_flags_compute, shader, null,
		(struct pipeline_attribute_bindings) { .count = 0, .bindings = null },
	descriptor_sets);
}

void video_vk_free_pipeline(struct pipeline* pipeline_) {
	if (vctx.in_frame) {
		vector_push(vctx.free_queue, ((struct free_queue_item) {
			.type = video_vk_object_pipeline,
			.as.pipeline = pipeline_
		}));
		return;
	}

	struct video_vk_pipeline* pipeline = (struct video_vk_pipeline*)pipeline_;

	list_remove(vctx.pipelines, pipeline);

	deinit_pipeline(pipeline);

	for (usize i = 0; i < vector_count(pipeline->descriptor_sets); i++) {
		struct pipeline_descriptor_set* set = (void*)(pipeline->descriptor_sets + i);

		core_free((void*)set->name);

		for (usize ii = 0; ii < set->count; ii++) {
			core_free((void*)set->descriptors[ii].name);
		}

		free_vector(set->descriptors);
	}

	free_vector(pipeline->descriptor_sets);

	if (~pipeline->flags & pipeline_flags_compute) {
		for (usize i = 0; i < pipeline->bindings.count; i++) {
			struct pipeline_attribute_binding* binding = (void*)(pipeline->bindings.bindings + i);

			core_free(binding->attributes.attributes);
		}

		core_free(pipeline->bindings.bindings);
	}

	core_free(pipeline);
}

void video_vk_begin_pipeline(const struct pipeline* pipeline_) {
	const struct video_vk_pipeline* pipeline = (const struct video_vk_pipeline*)pipeline_;

	bool is_compute = pipeline->flags & pipeline_flags_compute;

	VkPipelineBindPoint point = is_compute ?
		VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

	vkCmdBindPipeline(pipeline->command_buffers[vctx.current_frame], point, pipeline->pipeline);
}

void video_vk_end_pipeline(const struct pipeline* pipeline) {

}

void video_vk_invoke_compute(v3u group_count) {
	vkCmdDispatch(vctx.command_buffers[vctx.current_frame], group_count.x, group_count.y, group_count.z);
}

void video_vk_recreate_pipeline(struct pipeline* pipeline_) {
	struct video_vk_pipeline* pipeline = (struct video_vk_pipeline*)pipeline_;

	deinit_pipeline(pipeline);

	if (pipeline->flags & pipeline_flags_compute) {
		init_compute_pipeline(pipeline, pipeline->flags,
			(const struct video_vk_shader*)pipeline->shader,
			&(struct pipeline_descriptor_sets) {
				.sets = pipeline->descriptor_sets,
				.count = vector_count(pipeline->descriptor_sets)
			});
	} else {
		init_pipeline(pipeline, pipeline->flags, (const struct video_vk_shader*)pipeline->shader,
			(const struct video_vk_framebuffer*)pipeline->framebuffer,
			&pipeline->bindings, &(struct pipeline_descriptor_sets) {
				.sets = pipeline->descriptor_sets,
				.count = vector_count(pipeline->descriptor_sets)
			}, &pipeline->config);
	}
}

void video_vk_update_pipeline_uniform(struct pipeline* pipeline_, const char* set, const char* descriptor, const void* data) {
	struct video_vk_pipeline* pipeline = (struct video_vk_pipeline*)pipeline_;

	u64 set_name_hash  = hash_string(set);
	u64 desc_name_hash = hash_string(descriptor);

	struct video_vk_impl_descriptor_set** set_ptr = table_get(pipeline->set_table, set_name_hash);
	if (!set_ptr) {
		error("%s: No such descriptor set.", set);
		return;
	}

	struct video_vk_impl_descriptor_set* desc_set = *set_ptr;

	struct video_vk_impl_uniform_buffer** uniform_ptr = table_get(desc_set->uniforms, desc_name_hash);
	if (!uniform_ptr) {
		error("%s: No such uniform buffer on descriptor set `%s'.", descriptor, set);
		return;
	}

	struct video_vk_impl_uniform_buffer* uniform = *uniform_ptr;

	for (usize i = 0; i < max_frames_in_flight; i++) {
		add_memcpy_cmd(vctx.update_queues + i, uniform->datas[i], data, uniform->size);
	}
}

void video_vk_bind_pipeline_descriptor_set(struct pipeline* pipeline_, const char* set, usize target) {
	struct video_vk_pipeline* pipeline = (struct video_vk_pipeline*)pipeline_;

	struct video_vk_impl_descriptor_set** set_ptr = table_get(pipeline->set_table, hash_string(set));
	if (!set_ptr) {
		error("%s: No such descriptor set.", set);
		return;
	}

	struct video_vk_impl_descriptor_set* desc_set = *set_ptr;

	VkPipelineBindPoint point = pipeline->flags & pipeline_flags_compute ?
		VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

	vkCmdBindDescriptorSets(pipeline->command_buffers[vctx.current_frame], point,
		pipeline->layout, (u32)target, 1,
		desc_set->sets + vctx.current_frame, 0, null);
}

struct storage* video_vk_new_storage(u32 flags, usize size, void* initial_data) {
	struct video_vk_storage* storage = core_calloc(1, sizeof *storage);

	storage->flags = flags;
	storage->size = size;

	VkBufferUsageFlags usage =
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	VmaAllocationCreateFlags a_flags = 0;

	if (flags & storage_flags_vertex_buffer) {
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}

	if (flags & storage_flags_index_buffer) {
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		storage->index_type = VK_INDEX_TYPE_UINT16;
		if (flags & storage_flags_32bit_indices) {
			storage->index_type = VK_INDEX_TYPE_UINT32;
		}
	}

	if (flags & storage_flags_cpu_readable || flags & storage_flags_cpu_writable) {
		props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		a_flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

		new_buffer(size, usage, props, a_flags, &storage->buffer, &storage->memory);

		if (initial_data) {
			void* data;
			vmaMapMemory(vctx.allocator, storage->memory, &data);
			memcpy(data, initial_data, size);
			vmaUnmapMemory(vctx.allocator, storage->memory);
		}
	} else {
		new_buffer(size, usage, props, a_flags, &storage->buffer, &storage->memory);

		if (initial_data) {
			VkBuffer stage;
			VmaAllocation stage_memory;

			new_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
				&stage, &stage_memory);

			void* data;
			vmaMapMemory(vctx.allocator, stage_memory, &data);
			memcpy(data, initial_data, size);
			vmaUnmapMemory(vctx.allocator, stage_memory);

			copy_buffer(storage->buffer, stage, size, vctx.command_pool, vctx.graphics_compute_queue);

			vmaDestroyBuffer(vctx.allocator, stage, stage_memory);
		}
	}

	return (struct storage*)storage;
}

void video_vk_update_storage(struct storage* storage_, u32 mode, void* data) {
	struct video_vk_storage* storage = (struct video_vk_storage*)storage_;

	abort_with("Not implemented.");
}

void video_vk_update_storage_region(struct storage* storage_, u32 mode, void* data, usize offset, usize size) {
	struct video_vk_storage* storage = (struct video_vk_storage*)storage_;

	abort_with("Not implemented.");
}

void video_vk_copy_storage(struct storage* dst_, usize dst_offset, const struct storage* src_, usize src_offset, usize size) {
	struct video_vk_storage* dst = (struct video_vk_storage*)dst_;
	struct video_vk_storage* src = (struct video_vk_storage*)src_;

	VkCommandBuffer cb = begin_temp_command_buffer(vctx.command_pool);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = src->buffer,
			.size = size
	}, 0, null);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = dst->buffer,
			.size = size
	}, 0, null);

	vkCmdCopyBuffer(cb, src->buffer, dst->buffer, 1, &(VkBufferCopy) {
		.size = size,
		.srcOffset = src_offset,
		.dstOffset = dst_offset
	});

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = src->buffer,
			.size = size
	}, 0, null);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = dst->buffer,
			.size = size
	}, 0, null);


	end_temp_command_buffer(cb, vctx.command_pool, vctx.graphics_compute_queue);
}

void video_vk_storage_barrier(struct storage* storage_, u32 state) {
	struct video_vk_storage* storage = (struct video_vk_storage*)storage_;

	VkAccessFlags src_access, dst_access;
	u32 src_queue, dst_queue;
	VkPipelineStageFlags src_stage, dst_stage;

	switch (storage->state) {
		case storage_state_compute_read:
			src_access = VK_ACCESS_SHADER_READ_BIT;
			src_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_compute_write:
			src_access = VK_ACCESS_SHADER_WRITE_BIT;
			src_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_compute_read_write:
			src_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			src_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_fragment_read:
			src_access = VK_ACCESS_SHADER_READ_BIT;
			src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case storage_state_vertex_read:
			src_access = VK_ACCESS_SHADER_READ_BIT;
			src_stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case storage_state_fragment_write:
			src_access = VK_ACCESS_SHADER_WRITE_BIT;
			src_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case storage_state_vertex_write:
			src_access = VK_ACCESS_SHADER_WRITE_BIT;
			src_stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case storage_state_dont_care:
			src_access = VK_ACCESS_SHADER_READ_BIT;
			src_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
	}

	switch (storage->state) {
		case storage_state_compute_read:
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			dst_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_compute_write:
			dst_access = VK_ACCESS_SHADER_WRITE_BIT;
			dst_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_compute_read_write:
			dst_access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			dst_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case storage_state_fragment_read:
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			dst_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case storage_state_vertex_read:
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			dst_stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case storage_state_fragment_write:
			dst_access = VK_ACCESS_SHADER_WRITE_BIT;
			dst_stage  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case storage_state_vertex_write:
			dst_access = VK_ACCESS_SHADER_WRITE_BIT;
			dst_stage  = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case storage_state_dont_care:
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			dst_stage  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	}

	vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
		src_stage, dst_stage,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = src_access,
			.dstAccessMask = dst_access,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = storage->buffer,
			.size = storage->size
		}, 0, null);
	
	storage->state = state;
}

void video_vk_storage_bind_as(const struct storage* storage_, u32 as, u32 point) {
	const struct video_vk_storage* storage = (const struct video_vk_storage*)storage_;

	switch (as) {
		case storage_bind_as_vertex_buffer: {
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(vctx.command_buffers[vctx.current_frame], point, 1, &storage->buffer, offsets);
		} break;
		case storage_bind_as_index_buffer:
			vkCmdBindIndexBuffer(vctx.command_buffers[vctx.current_frame], storage->buffer, 0, storage->index_type);
			break;
	}
}

void video_vk_free_storage(struct storage* storage_) {
	struct video_vk_storage* storage = (struct video_vk_storage*)storage_;

	vkDeviceWaitIdle(vctx.device);

	vmaDestroyBuffer(vctx.allocator, storage->buffer, storage->memory);

	core_free(storage);
}

struct vertex_buffer* video_vk_new_vertex_buffer(void* verts, usize size, u32 flags) {
	struct video_vk_vertex_buffer* vb = core_calloc(1, sizeof(struct video_vk_vertex_buffer));

	vb->flags = flags;

	VkBufferUsageFlags usage = 0;
	
	if (flags & vertex_buffer_flags_transferable) {
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		flags |= vertex_buffer_flags_dynamic;
	}

	if (flags & vertex_buffer_flags_dynamic) {
		new_buffer(size, usage | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, &vb->buffer, &vb->memory);

		vmaMapMemory(vctx.allocator, vb->memory, &vb->data);

		if (verts) {
			memcpy(vb->data, verts, size);
		}
	} else {
		VkBuffer stage;
		VmaAllocation stage_memory;

		new_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			&stage, &stage_memory);
		
		void* data;
		vmaMapMemory(vctx.allocator, stage_memory, &data);
		memcpy(data, verts, size);
		vmaUnmapMemory(vctx.allocator, stage_memory);

		new_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vb->buffer, &vb->memory);
		copy_buffer(vb->buffer, stage, size, vctx.command_pool, vctx.graphics_compute_queue);

		vmaDestroyBuffer(vctx.allocator, stage, stage_memory);
	}

	return (struct vertex_buffer*)vb;
}

void video_vk_free_vertex_buffer(struct vertex_buffer* vb_) {
	struct video_vk_vertex_buffer* vb = (struct video_vk_vertex_buffer*)vb_;

	vkDeviceWaitIdle(vctx.device);

	if (vb->flags & vertex_buffer_flags_dynamic) {
		vmaUnmapMemory(vctx.allocator, vb->memory);
	}

	vmaDestroyBuffer(vctx.allocator, vb->buffer, vb->memory);

	core_free(vb);
}

void video_vk_bind_vertex_buffer(const struct vertex_buffer* vb_, u32 point) {
	const struct video_vk_vertex_buffer* vb = (const struct video_vk_vertex_buffer*)vb_;

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(vctx.command_buffers[vctx.current_frame], point, 1, &vb->buffer, offsets);
}

void video_vk_update_vertex_buffer(struct vertex_buffer* vb_, const void* data, usize size, usize offset) {
	struct video_vk_vertex_buffer* vb = (struct video_vk_vertex_buffer*)vb_;

#ifdef debug
	if (~vb->flags & vertex_buffer_flags_dynamic) {
		error("Attempting to call `update_vertex_buffer' on a non-dynamic vertex buffer.");
		return;
	}
#endif

	add_memcpy_cmd(vctx.update_queues + vctx.current_frame, ((u8*)vb->data) + offset, data, size);
}

void video_vk_copy_vertex_buffer(struct vertex_buffer* dst_, usize dst_offset, const struct vertex_buffer* src_, usize src_offset, usize size) {
	VkBuffer src = ((struct video_vk_vertex_buffer*)src_)->buffer;
	VkBuffer dst = ((struct video_vk_vertex_buffer*)dst_)->buffer;

	VkCommandBuffer cb = begin_temp_command_buffer(vctx.command_pool);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = src,
			.size = size
	}, 0, null);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = dst,
			.size = size
	}, 0, null);

	VkBufferCopy copy = {
		.srcOffset = (VkDeviceSize)src_offset,
		.dstOffset = (VkDeviceSize)dst_offset,
		.size      = (VkDeviceSize)size
	};
	vkCmdCopyBuffer(cb, src, dst, 1, &copy);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = src,
			.size = size
	}, 0, null);

	vkCmdPipelineBarrier(cb,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 1, &(VkBufferMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.buffer = dst,
			.size = size
	}, 0, null);

	end_temp_command_buffer(cb, vctx.command_pool, vctx.graphics_compute_queue);
}

struct index_buffer* video_vk_new_index_buffer(void* elements, usize count, u32 flags) {
	struct video_vk_index_buffer* ib = core_calloc(1, sizeof(struct video_vk_index_buffer));

	usize el_size = sizeof(u16);
	ib->index_type = VK_INDEX_TYPE_UINT16;
	if (flags & index_buffer_flags_u32) {
		el_size = sizeof(u32);
		ib->index_type = VK_INDEX_TYPE_UINT32;
	}

	ib->flags = flags;

	VkBuffer stage;
	VmaAllocation stage_memory;

	VkDeviceSize size = count * el_size;

	new_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		&stage, &stage_memory);
	
	void* data;
	vmaMapMemory(vctx.allocator, stage_memory, &data);
	memcpy(data, elements, size);
	vmaUnmapMemory(vctx.allocator, stage_memory);

	new_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ib->buffer, &ib->memory);
	copy_buffer(ib->buffer, stage, size, vctx.command_pool, vctx.graphics_compute_queue);

	vmaDestroyBuffer(vctx.allocator, stage, stage_memory);

	return (struct index_buffer*)ib;
}

void video_vk_free_index_buffer(struct index_buffer* ib_) {
	struct video_vk_index_buffer* ib = (struct video_vk_index_buffer*)ib_;

	vkDeviceWaitIdle(vctx.device);

	vmaDestroyBuffer(vctx.allocator, ib->buffer, ib->memory);

	core_free(ib);
}

void video_vk_bind_index_buffer(const struct index_buffer* ib_) {
	const struct video_vk_index_buffer* ib = (const struct video_vk_index_buffer*)ib_;

	vkCmdBindIndexBuffer(vctx.command_buffers[vctx.current_frame], ib->buffer, 0, ib->index_type);
}

void video_vk_draw(usize count, usize offset, usize instances) {
	vkCmdDraw(vctx.command_buffers[vctx.current_frame], (u32)count, (u32)instances, (u32)offset, 0);
	vctx.draw_call_count++;
}

void video_vk_draw_indexed(usize count, usize offset, usize instances) {
	vkCmdDrawIndexed(vctx.command_buffers[vctx.current_frame], (u32)count, (u32)instances, (u32)offset, 0, 0);
	vctx.draw_call_count++;
}

void video_vk_set_scissor(v4i rect) {
	v2i d = make_v2i(rect.x < 0 ? -rect.x : 0, rect.y < 0 ? -rect.y : 0);	

	vkCmdSetScissor(vctx.command_buffers[vctx.current_frame], 0, 1,
		&(VkRect2D) {
			.offset = {
				.x = cr_max(rect.x, 0),
				.y = cr_max(rect.y, 0)
			},
			.extent = {
				.width  = (u32)cr_max(rect.z - d.x, 1),
				.height = (u32)cr_max(rect.w - d.y, 1)
			}
		});
}

static VkShaderModule new_shader_module(const u8* buf, usize buf_size) {
	VkShaderModule m;

	if (vkCreateShaderModule(vctx.device, &(VkShaderModuleCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = buf_size,
			.pCode = (u32*)buf
		}, null, &m) != VK_SUCCESS) {
		error("Failed to create shader module.");
		return VK_NULL_HANDLE;
	}
	
	return m;
}

static void init_shader(struct video_vk_shader* shader, const struct shader_header* header, const u8* data) {
	shader->is_compute = header->is_compute;

	if (header->is_compute) {
		shader->compute = new_shader_module(data + header->compute_header.offset, header->compute_header.size);
	} else {
		shader->vertex   = new_shader_module(data + header->raster_header.v_offset, header->raster_header.v_size);
		shader->fragment = new_shader_module(data + header->raster_header.f_offset, header->raster_header.f_size);
	}
}

static void deinit_shader(struct video_vk_shader* shader) {
	if (shader->is_compute) {
		vkDestroyShaderModule(vctx.device, shader->compute, null);
	} else {
		vkDestroyShaderModule(vctx.device, shader->vertex,   null);
		vkDestroyShaderModule(vctx.device, shader->fragment, null);
	}
}

struct shader* video_vk_new_shader(const u8* data, usize data_size) {
	struct video_vk_shader* shader = core_calloc(1, sizeof *shader);

	struct shader_header* header = (struct shader_header*)data;

	if (memcmp("CSH", header->header, 3) != 0) {
		error("Invalid shader data.");
		return null;
	}

	init_shader(shader, header, data);

	return (struct shader*)shader;
}

void video_vk_free_shader(struct shader* shader) {
	deinit_shader((struct video_vk_shader*)shader);
	core_free(shader);
}

static void init_texture(struct video_vk_texture* texture, const struct image* image, u32 flags) {
	texture->size = image->size;

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VkDeviceSize image_size =
		(VkDeviceSize)image->size.x *
		(VkDeviceSize)image->size.y *
		(VkDeviceSize)4;

	VkBuffer stage;
	VmaAllocation stage_memory;

	new_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		&stage, &stage_memory);

	void* data;
	vmaMapMemory(vctx.allocator, stage_memory, &data);

	if (image->colours) {
		memcpy(data, image->colours, image_size);
	} else {
		memset(data, 0, image_size);
	}

	vmaUnmapMemory(vctx.allocator, stage_memory);

	u32 usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	if (flags & texture_flags_storage) {
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	}

	new_image(image->size, format, VK_IMAGE_TILING_OPTIMAL,
		usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&texture->image, &texture->memory, VK_IMAGE_LAYOUT_UNDEFINED, false);
	
	change_image_layout(texture->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, false);
	copy_buffer_to_image(stage, texture->image, image->size, vctx.command_pool, vctx.graphics_compute_queue);
	change_image_layout(texture->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false);

	texture->state = texture_state_shader_graphics_read;

	vmaDestroyBuffer(vctx.allocator, stage, stage_memory);

	texture->view = new_image_view(texture->image, format, VK_IMAGE_ASPECT_COLOR_BIT);

	if (vkCreateSampler(vctx.device, &(VkSamplerCreateInfo) {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = (flags & texture_flags_filter_linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
			.minFilter = (flags & texture_flags_filter_linear) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_FALSE,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
		}, null, &texture->sampler) != VK_SUCCESS) {
		abort_with("Failed to create texture sampler.");
	}
}

static void deinit_texture(struct video_vk_texture* texture) {
	vkDeviceWaitIdle(vctx.device);

	vkDestroySampler(vctx.device, texture->sampler, null);
	vkDestroyImageView(vctx.device, texture->view, null);
	vmaDestroyImage(vctx.allocator, texture->image, texture->memory);
}

struct texture* video_vk_new_texture(const struct image* image, u32 flags) {
	struct video_vk_texture* texture = core_calloc(1, sizeof(struct video_vk_texture));

	init_texture(texture, image, flags);

	return (struct texture*)texture;
}

void video_vk_free_texture(struct texture* texture_) {
	if (vctx.in_frame) {
		vector_push(vctx.free_queue, ((struct free_queue_item) {
			.type = video_vk_object_texture,
			.as.texture = texture_
		}));
		return;
	}

	struct video_vk_texture* texture = (struct video_vk_texture*)texture_;

	deinit_texture(texture);

	core_free(texture);
}

v2i video_vk_get_texture_size(const struct texture* texture) {
	return ((const struct video_vk_texture*)texture)->size;
}

void video_vk_texture_copy(struct texture* dst_, v2i dst_offset, const struct texture* src_, v2i src_offset, v2i dimensions) {
	struct video_vk_texture* dst = (struct video_vk_texture*)dst_;
	const struct video_vk_texture* src = (const struct video_vk_texture*)src_;

	VkCommandBuffer command_buffer = begin_temp_command_buffer(vctx.command_pool);

	/* Transition src to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL. */
	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, null, 0, null, 1, 
		&(VkImageMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = src->image,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		});
	
	/* Transition dst to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL. */
	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, null, 0, null, 1, 
		&(VkImageMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = dst->image,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		});

	vkCmdCopyImage(
		command_buffer,
		src->image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst->image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, (VkImageCopy[]) {
			{
				.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
				.srcOffset = { src_offset.x, src_offset.y, 0 },
				.dstOffset = { dst_offset.x, dst_offset.y, 0 },
				.extent    = { dimensions.x, dimensions.y, 1 }
			}
		});
	
	/* Transition src to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. */
	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 0, null, 1, 
		&(VkImageMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = src->image,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		});
	
	/* Transition dst to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. */
	vkCmdPipelineBarrier(command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
		0, 0, null, 0, null, 1, 
		&(VkImageMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = dst->image,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		});

	end_temp_command_buffer(command_buffer, vctx.command_pool, vctx.graphics_compute_queue);
}

void video_vk_texture_barrier(struct texture* texture_, u32 state) {
	struct video_vk_texture* texture = (struct video_vk_texture*)texture_;

	VkImageLayout old_layout, new_layout;
	VkAccessFlags src_access, dst_access;
	VkPipelineStageFlags old_stage, new_stage;

	switch (texture->state) {
		case texture_state_shader_write:
			old_layout = VK_IMAGE_LAYOUT_GENERAL;
			src_access = VK_ACCESS_SHADER_WRITE_BIT;
			old_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case texture_state_shader_graphics_read:
			old_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			src_access = VK_ACCESS_SHADER_READ_BIT;
			old_stage  = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			break;
		case texture_state_shader_compute_read:
			old_layout = VK_IMAGE_LAYOUT_GENERAL;
			src_access = VK_ACCESS_SHADER_READ_BIT;
			old_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case texture_state_attachment_write:
			old_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
			src_access = texture->is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			old_stage  = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			break;
	}

	switch (state) {
		case texture_state_shader_write:
			new_layout = VK_IMAGE_LAYOUT_GENERAL;
			dst_access = VK_ACCESS_SHADER_WRITE_BIT;
			new_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case texture_state_shader_graphics_read:
			new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			new_stage  = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			break;
		case texture_state_shader_compute_read:
			new_layout = VK_IMAGE_LAYOUT_GENERAL;
			dst_access = VK_ACCESS_SHADER_READ_BIT;
			new_stage  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case texture_state_attachment_write:
			new_layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
			dst_access = texture->is_depth ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			new_stage  = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
			break;
	}

	vkCmdPipelineBarrier(vctx.command_buffers[vctx.current_frame],
		old_stage, new_stage,
		0, 0, null, 0, null, 1, 
		&(VkImageMemoryBarrier) {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = texture->image,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
			.srcAccessMask = src_access,
			.dstAccessMask = dst_access,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
		});
	
	texture->state = state;
}

static void shader_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct shader_header* header = (struct shader_header*)raw;

	memset(payload, 0, payload_size);

	if (memcmp("CSH", header->header, 3) != 0) {
		error("%s is not a valid shader resource.", filename);
		return;
	}

	init_shader(payload, header, raw);
}

static void shader_on_unload(void* payload, usize payload_size) {
	deinit_shader(payload);
}

static void texture_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	struct image image;
	init_image_from_raw(&image, raw, raw_size);

	u32 flags;
	if (udata) {
		flags = *(u32*)udata;
	} else {
		flags = texture_flags_filter_none;
	}

	init_texture(payload, &image, flags);

	deinit_image(&image);
}

static void texture_on_unload(void* payload, usize payload_size) {
	deinit_texture(payload);
}

void video_vk_register_resources() {
	reg_res_type("shader", &(struct res_config) {
		.payload_size = sizeof(struct video_vk_shader),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_csh,
		.alt_raw_size = bir_error_csh_size,
		.on_load = shader_on_load,
		.on_unload = shader_on_unload
	});

	reg_res_type("texture", &(struct res_config) {
		.payload_size = sizeof(struct video_vk_texture),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.alt_raw = bir_error_png,
		.alt_raw_size = bir_error_png_size,
		.on_load = shader_on_load,
		.on_load = texture_on_load,
		.on_unload = texture_on_unload
	});
}

u32 video_vk_get_draw_call_count() {
	return vctx.draw_call_count;
}
