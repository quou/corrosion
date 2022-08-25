#pragma once

#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1002000

#include "common.h"
#include "video.h"
#include "vk_mem_alloc.h"

#define max_frames_in_flight 3

#pragma pack(push, 1)
struct shader_raster_header {
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
	u64 v_gl_offset;
	u64 f_gl_offset;
	u64 v_gl_size;
	u64 f_gl_size;
};

struct shader_compute_header {
	u64 offset;
	u64 size;
	u64 gl_offset;
	u64 gl_size;
};

struct shader_header {
	char header[3];
	u8 is_compute;
	u64 bind_table_count;
	u64 bind_table_offset;

	union {
		struct shader_raster_header raster_header;
		struct shader_compute_header compute_header;
	};
};
#pragma pack(pop)

struct queue_families {
	i32 graphics_compute;
	i32 present;
};

struct update_queue {
	usize capacity;
	usize count;
	u8* bytes;
};

enum {
	video_vk_object_texture = 0,
	video_vk_object_pipeline,
	video_vk_object_framebuffer
};

struct free_queue_item {
	u32 type;

	union {
		struct texture* texture;
		struct pipeline* pipeline;
		struct framebuffer* framebuffer;
	} as;
};

struct vk_video_context {
	VkInstance instance;

	VkDebugUtilsMessengerEXT messenger;

	VkPhysicalDevice pdevice;
	VkDevice device;

	VkQueue graphics_compute_queue;
	VkQueue present_queue;

	VmaAllocator allocator;

	VkFormat   swapchain_format;
	VkExtent2D swapchain_extent;

	VkSwapchainKHR swapchain;
	
	u32 swapchain_image_count;
	VkImage* swapchain_images;
	VkImageView* swapchain_image_views;

	VkCommandPool command_pool;
	VkCommandBuffer command_buffers[max_frames_in_flight];

	VkSemaphore image_avail_semaphores[max_frames_in_flight];
	VkSemaphore render_finish_semaphores[max_frames_in_flight];
	VkFence in_flight_fences[max_frames_in_flight];

	bool in_frame;
	vector(struct free_queue_item) free_queue;

	struct queue_families qfs;

	struct framebuffer* default_fb;

	bool want_recreate;
	bool enable_vsync;

	u32 current_frame;
	u32 prev_frame;
	u32 image_id;

	usize min_uniform_buffer_offset_alignment;

	struct update_queue update_queues[max_frames_in_flight];

	list(struct video_vk_framebuffer) framebuffers;
	list(struct video_vk_pipeline) pipelines;

	u32 draw_call_count;

	/* Extensions */
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
	PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
};

struct video_vk_texture {
	v2i size;

	u32 state;

	bool is_depth;

	VkImage image;
	VkImageView view;
	VkSampler sampler;

	VmaAllocation memory;
};

struct video_vk_framebuffer_attachment {
	u32 type;

	v4f clear_colour;

	VkFormat format;

	struct video_vk_texture* texture;
};

static inline VkImageAspectFlags get_vk_frambuffer_attachment_aspect_flags(const struct video_vk_framebuffer_attachment* a) {
	return a->type == framebuffer_attachment_colour ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
}

/* Since dynamic rendering is being used (we don't need to use Vulkan
 * framebuffers anymore, this is a "fake" framebuffer.
 *
 * It's sort of just an emulation of a typical framebuffer as it would
 * exist in something like OpenGL, mainly just a container for image
 * and depth attachments. */
struct video_vk_framebuffer {
	bool is_headless;
	bool use_depth;

	u32 flags;

	v2i size;

	struct video_vk_framebuffer_attachment* colours;
	usize colour_count;
	struct video_vk_framebuffer_attachment depth;

	VkFormat* colour_formats;
	VkFormat depth_format;

	VkRenderingAttachmentInfoKHR* colour_infos;

	table(usize, struct video_vk_framebuffer_attachment*) attachment_map;

	VkSampler sampler;

	usize attachment_count;

	struct framebuffer_attachment_desc* attachment_descs;

