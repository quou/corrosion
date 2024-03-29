#pragma once

#ifndef cr_no_vulkan
#include <vulkan/vulkan.h>
#endif

#include "common.h"
#include "video.h"

/* The Vulkan spec only requires support for 128 byte push constants,
 * so that's the maximum that we will support. */
#define max_push_const_size 128

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
	u64 sampler_name_table_offset;
};

struct shader_compute_header {
	u64 offset;
	u64 size;
	u64 gl_offset;
	u64 gl_size;
};

struct shader_sampler_name {
	u32 binding;
	u32 location;
	char name[32];
};

struct shader_sampler_name_table_header {
	u64 count;
};

struct shader_header {
	char header[3];
	u8 is_compute;

	union {
		struct shader_raster_header raster_header;
		struct shader_compute_header compute_header;
	};
};
#pragma pack(pop)

void copy_pipeline_descriptor(struct pipeline_descriptor* dst, const struct pipeline_descriptor* src);
void copy_pipeline_descriptor_set(struct pipeline_descriptor_set* dst, const struct pipeline_descriptor_set* src);

#ifndef cr_no_vulkan
#define max_frames_in_flight 3

struct queue_families {
	i32 graphics_compute;
	i32 present;
};

enum {
	video_vk_object_texture = 0,
	video_vk_object_pipeline,
	video_vk_object_framebuffer,
	video_vk_object_storage,
	video_vk_object_vertex_buffer,
	video_vk_object_index_buffer
};


struct free_queue_item {
	u32 type;

	union {
		struct texture* texture;
		struct pipeline* pipeline;
		struct framebuffer* framebuffer;
		struct storage* storage;
		struct vertex_buffer* vertex_buffer;
		struct index_buffer* index_buffer;
	} as;
};

struct video_vk_allocation {
	VkDeviceMemory memory;
	VkDeviceSize start;
	VkDeviceSize size;
	bool free;
	void* ptr;
};

struct video_vk_chunk {
	VkDeviceMemory memory;
	VkDeviceSize size;
	u32 type;
	void* ptr;
	vector(struct video_vk_allocation) allocs;
};

void video_vk_init_chunk(struct video_vk_chunk* chunk, VkDeviceSize size, u32 type);
void video_vk_deinit_chunk(struct video_vk_chunk* chunk);

bool video_vk_chunk_alloc(struct video_vk_chunk* chunk, struct video_vk_allocation* alloc,
	VkDeviceSize size, VkDeviceSize alignment);
i64 video_vk_chunk_find(struct video_vk_chunk* chunk, const struct video_vk_allocation* alloc);

struct video_vk_allocation video_vk_allocate(VkDeviceSize size, VkDeviceSize alignment, u32 type);
void video_vk_free(struct video_vk_allocation* alloc);
void* video_vk_map(struct video_vk_allocation* alloc);

#define video_vk_chunk_min_size (1024 * 1024)

struct vk_video_context {
	VkInstance instance;

	VkDebugUtilsMessengerEXT messenger;

	VkPhysicalDevice pdevice;
	VkDevice device;

	VkQueue graphics_compute_queue;
	VkQueue present_queue;

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

	VkAllocationCallbacks ac;

	vector(struct video_vk_chunk) chunks;

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

	list(struct video_vk_framebuffer) framebuffers;
	list(struct video_vk_pipeline) pipelines;

	u32 draw_call_count;

	/* Extensions */
	PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR;
	PFN_vkCmdEndRenderingKHR vkCmdEndRenderingKHR;
};

struct video_vk_texture {
	v2i size;
	i32 depth; /* Only used if this is a 3D texture. */

	u32 state;

	bool is_depth;
	bool is_3d;

	VkImage image;
	VkImageView view;
	VkSampler sampler;

	struct video_vk_allocation memory;
};

struct video_vk_framebuffer_attachment {
	u32 type;
	bool clear;

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
	struct video_vk_allocation memory;

