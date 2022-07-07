#pragma once

#include <math.h>

#include "common.h"

#define min(a_, b_) ((a_) < (b_) ? (a_) : (b_))
#define max(a_, b_) ((a_) > (b_) ? (a_) : (b_))

#define v2(t_) struct { t_ x; t_ y; }
#define v3(t_) struct { t_ x; t_ y; t_ z; }
#define v4(t_) struct { t_ x; t_ y; t_ z; t_ w; }

#define m4(t_) struct { t_ m[4][4]; }

typedef v2(f32) v2f;
typedef v2(i32) v2i;
typedef v2(u32) v2u;
typedef v3(f32) v3f;
typedef v3(i32) v3i;
typedef v3(u32) v3u;
typedef v4(f32) v4f;
typedef v4(i32) v4i;
typedef v4(u32) v4u;

typedef m4(f32) m4f;

#define pi_f 3.14159265358f
#define pi_d 3.14159265358979323846

#define to_rad(v_) _Generic((v_), \
		f32: (v_ * (pi_f / 180.0f)), \
		f64: (v_ * (pi_d / 180.0))  \
	)

#define to_deg(v_) _Generic((v_), \
		f32: (v_ * (180.0f / pi_f)), \
		f64: (v_ * (180.0  / pi_d))  \
	)

#define lerp(a_, b_, t_) ((a_) + (t_) * ((b_) - (a_)))
#define clamp(v_, min_, max_) ((v_) < (min_) ? (min_) : (v_) > (max_) ? (max_) : (v_))
#define map(v_, min1_, max1_, min2_, max2_) ((min2_) + ((v_) - (min1_)) * ((max2_) - (min2_)) / ((max1_) - (min1_)))

/* make_v */
#define make_v2(t_, x_, y_)         ((t_) { x_, y_ })
#define make_v3(t_, x_, y_, z_)     ((t_) { x_, y_, z_ })
#define make_v4(t_, x_, y_, z_, w_) ((t_) { x_, y_, z_, w_ })
#define make_v2f(x_, y_)         make_v2(v2f, (f32)x_, (f32)y_)
#define make_v2i(x_, y_)         make_v2(v2i, (i32)x_, (i32)y_)
#define make_v2u(x_, y_)         make_v2(v2u, (u32)x_, (u32)y_)
#define make_v3f(x_, y_, z_)     make_v3(v3f, (f32)x_, (f32)y_, (f32)z_)
#define make_v3i(x_, y_, z_)     make_v3(v3i, (i32)x_, (i32)y_, (i32)z_)
#define make_v3u(x_, y_, z_)     make_v3(v3u, (u32)x_, (u32)y_, (u32)z_)
#define make_v4f(x_, y_, z_, w_) make_v4(v4f, (f32)x_, (f32)y_, (f32)z_, (f32)w_)
#define make_v4i(x_, y_, z_, w_) make_v4(v4i, (i32)x_, (i32)y_, (i32)z_, (i32)w_)
#define make_v4u(x_, y_, z_, w_) make_v4(v4u, (u32)x_, (u32)y_, (u32)z_, (u32)w_)

#define make_rgb(rgb_) \
	make_v3f( \
		(f32)((((u32)(rgb_)) >> 16) & 0xff) / 255.0f, \
		(f32)((((u32)(rgb_)) >> 8)  & 0xff) / 255.0f, \
		(f32)(((u32)(rgb_))         & 0xff) / 255.0f)

#define make_rgba(rgb_, a_) \
	make_v4f( \
		(f32)((((u32)(rgb_)) >> 16) & 0xff) / 255.0f, \
		(f32)((((u32)(rgb_)) >> 8)  & 0xff) / 255.0f, \
		(f32)(((u32)(rgb_))         & 0xff) / 255.0f, (f32)a_ / 255.0f)