	struct video_vk_framebuffer* next;
	struct video_vk_framebuffer* prev;
};

struct video_vk_storage {
	VkBuffer buffer;
	VmaAllocation memory;

	VkIndexType index_type;

	usize size;

	u32 flags;
	u32 state;
};

struct video_vk_impl_uniform_buffer {
	VkBuffer buffers[max_frames_in_flight];
	VmaAllocation memories[max_frames_in_flight];

	usize size;
	void* datas[max_frames_in_flight];
};

struct video_vk_impl_descriptor_set {
	VkDescriptorSetLayout layout;
	VkDescriptorSet sets[max_frames_in_flight];

	table(u64, struct video_vk_impl_uniform_buffer*) uniforms;
};

struct video_vk_pipeline {
	usize sampler_count;
	usize uniform_count;
	usize storage_count;
	usize image_storage_count;

	usize descriptor_set_count;

	struct video_vk_impl_descriptor_set* desc_sets;
	struct video_vk_impl_uniform_buffer* uniforms;

	table(u64, struct video_vk_impl_descriptor_set*) set_table;

	VkDescriptorPool descriptor_pool;

	VkPipelineLayout layout;
	VkPipeline pipeline;

	VkCommandBuffer* command_buffers;

	u32 flags;
	const struct shader* shader;
	const struct framebuffer* framebuffer;
	struct pipeline_attribute_bindings bindings;
	vector(struct pipeline_descriptor_set) descriptor_sets;

	struct pipeline_config config;

	struct video_vk_pipeline* next;
	struct video_vk_pipeline* prev;
};

struct video_vk_vertex_buffer {
	u32 flags;

	void* data;

	VkBuffer buffer;
	VmaAllocation memory;
};

struct video_vk_index_buffer {
	u32 flags;

	VkIndexType index_type;

	VkBuffer buffer;
	VmaAllocation memory;
};

struct vk_video_context* get_vk_video_context();

struct video_vk_shader {
	VkShaderModule vertex;
	VkShaderModule fragment;
	VkShaderModule compute;

	bool is_compute;
};

enum {
	update_cmd_memcpy
};

struct update_cmd {
	u32 type;
	usize size;
};

struct update_cmd_memcpy {
	struct update_cmd cmd;
	usize size;
	void* target;
	/* Bytes to call memcpy on are located immediately after this struct in the buffer. */
};

static void add_memcpy_cmd(struct update_queue* buf, void* target, const void* data, usize size);

struct gl_video_context {
	struct framebuffer* default_fb;

	struct video_gl_pipeline* bound_pipeline;
	const struct video_gl_vertex_buffer* bound_vb;
	const struct video_gl_index_buffer*  bound_ib;
	const struct video_gl_framebuffer*   bound_fb;

	u32 draw_call_count;
};

struct video_gl_descriptor {
	u32 binding;
	u32 stage;
	struct pipeline_resource resource;

	u32 ub_id;
	usize ub_size;
};

struct video_gl_descriptor_set {
	table(u64, struct video_gl_descriptor) descriptors;
	usize count;
};

struct video_gl_pipeline {
	u32 flags;

	table(u32, struct pipeline_attribute_binding) attribute_bindings;
	table(u64, struct video_gl_descriptor_set) descriptor_sets;

	const struct video_gl_shader* shader;

	u32 vao;
	u32 mode;

	vector(u32) to_enable;
};

struct video_gl_desc_id {
	u32 set;
	u32 binding;
};

struct video_gl_shader {
	u32 program;

	table(u64, u32) bind_map;
};

struct video_gl_vertex_buffer {
	u32 flags;
	u32 id;

	u32 mode;
};

struct video_gl_index_buffer {
	u32 flags;
	u32 id;
};

struct video_gl_texture {
	u32 flags;
	u32 id;

	u32 state;

	v2i size;
};

struct video_gl_framebuffer {
	u32 id;

	bool has_depth;

	v2i size;

	u32* colours;
	usize colour_count;

	u32 depth_attachment;

	table(usize, u32) attachment_map; 
};
