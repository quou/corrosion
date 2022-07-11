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

#define v3f_cross(a_, b_) \
	make_v3f((a_).y * (b_).z - (a_).z * (b_).y, (a_).z * (b_).x - (a_).x * (b_).z, (a_).x * (b_).y - (a_).y * (b_).x)

#define v3f_dot(a_, b_) \
	((a_).x * (b_).x + (a_).y * (b_).y + (a_).z * (b_).z)

/* Quaternion. */
typedef v4f quat;

#define make_quat(a_, s_) make_v4f((a_).x, (a_).y, (a_).z, s_)

#define quat_identity()	make_quat(make_v3f(0.0f, 0.0f, 0.0f), 1.0f)

#define quat_scale(q_, s_) \
	((quat) { (q_).x * s_, (q_).y * s_, (q_).z * s_, (q_).w })

force_inline f32 quat_norm(quat q) {
	return
		(q.x * q.x) +
		(q.y * q.y) +
		(q.z * q.z) +
		(q.w * q.w);
}

#define quat_normalised(q_) \
	quat_scale(q_, 1.0f / sqrtf(quat_norm(q_)))

force_inline quat quat_mul(quat a, quat b) {
	return quat_normalised(((quat) {
		(((a.w * b.x) + (a.x * b.w)) + (a.y * b.z)) - (a.z * b.y),
		(((a.w * b.y) + (a.y * b.w)) + (a.z * b.x)) - (a.x * b.z),
		(((a.w * b.z) + (a.z * b.w)) + (a.x * b.y)) - (a.y * b.x),
		(((a.w * b.w) - (a.x * b.x)) - (a.y * b.y)) - (a.z * b.z)
	}));
}

#define euler(a_) quat_mul(quat_mul( \
	make_quat(make_v3f(1.0f, 0.0f, 0.0f), to_rad((a_).x)),  \
	make_quat(make_v3f(0.0f, 1.0f, 0.0f), to_rad((a_).y))), \
	make_quat(make_v3f(0.0f, 0.0f, 1.0f), to_rad((a_).z)))

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
		{ 0.0f, 0.0f,   1.0f, 0.0f }, \
		{ 0.0f, 0.0f,   0.0f, 1.0f }, \
	}})

force_inline m4f m4f_mul(m4f a, m4f b) {
	return ((m4f) {{ \
		{
			a.m[0][0] * b.m[0][0] + a.m[1][0] * b.m[0][1] + a.m[2][0] * b.m[0][2] + a.m[3][0] * b.m[0][3],
			a.m[0][1] * b.m[0][0] + a.m[1][1] * b.m[0][1] + a.m[2][1] * b.m[0][2] + a.m[3][1] * b.m[0][3],
			a.m[0][2] * b.m[0][0] + a.m[1][2] * b.m[0][1] + a.m[2][2] * b.m[0][2] + a.m[3][2] * b.m[0][3],
			a.m[0][3] * b.m[0][0] + a.m[1][3] * b.m[0][1] + a.m[2][3] * b.m[0][2] + a.m[3][3] * b.m[0][3] 
		},
		{
			a.m[0][0] * b.m[1][0] + a.m[1][0] * b.m[1][1] + a.m[2][0] * b.m[1][2] + a.m[3][0] * b.m[1][3],
			a.m[0][1] * b.m[1][0] + a.m[1][1] * b.m[1][1] + a.m[2][1] * b.m[1][2] + a.m[3][1] * b.m[1][3],
			a.m[0][2] * b.m[1][0] + a.m[1][2] * b.m[1][1] + a.m[2][2] * b.m[1][2] + a.m[3][2] * b.m[1][3],
			a.m[0][3] * b.m[1][0] + a.m[1][3] * b.m[1][1] + a.m[2][3] * b.m[1][2] + a.m[3][3] * b.m[1][3] 
		},
		{
			a.m[0][0] * b.m[2][0] + a.m[1][0] * b.m[2][1] + a.m[2][0] * b.m[2][2] + a.m[3][0] * b.m[2][3],
			a.m[0][1] * b.m[2][0] + a.m[1][1] * b.m[2][1] + a.m[2][1] * b.m[2][2] + a.m[3][1] * b.m[2][3],
			a.m[0][2] * b.m[2][0] + a.m[1][2] * b.m[2][1] + a.m[2][2] * b.m[2][2] + a.m[3][2] * b.m[2][3],
			a.m[0][3] * b.m[2][0] + a.m[1][3] * b.m[2][1] + a.m[2][3] * b.m[2][2] + a.m[3][3] * b.m[2][3] 
		},
		{
			a.m[0][0] * b.m[3][0] + a.m[1][0] * b.m[3][1] + a.m[2][0] * b.m[3][2] + a.m[3][0] * b.m[3][3],
			a.m[0][1] * b.m[3][0] + a.m[1][1] * b.m[3][1] + a.m[2][1] * b.m[3][2] + a.m[3][1] * b.m[3][3],
			a.m[0][2] * b.m[3][0] + a.m[1][2] * b.m[3][1] + a.m[2][2] * b.m[3][2] + a.m[3][2] * b.m[3][3],
			a.m[0][3] * b.m[3][0] + a.m[1][3] * b.m[3][1] + a.m[2][3] * b.m[3][2] + a.m[3][3] * b.m[3][3] 
		},
	}});

}