/* v_add */
#define _v2_add(t_, a_, b_) ((t_) { (a_).x + (b_).x, (a_).y + (b_).y })
#define _v3_add(t_, a_, b_) ((t_) { (a_).x + (b_).x, (a_).y + (b_).y, (a_).z + (b_).z })
#define _v4_add(t_, a_, b_) ((t_) { (a_).x + (b_).x, (a_).y + (b_).y, (a_).z + (b_).z, (a_).w + (b_).w })

#define v2f_add(a_, b_) _v2_add(v2f, a_, b_)
#define v2i_add(a_, b_) _v2_add(v2i, a_, b_)
#define v2u_add(a_, b_) _v2_add(v2u, a_, b_)
#define v3f_add(a_, b_) _v3_add(v3f, a_, b_)
#define v3i_add(a_, b_) _v3_add(v3i, a_, b_)
#define v3u_add(a_, b_) _v3_add(v3u, a_, b_)
#define v4f_add(a_, b_) _v4_add(v4f, a_, b_)
#define v4i_add(a_, b_) _v4_add(v4i, a_, b_)
#define v4u_add(a_, b_) _v4_add(v4u, a_, b_)

/* v_sub */
#define _v2_sub(t_, a_, b_) ((t_) { (a_).x - (b_).x, (a_).y - (b_).y })
#define _v3_sub(t_, a_, b_) ((t_) { (a_).x - (b_).x, (a_).y - (b_).y, (a_).z - (b_).z })
#define _v4_sub(t_, a_, b_) ((t_) { (a_).x - (b_).x, (a_).y - (b_).y, (a_).z - (b_).z, (a_).w - (b_).w })

#define v2f_sub(a_, b_) _v2_sub(v2f, a_, b_)
#define v2i_sub(a_, b_) _v2_sub(v2i, a_, b_)
#define v2u_sub(a_, b_) _v2_sub(v2u, a_, b_)
#define v3f_sub(a_, b_) _v3_sub(v3f, a_, b_)
#define v3i_sub(a_, b_) _v3_sub(v3i, a_, b_)
#define v3u_sub(a_, b_) _v3_sub(v3u, a_, b_)
#define v4f_sub(a_, b_) _v4_sub(v4f, a_, b_)
#define v4i_sub(a_, b_) _v4_sub(v4i, a_, b_)
#define v4u_sub(a_, b_) _v4_sub(v4u, a_, b_)

/* v_mul */
#define _v2_mul(t_, a_, b_) ((t_) { (a_).x * (b_).x, (a_).y * (b_).y })
#define _v3_mul(t_, a_, b_) ((t_) { (a_).x * (b_).x, (a_).y * (b_).y, (a_).z * (b_).z })
#define _v4_mul(t_, a_, b_) ((t_) { (a_).x * (b_).x, (a_).y * (b_).y, (a_).z * (b_).z, (a_).w * (b_).w })

#define v2f_mul(a_, b_) _v2_mul(v2f, a_, b_)
#define v2i_mul(a_, b_) _v2_mul(v2i, a_, b_)
#define v2u_mul(a_, b_) _v2_mul(v2u, a_, b_)
#define v3f_mul(a_, b_) _v3_mul(v3f, a_, b_)
#define v3i_mul(a_, b_) _v3_mul(v3i, a_, b_)
#define v3u_mul(a_, b_) _v3_mul(v3u, a_, b_)
#define v4f_mul(a_, b_) _v4_mul(v4f, a_, b_)
#define v4i_mul(a_, b_) _v4_mul(v4i, a_, b_)
#define v4u_mul(a_, b_) _v4_mul(v4u, a_, b_)

/* v_div */
#define _v2_div(t_, a_, b_) ((t_) { (a_).x / (b_).x, (a_).y / (b_).y })
#define _v3_div(t_, a_, b_) ((t_) { (a_).x / (b_).x, (a_).y / (b_).y, (a_).z / (b_).z })
#define _v4_div(t_, a_, b_) ((t_) { (a_).x / (b_).x, (a_).y / (b_).y, (a_).z / (b_).z, (a_).w / (b_).w })

