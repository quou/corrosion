/* Corrosion C++ header. */

#include <cstdint>
#include <ctime>
#include <utility>
#include <string>
#include <functional>
#include <iostream>
#include <cmath>

namespace corrosion {
	namespace impl {
		extern "C" {
			#include "cr.h"
		}
	}

	typedef int8_t   i8;
	typedef int16_t  i16;
	typedef int32_t  i32;
	typedef int64_t  i64;
	typedef uint8_t  u8;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;
	
	typedef float  f32;
	typedef double f64;

	typedef size_t usize;

#define cr_decl_v2_cpp(n_, T, b_) \
	struct n_ : public b_ { \
		n_() : b_(b_ { 0, 0 }) {} \
		n_(const b_& v) : b_(v) {} \
		n_(T xy)     : b_(b_ { xy, xy }) {} \
		n_(T x, T y) : b_(b_ { x,  y }) {} \
	};

#define cr_decl_v3_cpp(n_, T, b_, vec2_t) \
	struct n_ : public b_ { \
		n_(const b_& v)   : b_(v) {} \
		n_()              : b_(b_ { 0, 0, 0 }) {} \
		n_(T xyz)         : b_(b_ { xyz, xyz, xyz }) {}  \
		n_(T x, T y, T z) : b_(b_ { x,   y,   z }) {}  \
		n_(const vec2_t& xy, T z) : b_(b_ { xy.x, xy.y, z }) {} \
		n_(T x, const vec2_t& yz) : b_(b_ { x, yz.x, yz.y }) {} \
	};

#define cr_decl_v4_cpp(n_, T, b_, vec2_t, vec3_t) \
	struct n_ : public b_ { \
		n_(const b_& v)        : b_(v) {} \
		n_()                   : b_(b_ { 0, 0, 0, 0 }) {} \
		n_(T xyzw)             : b_(b_ { xyzw, xyzw, xyzw, xyzw }) {}  \
		n_(T x, T y, T z, T w) : b_(b_ { x,    y,    z,    w }) {}  \
		n_(const vec2_t& xy, T z, T w) : b_(b_ { xy.x, xy.y, z, w }) {} \
		n_(T x, const vec2_t& yz, T w) : b_(b_ { x, yz.x, yz.y, w }) {} \
		n_(T x, T y, const vec2_t& zw) : b_(b_ { x, y, zw.x, zw.y }) {} \
		n_(const vec3_t& xyz, T w) : b_(b_ { xyz.x, xyz.y, xyz.z, w }) {} \
		n_(T x, const vec3_t& yzw) : b_(b_ { x, yzw.x, yzw.y, yzw.z }) {} \
	};

	cr_decl_v2_cpp(v2f, f32, impl::v2f);
	cr_decl_v2_cpp(v2i, i32, impl::v2i);
	cr_decl_v2_cpp(v2u, u32, impl::v2u);
	cr_decl_v3_cpp(v3f, f32, impl::v3f, v2f);
	cr_decl_v3_cpp(v3i, i32, impl::v3i, v2i);
	cr_decl_v3_cpp(v3u, u32, impl::v3u, v2u);
	cr_decl_v4_cpp(v4f, f32, impl::v4f, v2f, v3f);
	cr_decl_v4_cpp(v4i, i32, impl::v4i, v2i, v3i);
	cr_decl_v4_cpp(v4u, u32, impl::v4u, v2u, v3u);

	struct quat {
		f32 x, y, z, w;
	};
	
	static inline v3f make_rgb(u32 rgb) {
		return v3f(
			(f32)((((u32)(rgb)) >> 16) & 0xff) / 255.0f,
			(f32)((((u32)(rgb)) >> 8)  & 0xff) / 255.0f,
			(f32)(((u32)(rgb))         & 0xff) / 255.0f);
	}

	static inline v4f make_rgba(u32 rgb, u8 a) {
		return v4f(make_rgb(rgb), (f32)a / 255.0f);
	}
	class Texture;

