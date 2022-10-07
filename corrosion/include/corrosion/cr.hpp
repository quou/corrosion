/* Corrosion C++ header. */

#include <cmath>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

	const static f32 pi_f = 3.14159265358f;
	const static f64 pi_d = 3.14159265358979323846;

#define cr_decl_v2_cpp(n_, T, b_) \
	struct n_ : public b_ { \
		n_() : b_(b_ { 0, 0 }) {} \
		n_(const b_& v) : b_(v) {} \
		n_(T xy)     : b_(b_ { xy, xy }) {} \
		n_(T x, T y) : b_(b_ { x,  y }) {} \
		\
		n_ operator+(const n_& rhs) const { return n_(x + rhs.x, y + rhs.y); } \
		n_ operator-(const n_& rhs) const { return n_(x - rhs.x, y - rhs.y); } \
		n_ operator*(const n_& rhs) const { return n_(x * rhs.x, y * rhs.y); } \
		n_ operator/(const n_& rhs) const { return n_(x / rhs.x, y / rhs.y); } \
		n_ operator+(T rhs) const { return n_(x + rhs, y + rhs); } \
		n_ operator-(T rhs) const { return n_(x - rhs, y - rhs); } \
		n_ operator*(T rhs) const { return n_(x * rhs, y * rhs); } \
		n_ operator/(T rhs) const { return n_(x / rhs, y / rhs); } \
		T dot(const n_& rhs) const { \
			return x * rhs.x + y * rhs.y; \
		} \
		n_ normalised() const { \
			const T l = mag(); \
			return n_(x / l, y / l); \
		} \
		T mag_sqrd() const { \
			return dot(*this); \
		} \
		T mag() const { \
			return std::sqrt(mag_sqrd()); \
		} \
	};