#define v2f_div(a_, b_) _v2_div(v2f, a_, b_)
#define v2i_div(a_, b_) _v2_div(v2i, a_, b_)
#define v2u_div(a_, b_) _v2_div(v2u, a_, b_)
#define v3f_div(a_, b_) _v3_div(v3f, a_, b_)
#define v3i_div(a_, b_) _v3_div(v3i, a_, b_)
#define v3u_div(a_, b_) _v3_div(v3u, a_, b_)
#define v4f_div(a_, b_) _v4_div(v4f, a_, b_)
#define v4i_div(a_, b_) _v4_div(v4i, a_, b_)
#define v4u_div(a_, b_) _v4_div(v4u, a_, b_)

/* v_scale */
#define _v2_scale(t_, a_, b_) ((t_) { (a_).x * b_, (a_).y * b_ })
#define _v3_scale(t_, a_, b_) ((t_) { (a_).x * b_, (a_).y * b_, (a_).z * b_ })
#define _v4_scale(t_, a_, b_) ((t_) { (a_).x * b_, (a_).y * b_, (a_).z * b_, (a_).w * b_ })

#define v2f_scale(a_, b_) _v2_scale(v2f, a_, b_)
#define v2i_scale(a_, b_) _v2_scale(v2i, a_, b_)
#define v2u_scale(a_, b_) _v2_scale(v2u, a_, b_)
#define v3f_scale(a_, b_) _v3_scale(v3f, a_, b_)
#define v3i_scale(a_, b_) _v3_scale(v3i, a_, b_)
#define v3u_scale(a_, b_) _v3_scale(v3u, a_, b_)
#define v4f_scale(a_, b_) _v4_scale(v4f, a_, b_)
#define v4i_scale(a_, b_) _v4_scale(v4i, a_, b_)
#define v4u_scale(a_, b_) _v4_scale(v4u, a_, b_)

/* v_mag_sqrd */
#define v2_mag_sqrd(v_) ((v_).x * (v_).x + (v_).y * (v_).y)
#define v3_mag_sqrd(v_) ((v_).x * (v_).x + (v_).y * (v_).y + (v_).z * (v_).z)
#define v4_mag_sqrd(v_) ((v_).x * (v_).x + (v_).y * (v_).y + (v_).z * (v_).z + (v_).w * (v_).w)

/* v_mag */
#define v2_mag(v_) (sqrtf(v2_mag_sqrd(v_)))
#define v3_mag(v_) (sqrtf(v3_mag_sqrd(v_)))
#define v4_mag(v_) (sqrtf(v4_mag_sqrd(v_)))

/* v_normalised */
#define _v2_normalised(t_, t2_) \
	force_inline t_ t_##_normalised(t_ v_) { \
		const t2_ l = v2_mag(v_); \
		return (t_) { v_.x / l, v_.y / l }; \
	}

#define _v3_normalised(t_, t2_) \
	force_inline t_ t_##_normalised(t_ v_) { \
		const t2_ l = v3_mag(v_); \
		return (t_) { v_.x / l, v_.y / l, v_.z / l }; \
	}

#define _v4_normalised(t_, t2_) \
	force_inline t_ t_##_normalised(t_ v_) { \
		const t2_ l = v4_mag(v_); \
		return (t_) { v_.x / l, v_.y / l, v_.z / l, v_.w / l }; \
	}

_v2_normalised(v2f, f32)
_v3_normalised(v3f, f32)
_v4_normalised(v4f, f32)

/* v_eq */
#define _v2_eq(t_, a_, b_) ((a_).x == (b_).x && (a_).y == (b_).y)
#define _v3_eq(t_, a_, b_) ((a_).x == (b_).x && (a_).y == (b_).y && (a_).z == (b_).z)
#define _v4_eq(t_, a_, b_) ((a_).x == (b_).x && (a_).y == (b_).y && (a_).z == (b_).z && (a_).w == (b_).w)