	class Framebuffer {
	private:
		impl::framebuffer* as_impl() {
			return reinterpret_cast<impl::framebuffer*>(this);
		}
	public:
		Framebuffer() = delete;

		struct Attachment_Desc {
			const char* name;

			enum class Type : u32 {
				colour = impl::framebuffer_attachment_colour,
				depth  = impl::framebuffer_attachment_depth
			} type;

			enum class Format : u32 {
				depth   = impl::framebuffer_format_depth,
				rgba8i  = impl::framebuffer_format_rgba8i,
				rgba16f = impl::framebuffer_format_rgba16f,
				rgba32f = impl::framebuffer_format_rgba32f
			};

			v4f clear_colour;
		};

		enum class Flags {
			fit                     = impl::framebuffer_flags_fit,
			headless                = impl::framebuffer_flags_headless,
			attachments_are_storage = impl::framebuffer_flags_attachments_are_storage
		};

		template <typename... Args>
		static Framebuffer* create(Flags flags, v2i size, Args... descs) {
			Attachment_Desc d[] = {
				descs...
			};

			impl::framebuffer_attachment_desc dc[sizeof d / sizeof *d];

			for (usize i = 0; i < sizeof d / sizeof *d; i++) {
				dc[i].name = d[i].name;
				dc[i].type = d[i].type;
				dc[i].format = d[i].format;
				dc[i].clear_colour = d[i].clear_colour;
			}

			return reinterpret_cast<Framebuffer*>(impl::video.new_framebuffer(
				static_cast<u32>(flags), size,
				dc, sizeof d / sizeof *d
			));
		}

		void release() {
			impl::video.free_framebuffer(as_impl());
		}

		void begin() {
			impl::video.begin_framebuffer(as_impl());
		}

		void end() {
			impl::video.end_framebuffer(as_impl());
		}

		v2i get_size() {
			return impl::video.get_framebuffer_size(as_impl());
		}

		void resize(v2i new_size) {
			impl::video.resize_framebuffer(as_impl(), new_size);
		}

		Texture* get_attachment(u32 index) {
			return reinterpret_cast<Texture*>(impl::video.get_attachment(as_impl(), index));
		}
	};

	class Video {
	private:
		friend class App_Base;
	public:
		enum class API : u32 {
			none   = impl::video_api_none,
			vulkan = impl::video_api_vulkan,
			opengl = impl::video_api_opengl
		};

		static Framebuffer* get_default_fb() {
			return reinterpret_cast<Framebuffer*>(impl::video.get_default_fb());
		}

		static void begin(bool present = true) {
			impl::video.begin(present);
		}

		static void end(bool present = true) {
			impl::video.end(present);
		}
	};

	class Shader {
	private:
		impl::shader* as_impl() {
			return reinterpret_cast<impl::shader*>(this);
		}
	public:
		Shader() = delete;

		static Shader* create(const u8* data, usize data_size) {
			return reinterpret_cast<Shader*>(impl::video.new_shader(data, data_size));
		}

		void release() {
			impl::video.free_shader(as_impl());
		}
	};

	struct Image : public impl::image {
		Image(v2i size) : impl::image(impl::image { .size = size, .colours = null }) {}
		Image(const u8* raw, usize raw_size) {
			impl::init_image_from_raw(this, raw, raw_size);
		}

		~Image() {
			if (colours) {
				impl::deinit_image(this);
			}
		}

		void flip_y() {
			impl::flip_image_y(this);
		}
	};

	class Texture {
	private:
		impl::texture* as_impl() {
			return reinterpret_cast<impl::texture*>(this);
		}
	public:
		enum class Flags : u32 {
			none          = impl::texture_flags_none,
			filter_linear = impl::texture_flags_filter_linear,
			filter_none   = impl::texture_flags_filter_none,
			storage       = impl::texture_flags_storage,
			repeat        = impl::texture_flags_repeat,
			clamp         = impl::texture_flags_clamp
		};