#define cr_decl_v3_cpp(n_, T, b_, vec2_t) \
	struct n_ : public b_ { \
		n_(const b_& v)   : b_(v) {} \
		n_()              : b_(b_ { 0, 0, 0 }) {} \
		n_(T xyz)         : b_(b_ { xyz, xyz, xyz }) {}  \
		n_(T x, T y, T z) : b_(b_ { x,   y,   z }) {}  \
		n_(const vec2_t& xy, T z) : b_(b_ { xy.x, xy.y, z }) {} \
		n_(T x, const vec2_t& yz) : b_(b_ { x, yz.x, yz.y }) {} \
		\
		n_ operator+(const n_& rhs) const { return n_(x + rhs.x, y + rhs.y, z + rhs.z); } \
		n_ operator-(const n_& rhs) const { return n_(x - rhs.x, y - rhs.y, z - rhs.z); } \
		n_ operator*(const n_& rhs) const { return n_(x * rhs.x, y * rhs.y, z * rhs.z); } \
		n_ operator/(const n_& rhs) const { return n_(x / rhs.x, y / rhs.y, z / rhs.z); } \
		n_ operator+(T rhs) const { return n_(x + rhs, y + rhs, z + rhs); } \
		n_ operator-(T rhs) const { return n_(x - rhs, y - rhs, z - rhs); } \
		n_ operator*(T rhs) const { return n_(x * rhs, y * rhs, z * rhs); } \
		n_ operator/(T rhs) const { return n_(x / rhs, y / rhs, z / rhs); } \
		T dot(const n_& rhs) const { \
			return x * rhs.x + y * rhs.y + z * rhs.z; \
		} \
		n_ cross(const n_& rhs) const { \
			return n_( \
				y * rhs.z - z * rhs.y, \
				z * rhs.x - x * rhs.z, \
				x * rhs.y - y * rhs.x  \
			); \
		} \
		n_ normalised() const { \
			const T l = mag(); \
			return n_(x / l, y / l, z / l); \
		} \
		T mag_sqrd() const { \
			return dot(*this); \
		} \
		T mag() const { \
			return std::sqrt(mag_sqrd()); \
		} \
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
		\
		n_ operator+(const n_& rhs) const { return n_(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); } \
		n_ operator-(const n_& rhs) const { return n_(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); } \
		n_ operator*(const n_& rhs) const { return n_(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); } \
		n_ operator/(const n_& rhs) const { return n_(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); } \
		n_ operator+(T rhs) const { return n_(x + rhs, y + rhs, z + rhs, w + rhs); } \
		n_ operator-(T rhs) const { return n_(x - rhs, y - rhs, z - rhs, w - rhs); } \
		n_ operator*(T rhs) const { return n_(x * rhs, y * rhs, z * rhs, w * rhs); } \
		n_ operator/(T rhs) const { return n_(x / rhs, y / rhs, z / rhs, w / rhs); } \
		T dot(const n_& rhs) const { \
			return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w; \
		} \
		n_ normalised() const { \
			const T l = mag(); \
			return n_(x / l, y / l, z / l, w / l); \
		} \
		T mag_sqrd() const { \
			return dot(*this); \
		} \
		T mag() const { \
			return std::sqrt(mag_sqrd()); \
		} \
	};

	template <typename T>
	T to_rad(T v) {
		return v * (static_cast<T>(pi_d) / static_cast<T>(180));
	}

	template <typename T>
	T to_deg(T v) {
		return v * (static_cast<T>(180) / static_cast<T>(pi_d));
	}

	template <typename A, typename B, typename T>
	A lerp(A a, B b, T t) {
		return a + t * (b - a);
	}

	template <typename V, typename Min, typename Max>
	V clamp(V v, Min min, Max max) {
		return cr_max(min, cr_min(max, v));
	}

	template <typename V, typename Min, typename Max>
	V map(V v, Min min1, Max max1, Min min2, Max max2) {
		return min2 + (v - min1) * (max2 - min2) / (max1 - min1);
	}

	template <typename V>
	V saturate(V v) {
		return clamp(v, static_cast<V>(0), static_cast<V>(1));
	}

	cr_decl_v2_cpp(v2f, f32, impl::v2f);
	cr_decl_v2_cpp(v2i, i32, impl::v2i);
	cr_decl_v2_cpp(v2u, u32, impl::v2u);
	cr_decl_v3_cpp(v3f, f32, impl::v3f, v2f);
	cr_decl_v3_cpp(v3i, i32, impl::v3i, v2i);
	cr_decl_v3_cpp(v3u, u32, impl::v3u, v2u);
	cr_decl_v4_cpp(v4f, f32, impl::v4f, v2f, v3f);
	cr_decl_v4_cpp(v4i, i32, impl::v4i, v2i, v3i);
	cr_decl_v4_cpp(v4u, u32, impl::v4u, v2u, v3u);

	struct quat : public impl::quat {
		quat() : impl::quat(impl::quat { 0.0f, 0.0f, 0.0f, 0.0f }) {}
		quat(const v3f& a, f32 s) : impl::quat(impl::quat { a.x, a.y, a.z, s }) {}
		quat(f32 x, f32 y, f32 z, f32 w) : impl::quat(impl::quat { x, y, z, w }) {}

		static quat identity() {
			return quat(v3f(0.0f, 0.0f, 0.0f), 1.0f);
		}

		quat operator*(const quat& rhs) {
			return quat(
		 		 x * rhs.w +
		 		 y * rhs.z -
		 		 z * rhs.y +
		 		 w * rhs.x,

				-x * rhs.z +
		 		 y * rhs.w +
		 		 z * rhs.x +
		 		 w * rhs.y,

		 		 x * rhs.y -
		 		 y * rhs.x +
		 		 z * rhs.w +
		 		 w * rhs.z,

				-x * rhs.x -
		 		 y * rhs.y -
		 		 z * rhs.z +
		 		 w * rhs.w
			).normalised();
		}

		quat operator*(f32 rhs) {
			return quat(x * rhs, y * rhs, z * rhs, w);
		}

		quat conjugate() {
			return quat(-x, -y, -z, w);
		}

		v4f to_v4f() {
			return corrosion::v4f(x, y, z, w);
		}

		f32 normal() {
			v4f v = to_v4f();
			return v.dot(v);
		}

		quat normalised() {
			return *this * (1.0f / std::sqrt(normal()));
		}

		static quat rotate(f32 angle, const v3f& axis) {
			quat q;

			angle = to_rad(angle);

			f32 half_angle = 0.5f * angle;
			f32 sin_ha = sinf(half_angle);

			q.w = std::cos(half_angle);
			q.x = sin_ha * axis.x;
			q.y = sin_ha * axis.y;
			q.z = sin_ha * axis.z;

			return q;
		}

		static quat euler(const v3f& a) {
			quat p = rotate(a.x, v3f(1.0f, 0.0f, 0.0f));
			quat y = rotate(a.y, v3f(0.0f, 1.0f, 0.0f));
			quat r = rotate(a.z, v3f(0.0f, 0.0f, 1.0f));

			return p * y * r;
		}
	};

	struct m4f : public impl::m4f {
		m4f(const impl::m4f& m) : impl::m4f(m) {}
		m4f(f32 d) : impl::m4f(impl::m4f{ {
			{ d,    0.0f, 0.0f, 0.0f },
			{ 0.0f, d,    0.0f, 0.0f },
			{ 0.0f, 0.0f, d,    0.0f },
			{ 0.0f, 0.0f, 0.0f, d    },
		} }) {}
		m4f() : impl::m4f(impl::m4f{ {
			{ 1.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f, 1.0f },
		} }) {}

		m4f(v4f r0, v4f r1, v4f r2, v4f r3) : impl::m4f(impl::m4f{ {
			{ r0.x, r0.y, r0.z, r0.w },
			{ r1.x, r1.y, r1.z, r1.w },
			{ r2.x, r2.y, r2.z, r2.w },
			{ r3.x, r3.y, r3.z, r3.w },
		} }) {}

		static m4f identity() {
			return m4f(1.0f);
		}

		static m4f screenspace(f32 hw, f32 hh) {
			return m4f(
				v4f(hw,   0.0f, 0.0f, hw),
				v4f(0.0f, -hh,  0.0f, hh),
				v4f(0.0f, 0.0f, 1.0f, 0.0f),
				v4f(0.0f, 0.0f, 0.0f, 1.0f)
			);
		}

		m4f operator*(const m4f& rhs) {
			return m4f(
				v4f(
					m[0][0] * rhs.m[0][0] + m[1][0] * rhs.m[0][1] + m[2][0] * rhs.m[0][2] + m[3][0] * rhs.m[0][3],
					m[0][1] * rhs.m[0][0] + m[1][1] * rhs.m[0][1] + m[2][1] * rhs.m[0][2] + m[3][1] * rhs.m[0][3],
					m[0][2] * rhs.m[0][0] + m[1][2] * rhs.m[0][1] + m[2][2] * rhs.m[0][2] + m[3][2] * rhs.m[0][3],
					m[0][3] * rhs.m[0][0] + m[1][3] * rhs.m[0][1] + m[2][3] * rhs.m[0][2] + m[3][3] * rhs.m[0][3]
				),
				v4f(
					m[0][0] * rhs.m[1][0] + m[1][0] * rhs.m[1][1] + m[2][0] * rhs.m[1][2] + m[3][0] * rhs.m[1][3],
					m[0][1] * rhs.m[1][0] + m[1][1] * rhs.m[1][1] + m[2][1] * rhs.m[1][2] + m[3][1] * rhs.m[1][3],
					m[0][2] * rhs.m[1][0] + m[1][2] * rhs.m[1][1] + m[2][2] * rhs.m[1][2] + m[3][2] * rhs.m[1][3],
					m[0][3] * rhs.m[1][0] + m[1][3] * rhs.m[1][1] + m[2][3] * rhs.m[1][2] + m[3][3] * rhs.m[1][3] 
				),
				v4f(
					m[0][0] * rhs.m[2][0] + m[1][0] * rhs.m[2][1] + m[2][0] * rhs.m[2][2] + m[3][0] * rhs.m[2][3],
					m[0][1] * rhs.m[2][0] + m[1][1] * rhs.m[2][1] + m[2][1] * rhs.m[2][2] + m[3][1] * rhs.m[2][3],
					m[0][2] * rhs.m[2][0] + m[1][2] * rhs.m[2][1] + m[2][2] * rhs.m[2][2] + m[3][2] * rhs.m[2][3],
					m[0][3] * rhs.m[2][0] + m[1][3] * rhs.m[2][1] + m[2][3] * rhs.m[2][2] + m[3][3] * rhs.m[2][3] 
				),
				v4f(
					m[0][0] * rhs.m[3][0] + m[1][0] * rhs.m[3][1] + m[2][0] * rhs.m[3][2] + m[3][0] * rhs.m[3][3],
					m[0][1] * rhs.m[3][0] + m[1][1] * rhs.m[3][1] + m[2][1] * rhs.m[3][2] + m[3][1] * rhs.m[3][3],
					m[0][2] * rhs.m[3][0] + m[1][2] * rhs.m[3][1] + m[2][2] * rhs.m[3][2] + m[3][2] * rhs.m[3][3],
					m[0][3] * rhs.m[3][0] + m[1][3] * rhs.m[3][1] + m[2][3] * rhs.m[3][2] + m[3][3] * rhs.m[3][3] 
				)
			);
		}

		v4f operator*(const v4f& rhs) {
			return v4f(
				m[0][0] * rhs.x + m[1][0] * rhs.y + m[2][0] * rhs.z + m[3][0] * rhs.w,
				m[0][1] * rhs.x + m[1][1] * rhs.y + m[2][1] * rhs.z + m[3][1] * rhs.w,
				m[0][2] * rhs.x + m[1][2] * rhs.y + m[2][2] * rhs.z + m[3][2] * rhs.w,
				m[0][3] * rhs.x + m[1][3] * rhs.y + m[2][3] * rhs.z + m[3][3] * rhs.w);
		}

		static m4f translation(const v3f& t) {
			return m4f(
				v4f(1.0f, 0.0f, 0.0f, 0.0f),
				v4f(0.0f, 1.0f, 0.0f, 0.0f),
				v4f(0.0f, 0.0f, 1.0f, 0.0f),
				v4f(t.x,  t.y,  t.z, 1.0f)
			);
		}

		static m4f scale(const v3f& s) {
			return m4f(
				v4f(s.x,  0.0f, 0.0f, 0.0f),
				v4f(0.0f, s.y,  0.0f, 0.0f),
				v4f(0.0f, 0.0f, s.z,  0.0f),
				v4f(0.0f, 0.0f, 0.0f, 1.0f)
			);
		}

		static m4f rotation(const quat& q) {
			m4f r;

			f32 qx, qy, qz, qw, qx2, qy2, qz2, qxqx2, qyqy2, qzqz2, qxqy2, qyqz2, qzqw2, qxqz2, qyqw2, qxqw2;
			qx = q.x;
			qy = q.y;
			qz = q.z;
			qw = q.w;
			qx2 = (qx + qx);
			qy2 = (qy + qy);
			qz2 = (qz + qz);
			qxqx2 = (qx * qx2);
			qxqy2 = (qx * qy2);
			qxqz2 = (qx * qz2);
			qxqw2 = (qw * qx2);
			qyqy2 = (qy * qy2);
			qyqz2 = (qy * qz2);
			qyqw2 = (qw * qy2);
			qzqz2 = (qz * qz2);
			qzqw2 = (qw * qz2);

			r.m[0][0] = ((1.0f - qyqy2) - qzqz2);
			r.m[0][1] = qxqy2 - qzqw2;
			r.m[0][2] = qxqz2 + qyqw2;
			r.m[0][3] = 0.0f;

			r.m[1][0] = qxqy2 + qzqw2;
			r.m[1][1] = (1.0f - qxqx2) - qzqz2;
			r.m[1][2] = qyqz2 - qxqw2;
			r.m[1][3] = 0.0f;

			r.m[2][0] = qxqz2 - qyqw2;
			r.m[2][1] = qyqz2 + qxqw2;
			r.m[2][2] = (1.0f - qxqx2) - qyqy2;
			r.m[2][3] = 0.0f;

			return r;
		}

		static m4f lookat(const v3f& c, const v3f& o, v3f u) {
			m4f r;

			const v3f f = (o - c).normalised();
			u = u.normalised();
			const v3f s = f.cross(u).normalised();
			u = s.cross(f);

			r.m[0][0] = s.x;
			r.m[1][0] = s.y;
			r.m[2][0] = s.z;
			r.m[0][1] = u.x;
			r.m[1][1] = u.y;
			r.m[2][1] = u.z;
			r.m[0][2] = -f.x;
			r.m[1][2] = -f.y;
			r.m[2][2] = -f.z;
			r.m[3][0] = -s.dot(c);
			r.m[3][1] = -u.dot(c);
			r.m[3][2] =  f.dot(c);

			return r;
		}

		static m4f ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
			m4f res;

			res.m[0][0] = 2.0f / (r - l);
			res.m[1][1] = 2.0f / (t - b);
			res.m[2][2] = 2.0f / (n - f);

			res.m[3][0] = (l + r) / (l - r);
			res.m[3][1] = (b + t) / (b - t);
			res.m[3][2] = (f + n) / (f - n);

			return res;
		}

		static m4f persp(f32 fov, f32 aspect, f32 near_clip, f32 far_clip) {
			m4f r;

			const f32 q = 1.0f / std::tan(to_rad(fov) / 2.0f);
			const f32 a = q / aspect;
			const f32 b = (near_clip + far_clip) / (near_clip - far_clip);
			const f32 c = (2.0f * near_clip * far_clip) / (near_clip - far_clip);

			r.m[0][0] = a;
			r.m[1][1] = q;
			r.m[2][2] = b;
			r.m[2][3] = -1.0f;
			r.m[3][2] = c;

			return r;
		}
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