	VkIndexType index_type;

	void* mapping;

	usize size;

	u32 flags;
	u32 state;
};

struct video_vk_impl_uniform_buffer {
	VkBuffer buffers[max_frames_in_flight];
	struct video_vk_allocation memories[max_frames_in_flight];

	usize size;
	void* datas[max_frames_in_flight];
};

struct video_vk_impl_descriptor_set {
	VkDescriptorSetLayout layout;
	VkDescriptorSet sets[max_frames_in_flight];

	table(const char*, struct video_vk_impl_uniform_buffer*) uniforms;
};

struct video_vk_pipeline {
	usize sampler_count;
	usize uniform_count;
	usize storage_count;
	usize image_storage_count;

	usize descriptor_set_count;

	struct video_vk_impl_descriptor_set* desc_sets;
	struct video_vk_impl_uniform_buffer* uniforms;

	table(const char*, struct video_vk_impl_descriptor_set*) set_table;

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
	struct video_vk_allocation memory;
};

struct video_vk_index_buffer {
	u32 flags;

	VkIndexType index_type;

	VkBuffer buffer;
	struct video_vk_allocation memory;
};

struct vk_video_context* get_vk_video_context();

struct video_vk_shader {
	VkShaderModule vertex;
	VkShaderModule fragment;
	VkShaderModule compute;

	bool is_compute;
};

#endif /* cr_no_vulkan */

#ifndef cr_no_opengl
struct gl_video_context {
	struct framebuffer* default_fb;

	bool want_recreate;

	struct video_gl_pipeline* bound_pipeline;
	const struct video_gl_vertex_buffer* bound_vb;
	const struct video_gl_index_buffer*  bound_ib;
	const struct video_gl_framebuffer*   bound_fb;

	list(struct video_gl_framebuffer) framebuffers;
	list(struct video_gl_pipeline) pipelines;

	u32 draw_call_count;

	v4f default_clear;
};

struct video_gl_descriptor {
	u32 binding;
	u32 stage;
	struct pipeline_resource resource;

	u32 ub_id;
	usize ub_size;
};

struct video_gl_descriptor_set {
	u32 index;
	table(u64, struct video_gl_descriptor) descriptors;
	usize count;
};

struct video_gl_pipeline {
	u32 flags;

	table(u32, struct pipeline_attribute_binding) attribute_bindings;
	table(u64, struct video_gl_descriptor_set) descriptor_sets;

	struct video_gl_shader* shader;
	const struct video_gl_framebuffer* framebuffer;

	struct pipeline_config config;

	u32 vao;
	u32 mode;

	vector(u32) to_enable;

	struct video_gl_pipeline* next;
	struct video_gl_pipeline* prev;
};

struct video_gl_desc_id {
	u32 set;
	u32 binding;
};

struct video_gl_shader {
	u32 program;
	table(u32, u32) sampler_locs;
};

struct video_gl_vertex_buffer {
	u32 flags;
	u32 id;

	u32 mode;
};

struct video_gl_index_buffer {
	u32 flags;
	u32 id;

	u32 type;
	usize el_size;
};

struct video_gl_texture {
	u32 flags;
	u32 id;

	u32 state;

	v2i size;

	u32 format;
	u32 type;
};

struct video_gl_framebuffer {
	u32 id;

	u32 flags;

	bool has_depth;

	v2i size;

	u32* colours;
	usize colour_count;

	v4f* clear_colours;

	u32 depth_attachment;

	u32* colour_formats;
	u32* colour_types;

	u32* draw_buffers;
	u32* draw_buffers2;

	u32 flipped_fb;
	struct video_gl_texture* flipped_colours;
	struct video_gl_texture flipped_depth;

	usize attachment_count;

	table(usize, struct video_gl_texture*) attachment_map;

	struct video_gl_framebuffer* next;
	struct video_gl_framebuffer* prev;
};

#endif /* cr_no_opengl */