		enum class Format : u32 {
			r8i     = impl::texture_format_r8i,
			r16f    = impl::texture_format_r16f,
			r32f    = impl::texture_format_r32f,
			rg8i    = impl::texture_format_rg8i,
			rg16f   = impl::texture_format_rg16f,
			rg32f   = impl::texture_format_rg32f,
			rgb8i   = impl::texture_format_rgb8i,
			rgb16f  = impl::texture_format_rgb16f,
			rgb32f  = impl::texture_format_rgb32f,
			rgba8i  = impl::texture_format_rgba8i,
			rgba16f = impl::texture_format_rgba16f,
			rgba32f = impl::texture_format_rgba32f,
			count   = impl::texture_format_count
		};

		enum class State : u32 {
			shader_write          = impl::texture_state_shader_write,
			shader_graphics_read  = impl::texture_state_shader_graphics_read,
			shader_compute_read   = impl::texture_state_shader_compute_read,
			shader_compute_sample = impl::texture_state_shader_compute_sample,
			attachment_write      = impl::texture_state_attachment_write
		};

		Texture() = delete;

		static Texture* create(const Image* image, Flags flags, Format format) {
			return reinterpret_cast<Texture*>(impl::video.new_texture(image,
				static_cast<u32>(flags), static_cast<u32>(format)));
		}

		static Texture* create(v3i size, Flags flags, Format format) {
			return reinterpret_cast<Texture*>(impl::video.new_texture_3d(size,
				static_cast<u32>(flags), static_cast<u32>(format)));
		}

		void release() {
			impl::video.free_texture(as_impl());
		}

		v2i get_size() {
			return impl::video.get_texture_size(as_impl());
		}

		v3i get_3d_size() {
			return impl::video.get_texture_3d_size(as_impl());
		}

		void copy_to(Texture* dst, v2i dst_offset, v2i src_offset, v2i region) {
			impl::video.texture_copy(dst->as_impl(), dst_offset, as_impl(), src_offset, region);
		}
		
		void copy_to(Texture* dst, v3i dst_offset, v3i src_offset, v3i region) {
			impl::video.texture_copy_3d(dst->as_impl(), dst_offset, as_impl(), src_offset, region);
		}

		void barrier(State state) {
			impl::video.texture_barrier(as_impl(), static_cast<u32>(state));
		}
	};

	class Font {
	private:
		impl::font* as_impl() {
			return reinterpret_cast<impl::font*>(this);
		}

		friend class Text_Renderer;
	public:
		Font() = delete;

		static usize get_class_size() {
			return impl::font_struct_size();
		}

		static Font* create(const u8* raw, usize raw_size, f32 size) {
			return reinterpret_cast<Font*>(impl::new_font(raw, raw_size, size));
		}

		void release() {
			impl::free_font(as_impl());
		}

		v2f get_char_dimensions(const char* c) {
			return impl::get_char_dimensions(as_impl(), c);
		}

		v2f get_text_dimensions(const char* text) {
			return impl::get_text_dimensions(as_impl(), text);
		}

		v2f get_text_dimensions(const char* text, usize n) {
			return impl::get_text_n_dimensions(as_impl(), text, n);
		}

		f32 get_height() {
			return impl::get_font_height(as_impl());
		}

		void* get_data() {
			return impl::get_font_data(as_impl());
		}
	};

	class Text_Renderer : public impl::text_renderer {
	private:
	public:
		Text_Renderer(void* uptr, void (*draw_char)(void*, const Texture*, v2f, v4f, v4f)) {
			this->uptr = uptr;
			this->draw_character = (void (*)(void*, const impl::texture*, impl::v2f, impl::v4f, impl::v4f))draw_char;
		}

		void render_text(Font* font, const char* text, v2f pos, v4f col) {
			impl::render_text(this, font->as_impl(), text, pos, col);
		}
	};