#define cr_declare_enum_flag_ops(T) \
	T operator~ (T a) { return (T)~(int)a; } \
	T operator| (T a, T b) { return (T)((int)a | (int)b); } \
	T operator& (T a, T b) { return (T)((int)a & (int)b); } \
	T operator^ (T a, T b) { return (T)((int)a ^ (int)b); } \
	T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); } \
	T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); } \
	T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

	class Texture;
	class Storage;

	class Shader {
	private:
		impl::shader* as_impl() {
			return reinterpret_cast<impl::shader*>(this);
		}

		const impl::shader* as_impl() const {
			return reinterpret_cast<const impl::shader*>(this);
		}

		friend class Pipeline;
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

	class Framebuffer {
	private:
		impl::framebuffer* as_impl() {
			return reinterpret_cast<impl::framebuffer*>(this);
		}

		const impl::framebuffer* as_impl() const {
			return reinterpret_cast<const impl::framebuffer*>(this);
		}

		friend class Pipeline;
		friend class UI;
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
			} format;

			v4f clear_colour;

			Attachment_Desc(const char* name, Type type, Format format, v4f clear_colour)
				: name(name), type(type), format(format), clear_colour(clear_colour) {}
		};

		enum class Flags {
			fit                     = impl::framebuffer_flags_fit,
			headless                = impl::framebuffer_flags_headless,
			attachments_are_storage = impl::framebuffer_flags_attachments_are_storage
		};

		template <typename... Args>
		static Framebuffer* create(Flags flags, v2i size, const std::vector<Attachment_Desc>& attachments) {
			std::vector<impl::framebuffer_attachment_desc> dc(attachments.size());

			for (usize i = 0; i < attachments.size(); i++) {
				dc.push_back(impl::framebuffer_attachment_desc {
					.name         = attachments[i].name,
					.type         = static_cast<u32>(attachments[i].type),
					.format       = static_cast<u32>(attachments[i].format),
					.clear_colour = attachments[i].clear_colour
				});
			}

			return reinterpret_cast<Framebuffer*>(impl::video.new_framebuffer(
				static_cast<u32>(flags), size,
				&dc[0], dc.size()
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

	class Texture {
	private:
		impl::texture* as_impl() {
			return reinterpret_cast<impl::texture*>(this);
		}

		const impl::texture* as_impl() const {
			return reinterpret_cast<const impl::texture*>(this);
		}

		friend class Pipeline;
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

		void copy_to(Texture& dst, v2i dst_offset, v2i src_offset, v2i region) {
			impl::video.texture_copy(dst.as_impl(), dst_offset, as_impl(), src_offset, region);
		}
		
		void copy_to(Texture& dst, v3i dst_offset, v3i src_offset, v3i region) {
			impl::video.texture_copy_3d(dst.as_impl(), dst_offset, as_impl(), src_offset, region);
		}

		void barrier(State state) {
			impl::video.texture_barrier(as_impl(), static_cast<u32>(state));
		}
	};

	class Storage {
	private:
		impl::storage* as_impl() {
			return reinterpret_cast<impl::storage*>(this);
		}

		const impl::storage* as_impl() const {
			return reinterpret_cast<const impl::storage*>(this);
		}

		friend class Pipeline;
	public:
		Storage() = delete;

		enum class Flags : u32 {
			none          = impl::storage_flags_none,
			cpu_writable  = impl::storage_flags_cpu_writable,
			cpu_readable  = impl::storage_flags_cpu_readable,
			vertex_buffer = impl::storage_flags_vertex_buffer,
			index_buffer  = impl::storage_flags_index_buffer,
			indices_16bit = impl::storage_flags_16bit_indices,
			indices_32bit = impl::storage_flags_32bit_indices
		};

		enum class Bind_As : u32 {
			vertex_buffer = impl::storage_bind_as_vertex_buffer,
			index_buffer  = impl::storage_bind_as_index_buffer
		};

		enum class State : u32 {
			compute_read       = impl::storage_state_compute_read,
			compute_write      = impl::storage_state_compute_write,
			compute_read_write = impl::storage_state_compute_read_write,
			fragment_read      = impl::storage_state_fragment_read,
			fragment_write     = impl::storage_state_fragment_write,
			vertex_read        = impl::storage_state_vertex_read,
			vertex_write       = impl::storage_state_vertex_write,
			dont_care          = impl::storage_state_dont_care
		};

		static Storage* create(Flags flags, usize size, void* initial_data = null) {
			return reinterpret_cast<Storage*>(impl::video.new_storage(static_cast<u32>(flags), size, initial_data));
		}

		void update(void* data) {
			impl::video.update_storage(as_impl(), data);
		}

		void update(void* data, usize offset, usize size) {
			impl::video.update_storage_region(as_impl(), data, offset, size);
		}

		void copy_to(Storage& dst, usize dst_offset, usize src_offset, usize size) {
			impl::video.copy_storage(dst.as_impl(), dst_offset, as_impl(), src_offset, size);
		}

		void barrier(State state) {
			impl::video.storage_barrier(as_impl(), static_cast<u32>(state));
		}

		void bind_as(Bind_As as, u32 point = 0) {
			impl::video.storage_bind_as(as_impl(), static_cast<u32>(as), point);
		}

		void release() {
			impl::video.free_storage(as_impl());
		}
	};

	class Vertex_Buffer {
	private:
		impl::vertex_buffer* as_impl() {
			return reinterpret_cast<impl::vertex_buffer*>(this);
		}

		const impl::vertex_buffer* as_impl() const {
			return reinterpret_cast<const impl::vertex_buffer*>(this);
		}
	public:
		Vertex_Buffer() = delete;

		enum class Flags : u32 {
			none         = impl::vertex_buffer_flags_none,
			dynamic      = impl::vertex_buffer_flags_dynamic,
			transferable = impl::vertex_buffer_flags_transferable
		};

		static Vertex_Buffer* create(void* data, usize data_size, Flags flags) {
			return reinterpret_cast<Vertex_Buffer*>(impl::video.new_vertex_buffer(
				data, data_size, static_cast<u32>(flags)
			));
		}

		void release() {
			impl::video.free_vertex_buffer(as_impl());
		}

		void bind(u32 point) {
			impl::video.bind_vertex_buffer(as_impl(), point);
		}

		void update(const void* data, usize size, usize offset) {
			impl::video.update_vertex_buffer(as_impl(), data, size, offset);
		}

		void copy_to(Vertex_Buffer& dst, usize dst_offset, usize src_offset, usize size) {
			impl::video.copy_vertex_buffer(dst.as_impl(), dst_offset, as_impl(), src_offset, size);
		}
	};

	class Index_Buffer {
	private:
		impl::index_buffer* as_impl() {
			return reinterpret_cast<impl::index_buffer*>(this);
		}

		const impl::index_buffer* as_impl() const {
			return reinterpret_cast<const impl::index_buffer*>(this);
		}
	public:
		Index_Buffer() = delete;

		enum class Flags : u32 {
			none     = impl::index_buffer_flags_none,
			data_u16 = impl::index_buffer_flags_u16,
			data_u32 = impl::index_buffer_flags_u32
		};

		static Index_Buffer* create(void* data, usize data_size, Flags flags) {
			return reinterpret_cast<Index_Buffer*>(impl::video.new_index_buffer(
				data, data_size, static_cast<u32>(flags)
			));
		}

		void release() {
			impl::video.free_index_buffer(as_impl());
		}

		void bind() {
			impl::video.bind_index_buffer(as_impl());
		}
	};

	struct Descriptor_Resource {
		enum class Type : u32 {
			uniform_buffer  = impl::pipeline_resource_uniform_buffer,
			texture         = impl::pipeline_resource_texture,
			texture_storage = impl::pipeline_resource_texture_storage,
			storage         = impl::pipeline_resource_storage
		} type;

		struct Uniform {
			usize size;
		};

		union {
			Uniform uniform;
			Texture* texture;
			Storage* storage;
		};

		Descriptor_Resource(Uniform uniform) : type(Type::uniform_buffer), uniform(uniform) {}
		Descriptor_Resource(Texture* texture, bool is_storage = false)
			: type(is_storage ? Type::texture_storage : Type::texture), texture(texture) {}
		Descriptor_Resource(Storage* storage) : type(Type::storage), storage(storage) {}
	};

	enum class Pipeline_Stage : u32 {
		vertex   = impl::pipeline_stage_vertex,
		fragment = impl::pipeline_stage_fragment,
		compute  = impl::pipeline_stage_compute
	};

	struct Descriptor {
		const char* name;
		u32 binding;
		Pipeline_Stage stage;
		Descriptor_Resource resource;

		Descriptor(const char* name, u32 binding, Pipeline_Stage stage, Descriptor_Resource resource)
			: name(name), binding(binding), stage(stage), resource(std::move(resource)) { }
	};

	struct Descriptor_Set {
		const char* name;
		std::vector<Descriptor> descriptors;

		Descriptor_Set(const char* name, std::vector<Descriptor> descriptors) :
			name(name), descriptors(std::move(descriptors)) {}
	};

	struct Attribute {
		const char* name;
		u32 location;
		enum class Type : u32 {
			float_ = impl::pipeline_attribute_float,
			vec2   = impl::pipeline_attribute_vec2,
			vec3   = impl::pipeline_attribute_vec3,
			vec4   = impl::pipeline_attribute_vec4,
		} type;
		usize offset;

		Attribute(const char* name, u32 location, Type type, usize offset) :
			name(name), location(location), type(type), offset(offset) {}
	};

	struct Attribute_Binding {
		u32 binding;
		
		enum class Rate : u32 {
			per_vertex   = impl::pipeline_attribute_rate_per_vertex,
			per_instance = impl::pipeline_attribute_rate_per_instance,
		} rate;

		usize stride;

		std::vector<Attribute> attributes;

		Attribute_Binding(u32 binding, Rate rate, usize stride, std::vector<Attribute> attributes) :
			binding(binding), rate(rate), stride(stride), attributes(std::move(attributes)) {}
	};

	class Pipeline {
	private:
		impl::pipeline* as_impl() {
			return reinterpret_cast<impl::pipeline*>(this);
		}
	public:
		Pipeline() = delete;

		enum class Flags : u32 {
			none            = impl::pipeline_flags_none,
			depth_test      = impl::pipeline_flags_depth_test,
			cull_back_face  = impl::pipeline_flags_cull_back_face,
			cull_front_face = impl::pipeline_flags_cull_front_face,
			blend           = impl::pipeline_flags_blend,
			dynamic_scissor = impl::pipeline_flags_dynamic_scissor,
			draw_lines      = impl::pipeline_flags_draw_lines,
			draw_line_strip = impl::pipeline_flags_draw_line_strip,
			draw_tris       = impl::pipeline_flags_draw_tris,
			draw_points     = impl::pipeline_flags_draw_points,
			compute         = impl::pipeline_flags_compute
		};

		static Pipeline* create(Flags flags, const Shader& shader, const Framebuffer& framebuffer,
			const std::vector<Attribute_Binding> attrib_bindings, const std::vector<Descriptor_Set> descriptor_sets) {
			std::vector<impl::pipeline_attribute_binding> cattrib_bindings;
			cattrib_bindings.reserve(attrib_bindings.size());
			std::vector<impl::pipeline_attribute> cattribs;

			std::vector<impl::pipeline_descriptor_set> cdescriptor_sets;
			cdescriptor_sets.reserve(descriptor_sets.size());
			std::vector<impl::pipeline_descriptor> cdescriptors;

			for (const auto& attrib_binding : attrib_bindings) {
				impl::pipeline_attribute_binding binding{};
				binding.binding = attrib_binding.binding;
				binding.rate = static_cast<u32>(attrib_binding.rate);
				binding.stride = attrib_binding.stride;

				usize offset = cattribs.size();

				for (const auto& attrib : attrib_binding.attributes) {
					cattribs.push_back(impl::pipeline_attribute {
						.name = attrib.name,
						.location = attrib.location,
						.type = static_cast<u32>(attrib.type),
						.offset = attrib.offset
					});
				}

				binding.attributes.count = cattribs.size();
				binding.attributes.attributes = &cattribs[offset];

				cattrib_bindings.push_back(std::move(binding));
			}

			for (const auto& set : descriptor_sets) {
				impl::pipeline_descriptor_set cset{};

				usize offset = cdescriptors.size();

				for (const auto& desc : set.descriptors) {
					impl::pipeline_descriptor cdesc{};

					cdesc.resource.type = static_cast<u32>(desc.resource.type);
					cdesc.name = desc.name;
					cdesc.binding = desc.binding;
					cdesc.stage = static_cast<u32>(desc.stage);

					switch (desc.resource.type) {
					case Descriptor_Resource::Type::uniform_buffer:
						cdesc.resource.uniform.size = desc.resource.uniform.size;
						break;
					case Descriptor_Resource::Type::texture:
					case Descriptor_Resource::Type::texture_storage:
						cdesc.resource.texture = desc.resource.texture->as_impl();
						break;
					case Descriptor_Resource::Type::storage:
						cdesc.resource.storage = desc.resource.storage->as_impl();
						break;
					default: break;
					}

					cdescriptors.push_back(std::move(cdesc));
				}

				cset.name = set.name;
				cset.count = cdescriptors.size();
				cset.descriptors = &cdescriptors[offset];

				cdescriptor_sets.push_back(std::move(cset));
			}
			
			return reinterpret_cast<Pipeline*>(impl::video.new_pipeline(static_cast<u32>(flags), shader.as_impl(), framebuffer.as_impl(),
				impl::pipeline_attribute_bindings { &cattrib_bindings[0], cattrib_bindings.size() },
				impl::pipeline_descriptor_sets { &cdescriptor_sets[0], cdescriptor_sets.size() }));
		}

		void release() {
			impl::video.free_pipeline(as_impl());
		}

		void begin() {
			impl::video.begin_pipeline(as_impl());
		}

		void end() {
			impl::video.end_pipeline(as_impl());
		}

		void recreate() {
			impl::video.recreate_pipeline(as_impl());
		}

		void update_uniform(const char* set, const char* descriptor, const void* data) {
			impl::video.update_pipeline_uniform(as_impl(), set, descriptor, data);
		}

		void bind_descriptor_set(const char* set, usize target) {
			impl::video.bind_pipeline_descriptor_set(as_impl(), set, target);
		}

		void change_shader(const Shader& shader) {
			impl::video.pipeline_change_shader(as_impl(), shader.as_impl());
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

		enum class Features : u32 {
			base    = impl::video_feature_base,
			compute = impl::video_feature_compute,
			storage = impl::video_feature_storage,
			barrier = impl::video_feature_barrier
		};

		static Framebuffer& get_default_fb() {
			return *reinterpret_cast<Framebuffer*>(impl::video.get_default_fb());
		}

		static void begin(bool present = true) {
			impl::video.begin(present);
		}

		static void end(bool present = true) {
			impl::video.end(present);
		}

		static void draw(usize count, usize offset = 0, usize instances = 1) {
			impl::video.draw(count, offset, instances);
		}

		static void draw_index(usize count, usize offset = 0, usize instances = 1) {
			impl::video.draw(count, offset, instances);
		}

		static void set_scissor(v4i rect) {
			impl::video.set_scissor(rect);
		}

		static void invoke_compute(v3u group_count) {
			impl::video.invoke_compute(group_count);
		}

		static u32 get_draw_call_count() {
			return impl::video.get_draw_call_count();
		}

		static const char* get_api_name() {
			return impl::video.get_api_name();
		}

		static Features query_features() {
			return static_cast<Features>(impl::video.query_features());
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

		static Resource load(const char* type, const char* filename, void* udata = null) {
			auto ir = impl::res_load(type, filename, udata);

			return Resource(ir.payload, ir.id);
		}

		static void unload(const Resource& r) {
			impl::res_unload(&r);
		}

		static Shader* load_shader(const char* filename, Resource* r = null) {
			return reinterpret_cast<Shader*>(impl::load_shader(filename, r));
		}

		static Texture* load_texture(const char* filename, Texture::Flags flags, Resource* r = null) {
			return reinterpret_cast<Texture*>(impl::load_texture(filename, static_cast<u32>(flags), r));
		}
		
		static Font* load_font(const char* filename, i32 size, Resource* r = null) {
			return reinterpret_cast<Font*>(impl::load_font(filename, size, r));
		}

		static void init(const char* argv0) {
			impl::res_init(argv0);
		}

		static void deinit() {
			impl::res_deinit();
		}
	};

	class Camera : public impl::camera {
	private:
	public:
		Camera() : impl::camera(impl::camera {
			.position = { 0.0f, 0.0f, 0.0f },
			.rotation = { 0.0f, 0.0f, 0.0f },
			.fov = 70.0f,
			.near_plane = 0.1f,
			.far_plane = 100.0f
		}) {}

		Camera(v3f position, f32 fov, f32 near_plane = 0.1f, f32 far_plane = 100.0f) : impl::camera(impl::camera {
			.position = position,
			.rotation = { 0.0f, 0.0f, 0.0f },
			.fov = fov,
			.near_plane = near_plane,
			.far_plane = far_plane
		}) {}

		m4f get_view() {
			return impl::get_camera_view(this);
		}

		m4f get_projection(f32 aspect) {
			return impl::get_camera_projection(this, aspect);
		}
	};

	/* Declare operator overloads for all of the flag enums. */
	cr_declare_enum_flag_ops(Framebuffer::Flags);
	cr_declare_enum_flag_ops(Texture::Flags);
	cr_declare_enum_flag_ops(Storage::Flags);
	cr_declare_enum_flag_ops(Vertex_Buffer::Flags);
	cr_declare_enum_flag_ops(Index_Buffer::Flags);
	cr_declare_enum_flag_ops(Pipeline::Flags);

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

	class UI {
	private:
		impl::ui* as_impl() {
			return reinterpret_cast<impl::ui*>(this);
		}

		const impl::ui* as_impl() const {
			return reinterpret_cast<const impl::ui*>(this);
		}
	public:
		static UI* create(const Framebuffer& framebuffer) {
			return reinterpret_cast<UI*>(impl::new_ui(framebuffer.as_impl()));
		}

		void release() {
			impl::free_ui(as_impl());
		}

		void begin() {
			impl::ui_begin(as_impl());
		}

		void end() {
			impl::ui_end(as_impl());
		}

		void draw_rect(v2f position, f32 z, v2f dimensions, v4f colour, f32 radius) {
			impl::ui_draw_rect(as_impl(), position, z, dimensions, colour, radius);
		}
	};
};