#define v2f_eq(a_, b_) _v2_eq(v2f, a_, b_)
#define v2i_eq(a_, b_) _v2_eq(v2i, a_, b_)
#define v2u_eq(a_, b_) _v2_eq(v2u, a_, b_)
#define v3f_eq(a_, b_) _v3_eq(v3f, a_, b_)
#define v3i_eq(a_, b_) _v3_eq(v3i, a_, b_)
#define v3u_eq(a_, b_) _v3_eq(v3u, a_, b_)
#define v4f_eq(a_, b_) _v4_eq(v4f, a_, b_)
#define v4i_eq(a_, b_) _v4_eq(v4i, a_, b_)
#define v4u_eq(a_, b_) _v4_eq(v4u, a_, b_)

/* Quaternion. */

/* 4x4 float matrix. */
#define make_m4f(d_) \
	((m4f) {{ \
		{ d_,   0.0f, 0.0f, 0.0f }, \
		{ 0.0f, d_,   0.0f, 0.0f }, \
		{ 0.0f, 0.0f, d_,   0.0f }, \
		{ 0.0f, 0.0f, 0.0f, d_   }, \
	}})

#define m4f_zero() \
	((m4f) {{ \
		{ 0.0f, 0.0f, 0.0f, 0.0f }, \
		{ 0.0f, 0.0f, 0.0f, 0.0f }, \
		{ 0.0f, 0.0f, 0.0f, 0.0f }, \
		{ 0.0f, 0.0f, 0.0f, 0.0f }, \
	}})

#define m4f_identity() make_m4f(1.0f)

#define m4f_screenspace(hw_, hh_) \
	((m4f) {{ \
		{ hw_,  0.0f,   0.0f, hw_  }, \
		{ 0.0f, -(hh_), 0.0f, hh_  }, \
		{ 0.0f, 0.0f,   0.0f, 0.0f }, \
		{ 0.0f, 0.0f,   0.0f, 0.0f }, \
	}})

#define m4f_mul(a_, b_) \
	((m4f) {{ \
		{ \
			(a_).m[0][0] * (b_).m[0][0] + (a_).m[1][0] * (b_).m[0][1] + (a_).m[2][0] * (b_).m[0][2] + (a_).m[3][0] * (b_).m[0][3], \
			(a_).m[0][1] * (b_).m[0][0] + (a_).m[1][1] * (b_).m[0][1] + (a_).m[2][1] * (b_).m[0][2] + (a_).m[3][1] * (b_).m[0][3], \
			(a_).m[0][2] * (b_).m[0][0] + (a_).m[1][2] * (b_).m[0][1] + (a_).m[2][2] * (b_).m[0][2] + (a_).m[3][2] * (b_).m[0][3], \
			(a_).m[0][3] * (b_).m[0][0] + (a_).m[1][3] * (b_).m[0][1] + (a_).m[2][3] * (b_).m[0][2] + (a_).m[3][3] * (b_).m[0][3] \
		}, \
		{ \
			(a_).m[0][0] * (b_).m[1][0] + (a_).m[1][0] * (b_).m[1][1] + (a_).m[2][0] * (b_).m[1][2] + (a_).m[3][0] * (b_).m[1][3], \
			(a_).m[0][1] * (b_).m[1][0] + (a_).m[1][1] * (b_).m[1][1] + (a_).m[2][1] * (b_).m[1][2] + (a_).m[3][1] * (b_).m[1][3], \
			(a_).m[0][2] * (b_).m[1][0] + (a_).m[1][2] * (b_).m[1][1] + (a_).m[2][2] * (b_).m[1][2] + (a_).m[3][2] * (b_).m[1][3], \
			(a_).m[0][3] * (b_).m[1][0] + (a_).m[1][3] * (b_).m[1][1] + (a_).m[2][3] * (b_).m[1][2] + (a_).m[3][3] * (b_).m[1][3] \
		}, \
		{ \
			(a_).m[0][0] * (b_).m[2][0] + (a_).m[1][0] * (b_).m[2][1] + (a_).m[2][0] * (b_).m[2][2] + (a_).m[3][0] * (b_).m[2][3], \
			(a_).m[0][1] * (b_).m[2][0] + (a_).m[1][1] * (b_).m[2][1] + (a_).m[2][1] * (b_).m[2][2] + (a_).m[3][1] * (b_).m[2][3], \
			(a_).m[0][2] * (b_).m[2][0] + (a_).m[1][2] * (b_).m[2][1] + (a_).m[2][2] * (b_).m[2][2] + (a_).m[3][2] * (b_).m[2][3], \
			(a_).m[0][3] * (b_).m[2][0] + (a_).m[1][3] * (b_).m[2][1] + (a_).m[2][3] * (b_).m[2][2] + (a_).m[3][3] * (b_).m[2][3] \
		}, \
		{ \
			(a_).m[0][0] * (b_).m[3][0] + (a_).m[1][0] * (b_).m[3][1] + (a_).m[2][0] * (b_).m[3][2] + (a_).m[3][0] * (b_).m[3][3], \
			(a_).m[0][1] * (b_).m[3][0] + (a_).m[1][1] * (b_).m[3][1] + (a_).m[2][1] * (b_).m[3][2] + (a_).m[3][1] * (b_).m[3][3], \
			(a_).m[0][2] * (b_).m[3][0] + (a_).m[1][2] * (b_).m[3][1] + (a_).m[2][2] * (b_).m[3][2] + (a_).m[3][2] * (b_).m[3][3], \
			(a_).m[0][3] * (b_).m[3][0] + (a_).m[1][3] * (b_).m[3][1] + (a_).m[2][3] * (b_).m[3][2] + (a_).m[3][3] * (b_).m[3][3] \
		}, \
	}})


