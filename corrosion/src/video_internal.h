#pragma once

#include <vulkan/vulkan.h>

#include "common.h"
#include "video.h"
#include "vk_mem_alloc.h"

#define max_frames_in_flight 3

#pragma pack(push, 1)
struct shader_header {
	char header[2];
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
};
#pragma pack(pop)

struct queue_families {
	i32 graphics;
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

	VkQueue graphics_queue;
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

	u32 current_frame;
	u32 image_id;

	struct update_queue update_queues[max_frames_in_flight];

	/* TODO: Make these linked lists? */
	table(struct video_vk_framebuffer*, struct video_vk_framebuffer*) framebuffers;
	table(struct video_vk_pipeline*,    struct video_vk_pipeline*)    pipelines;
};

struct video_vk_framebuffer_attachment {
	u32 type;

	VkImage images[max_frames_in_flight];
	VkImageView image_views[max_frames_in_flight];
	VmaAllocation image_memories[max_frames_in_flight];
};

static inline VkImageAspectFlags get_vk_frambuffer_attachment_aspect_flags(const struct video_vk_framebuffer_attachment* a) {
	return a->type == framebuffer_attachment_colour ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
}

struct video_vk_framebuffer {
	bool is_headless;
	bool use_depth;

	u32 flags;

	v2i size;

	VkFramebuffer* swapchain_framebuffers;
	VkFramebuffer  offscreen_framebuffers[max_frames_in_flight];

	/* Points to either swapchain_framebuffers if this is
	 * the default framebuffer, or offscreen_framebuffers
	 * if this is a headless framebuffer. */
	VkFramebuffer* framebuffers;

	struct video_vk_framebuffer_attachment* colours;
	usize colour_count;
	struct video_vk_framebuffer_attachment depth;

	/* Used if this is the default framebuffer. On the default
	 * framebuffer, the depth buffer can't be sampled and a
	 * separate one won't be needed for each frame in flight. */
	VkImage depth_image;
	VkImageView depth_image_view;
	VmaAllocation depth_memory;

	table(usize, struct video_vk_framebuffer_attachment*) attachment_map;

	VkClearValue* clear_colours;

	VkRenderPass render_pass;
	VkSampler sampler;

	usize attachment_count;

	struct framebuffer_attachment_desc* attachment_descs;
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

	usize descriptor_set_count;

	struct video_vk_impl_descriptor_set* desc_sets;
	struct video_vk_impl_uniform_buffer* uniforms;

	table(u64, struct video_vk_impl_descriptor_set*) set_table;

	VkDescriptorPool descriptor_pool;

	VkPipelineLayout layout;
	VkPipeline pipeline;

	u32 flags;
	const struct shader* shader;
	const struct framebuffer* framebuffer;
	struct pipeline_attributes attributes;
	vector(struct pipeline_descriptor_set) descriptor_sets;
};

struct video_vk_vertex_buffer {
	u32 flags;

	void* data;

	VkBuffer buffer;
	VmaAllocation memory;
};

struct video_vk_index_buffer {
	u32 flags;

	VkBuffer buffer;
	VmaAllocation memory;
};

struct vk_video_context* get_vk_video_context();

struct video_vk_shader {
	VkShaderModule vertex;
	VkShaderModule fragment;
};

struct video_vk_texture {
	v2i size;

	VkImage image;
	VkImageView view;
	VkSampler sampler;

	VmaAllocation memory;
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
