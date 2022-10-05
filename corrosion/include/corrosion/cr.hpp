/* Corrosion C++ header. */

#include <cstdint>
#include <ctime>
#include <utility>
#include <string>

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
		n_(const b_& v) : b_(v) {} \
		n_(T xy)     : b_(b_ { xy, xy }) {} \
		n_(T x, T y) : b_(b_ { x,  y }) {} \
	};

#define cr_decl_v3_cpp(n_, T, b_, vec2_t) \
	struct n_ : public b_ { \
		n_(const b_& v)   : b_(v) {} \
		n_(T xyz)         : b_(b_ { xyz, xyz, xyz }) {}  \
		n_(T x, T y, T z) : b_(b_ { x,   y,   z }) {}  \
		n_(const vec2_t& xy, T z) : b_(b_ { xy.x, xy.y, z }) {} \
		n_(T x, const vec2_t& yz) : b_(b_ { x, yz.x, yz.y }) {} \
	};

#define cr_decl_v4_cpp(n_, T, b_, vec2_t, vec3_t) \
	struct n_ : public b_ { \
		n_(const b_& v)        : b_(v) {} \
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

	class Framebuffer {
	private:
		impl::framebuffer* ih;

		Framebuffer(impl::framebuffer* ih) : ih(ih) {}

		friend class App_Base;
	public:
		void begin() {
			impl::video.begin_framebuffer(ih);
		}

		void end() {
			impl::video.end_framebuffer(ih);
		}

		v2i get_size() {
			return impl::video.get_framebuffer_size(ih);
		}

		void resize(v2i new_size) {
			impl::video.resize_framebuffer(ih, new_size);
		}
	};

	class Video {
	private:
		inline static Framebuffer* default_fb;

		friend class App_Base;
	public:
		enum class API {
			none   = impl::video_api_none,
			vulkan = impl::video_api_vulkan,
			opengl = impl::video_api_opengl
		};

		static Framebuffer* get_default_fb() {
			return default_fb;
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

			impl::res_init(argv[0]);

			impl::init_video(&c_vid_conf);

			impl::gizmos_init();

			on_init();

			u64 now = impl::get_timer(), last = impl::get_timer();
			f64 ts = 0.0;

			bool r = true;

			Video::default_fb = new Framebuffer(null);

			while (impl::window_open() && r) {
				impl::table_lookup_count = 0;
				impl::heap_allocation_count = 0;

				impl::update_events();

				impl::video.begin(true);

				Video::default_fb->ih = impl::video.get_default_fb();

				on_update(ts);
				impl::video.end(true);

				now = impl::get_timer();
				ts = (f64)(now - last) / (f64)impl::get_timer_frequency();
				last = now;
			}

			delete Video::default_fb;

			on_deinit();

			impl::gizmos_deinit();

			impl::res_deinit();

			impl::deinit_video();
			impl::deinit_window();

			impl::leak_check();

			impl::alloc_deinit();
		}

		virtual ~App_Base() {}
	};
};