#define m4f_translation(v_) \
	((m4f) {{ \
		{ 0.0f,   0.0f,   0.0f,   0.0f }, \
		{ 0.0f,   0.0f,   0.0f,   0.0f }, \
		{ 0.0f,   0.0f,   0.0f,   0.0f }, \
		{ (v_).x, (v_).y, (v_).z, 0.0f }, \
	}})

#define m4f_scale(v_) \
	((m4f) {{ \
		{ (v_).x, 0.0f,   0.0f,   0.0f }, \
		{ 0.0f,   (v_).y, 0.0f,   0.0f }, \
		{ 0.0f,   0.0f,   (v_).z, 0.0f }, \
		{ 0.0f,   0.0f,   0.0f,   0.0f }, \
	}})

force_inline m4f m4f_rotation(f32 a, v3f v) {
	m4f r = m4f_identity();

	const f32 c = cosf(a);
	const f32 s = sinf(a);

	const f32 omc = 1.0f - c;

	const f32 x = v.x;
	const f32 y = v.y;
	const f32 z = v.z;

	r.m[0][0] = x * x * omc + c;
	r.m[0][1] = y * x * omc + z * s;
	r.m[0][2] = x * z * omc - y * s;
	r.m[1][0] = x * y * omc - z * s;
	r.m[1][1] = y * y * omc + c;
	r.m[1][2] = y * z * omc + x * s;
	r.m[2][0] = x * z * omc + y * s;
	r.m[2][1] = y * z * omc - x * s;
	r.m[2][2] = z * z * omc + c;

	return r;
}

#define m4f_transform(m_, p_) \
	make_v4f( \
		(m_).m[0][0] * (p_).x + (m_).m[1][0] * (p_).y + (m_).m[2][0] * (p_).z + (m_).m[3][0] + (p_).w, \
		(m_).m[0][1] * (p_).x + (m_).m[1][1] * (p_).y + (m_).m[2][1] * (p_).z + (m_).m[3][1] + (p_).w, \
		(m_).m[0][2] * (p_).x + (m_).m[1][2] * (p_).y + (m_).m[2][2] * (p_).z + (m_).m[3][2] + (p_).w, \
		(m_).m[0][3] * (p_).x + (m_).m[1][3] * (p_).y + (m_).m[2][3] * (p_).z + (m_).m[3][3] + (p_).w)