#define m4f_translation(v_) \
	((m4f) {{ \
		{ 1.0f,   0.0f,   0.0f,   0.0f }, \
		{ 0.0f,   1.0f,   0.0f,   0.0f }, \
		{ 0.0f,   0.0f,   1.0f,   0.0f }, \
		{ (v_).x, (v_).y, (v_).z, 1.0f }, \
	}})

#define m4f_scale(v_) \
	((m4f) {{ \
		{ (v_).x, 0.0f,   0.0f,   0.0f }, \
		{ 0.0f,   (v_).y, 0.0f,   0.0f }, \
		{ 0.0f,   0.0f,   (v_).z, 0.0f }, \
		{ 0.0f,   0.0f,   0.0f,   1.0f }, \
	}})

force_inline m4f m4f_rotation(quat q) {
	m4f r = m4f_identity();

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
	r.m[0][1] = (qxqy2 - qzqw2);
	r.m[0][2] = (qxqz2 + qyqw2);
	r.m[0][3] = 0.0f;

	r.m[1][0] = (qxqy2 + qzqw2);
	r.m[1][1] = ((1.0f - qxqx2) - qzqz2);
	r.m[1][2] = (qyqz2 - qxqw2);
	r.m[1][3] = 0.0f;

	r.m[2][0] = (qxqz2 - qyqw2);
	r.m[2][1] = (qyqz2 + qxqw2);
	r.m[2][2] = ((1.0f - qxqx2) - qyqy2);
	r.m[2][3] = 0.0f;

	return r;
}

force_inline m4f m4f_lookat(v3f c, v3f o, v3f u) {
	m4f r = m4f_identity();

	const v3f f = v3f_normalised(v3f_sub(o, c));
	u = v3f_normalised(u);
	const v3f s = v3f_normalised(v3f_cross(f, u));
	u = v3f_cross(s, f);

	r.m[0][0] = s.x;
	r.m[1][0] = s.y;
	r.m[2][0] = s.z;
	r.m[0][1] = u.x;
	r.m[1][1] = u.y;
	r.m[2][1] = u.z;
	r.m[0][2] = -f.x;
	r.m[1][2] = -f.y;
	r.m[2][2] = -f.z;
	r.m[3][0] = -v3f_dot(s, c);
	r.m[3][1] = -v3f_dot(u, c);
	r.m[3][2] =  v3f_dot(f, c);

	return r;
}

force_inline v4f m4f_transform(m4f m, v4f p) {
	return make_v4f(
		m.m[0][0] * p.x + m.m[1][0] * p.y + m.m[2][0] * p.z + m.m[3][0] * p.w,
		m.m[0][1] * p.x + m.m[1][1] * p.y + m.m[2][1] * p.z + m.m[3][1] * p.w,
		m.m[0][2] * p.x + m.m[1][2] * p.y + m.m[2][2] * p.z + m.m[3][2] * p.w,
		m.m[0][3] * p.x + m.m[1][3] * p.y + m.m[2][3] * p.z + m.m[3][3] * p.w);
}