	struct Resource_Config : public impl::res_config {
		template <typename Payload_T, typename Udata_T>
		static Resource_Config create(
			bool free_raw_on_load, bool terminate_raw,
			const void* alt_raw = null, usize alt_raw_size = 0) {

			Resource_Config cfg{};

			cfg.payload_size = sizeof(Payload_T);
			cfg.udata_size = sizeof(Udata_T);
			cfg.free_raw_on_load = free_raw_on_load;
			cfg.terminate_raw = terminate_raw;
			cfg.alt_raw = alt_raw;
			cfg.alt_raw_size = alt_raw_size;

			return std::move(cfg);
		}
	};

	class Resource : private impl::resource {
	private:
		Resource(void* payload, u64 id)
			: impl::resource(impl::resource { .id = id, .payload = payload }) {}

		friend class Resource_Manager;
	public:
		Resource()
			: impl::resource(impl::resource { .id = UINT64_MAX, .payload = null }) {}

		template <typename T>
		T* as() {
			return reinterpret_cast<T*>(payload);
		}

		u64 get_id() {
			return id;
		}
	};

	class Resource_Manager {
	private:
	public:
		static void reg(const char* type, const Resource_Config& cfg) {
			impl::reg_res_type(type, &cfg);
		}

		Resource load(const char* type, const char* filename, void* udata = null) {
			auto ir = impl::res_load(type, filename, udata);

			return Resource(ir.payload, ir.id);
		}

		void unload(const Resource& r) {
			impl::res_unload(&r);
		}

		Shader* load_shader(const char* filename, Resource* r = null) {
			return reinterpret_cast<Shader*>(impl::load_shader(filename, r));
		}

		Texture* load_texture(const char* filename, Texture::Flags flags, Resource* r = null) {
			return reinterpret_cast<Texture*>(impl::load_texture(filename, static_cast<u32>(flags), r));
		}
		
		Font* load_font(const char* filename, i32 size, Resource* r = null) {
			return reinterpret_cast<Font*>(impl::load_font(filename, size, r));
		}

		static void init(const char* argv0) {
			impl::res_init(argv0);
		}

		static void deinit() {
			impl::res_deinit();
		}
	};

	class App_Base {
	protected:
		virtual void on_init() {}
		virtual void on_update(f64 ts) {}
		virtual void on_deinit() {}
	public:
		struct Config {
			std::string name;

			struct {
				std::string title;
				v2i size;
				bool resizable;
			} window_config;

			struct {
				Video::API api;
				bool enable_validation;
				bool enable_vsync;
				v4f clear_colour;
			} video_config;
		} config;

		App_Base(Config config) : config(std::move(config)) {}

		void run(i32 argc, const char** argv) {
			impl::alloc_init();

			impl::init_timer();

			srand((u32)time(0));

			impl::window_config c_win_conf = {
				.title = config.window_config.title.c_str(),
				.size = config.window_config.size,
				.resizable = config.window_config.resizable
			};

			impl::video_config c_vid_conf = {
				.api = static_cast<u32>(config.video_config.api),
				.enable_validation = config.video_config.enable_validation,
				.enable_vsync = config.video_config.enable_vsync,
				.clear_colour = config.video_config.clear_colour
			};

			impl::init_window(&c_win_conf, c_vid_conf.api);

			Resource_Manager::init(argv[0]);

			impl::init_video(&c_vid_conf);

			impl::gizmos_init();

			on_init();

			u64 now = impl::get_timer(), last = impl::get_timer();
			f64 ts = 0.0;

			bool r = true;

			while (impl::window_open() && r) {
				impl::table_lookup_count = 0;
				impl::heap_allocation_count = 0;

				impl::update_events();

				Video::begin();
				on_update(ts);
				Video::end();

				now = impl::get_timer();
				ts = (f64)(now - last) / (f64)impl::get_timer_frequency();
				last = now;
			}

			on_deinit();

			impl::gizmos_deinit();

			Resource_Manager::deinit();

			impl::deinit_video();
			impl::deinit_window();

			impl::leak_check();

			impl::alloc_deinit();
		}

		virtual ~App_Base() {}
	};
};
