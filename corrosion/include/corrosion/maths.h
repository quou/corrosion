#pragma once

#include <math.h>

#include "common.h"

#define cr_min(a_, b_) ((a_) < (b_) ? (a_) : (b_))
#define cr_max(a_, b_) ((a_) > (b_) ? (a_) : (b_))

#define cr_v2(t_) struct { \
	union { t_ x; t_ u; }; \
	union { t_ y; t_ v; }; \
}
#define cr_v3(t_) struct { \
	union { t_ x; t_ r; t_ h; }; \
	union { t_ y; t_ g; t_ s; }; \
	union { t_ z; t_ b; t_ v; }; \
}
#define cr_v4(t_) struct { \
	union { t_ x; t_ r; t_ h; }; \
	union { t_ y; t_ g; t_ s; }; \
	union { t_ z; t_ b; t_ v; }; \
	union { t_ w; t_ a;       }; \
}

#define cr_m4(t_) struct { t_ m[4][4]; }

typedef cr_v2(f32) v2f;
typedef cr_v2(i32) v2i;
typedef cr_v2(u32) v2u;
typedef cr_v3(f32) v3f;
typedef cr_v3(i32) v3i;
typedef cr_v3(u32) v3u;
typedef cr_v4(f32) v4f;
typedef cr_v4(i32) v4i;
typedef cr_v4(u32) v4u;

typedef cr_m4(f32) m4f;

typedef v4f quat;

#if !defined(__cplusplus)

#define pi_f 3.14159265358f
#define pi_d 3.14159265358979323846

#define to_rad(v_) _Generic((v_), \
		f32: (v_ * (pi_f / 180.0f)), \
		f64: (v_ * (pi_d / 180.0)),  \
		const f32: (v_ * (pi_f / 180.0f)), \
		const f64: (v_ * (pi_d / 180.0))  \
	)

#define to_deg(v_) _Generic((v_), \
		f32: (v_ * (180.0f / pi_f)), \
		f64: (v_ * (180.0  / pi_d)),  \
		const f32: (v_ * (180.0f / pi_f)), \
		const f64: (v_ * (180.0  / pi_d)) \
	)

#define lerp(a_, b_, t_) ((a_) + (t_) * ((b_) - (a_)))
#define clamp(v_, min_, max_) (cr_max(min_, cr_min(max_, v_)))
#define map(v_, min1_, max1_, min2_, max2_) ((min2_) + ((v_) - (min1_)) * ((max2_) - (min2_)) / ((max1_) - (min1_)))
#define saturate(v_) (clamp(v_, 0.0f, 1.0f))

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

/* v_zero */
#define v2f_zero() make_v2f(0.0f, 0.0f)
#define v2i_zero() make_v2i(0,    0)
#define v2u_zero() make_v2u(0,    0)
#define v3f_zero() make_v3f(0.0f, 0.0f, 0.0f)
#define v3i_zero() make_v3i(0,    0,    0)
#define v3u_zero() make_v3u(0,    0,    0)
#define v4f_zero() make_v4f(0.0f, 0.0f, 0.0f, 0.0f)
#define v4i_zero() make_v4i(0,    0,    0,    0)
#define v4u_zero() make_v4u(0,    0,    0,    0)

/* Swizzle. */
#define v_xy(t_, v_)   make_v2(t_, (v_).x, (v_).y)
#define v_xz(t_, v_)   make_v2(t_, (v_).x, (v_).z)
#define v_xw(t_, v_)   make_v2(t_, (v_).x, (v_).w)
#define v_yz(t_, v_)   make_v2(t_, (v_).y, (v_).z)
#define v_yw(t_, v_)   make_v2(t_, (v_).y, (v_).w)
#define v_zw(t_, v_)   make_v2(t_, (v_).z, (v_).w)
#define v_yx(t_, v_)   make_v2(t_, (v_).y, (v_).x)
#define v_zx(t_, v_)   make_v2(t_, (v_).z, (v_).x)
#define v_wx(t_, v_)   make_v2(t_, (v_).w, (v_).x)
#define v_zy(t_, v_)   make_v2(t_, (v_).z, (v_).y)
#define v_wy(t_, v_)   make_v2(t_, (v_).w, (v_).y)
#define v_wz(t_, v_)   make_v2(t_, (v_).w, (v_).z)
#define v_xyz(t_, v_)  make_v3(t_, (v_).x, (v_).y, (v_).z)
#define v_xzy(t_, v_)  make_v3(t_, (v_).x, (v_).z, (v_).y)
#define v_zyx(t_, v_)  make_v3(t_, (v_).z, (v_).y, (v_).x)
#define v_yzx(t_, v_)  make_v3(t_, (v_).y, (v_).z, (v_).x)
#define v_xyzw(t_, v_) make_v4(t_, (v_).x, (v_).y, (v_).z, (v_).w)
#define v_xzyw(t_, v_) make_v4(t_, (v_).x, (v_).z, (v_).y, (v_).w)
#define v_zyxw(t_, v_) make_v4(t_, (v_).z, (v_).y, (v_).x, (v_).w)
#define v_yzxw(t_, v_) make_v4(t_, (v_).y, (v_).z, (v_).x, (v_).w)
#define v_xywz(t_, v_) make_v4(t_, (v_).x, (v_).y, (v_).w, (v_).z)
#define v_xzwy(t_, v_) make_v4(t_, (v_).x, (v_).z, (v_).w, (v_).y)
#define v_zywx(t_, v_) make_v4(t_, (v_).z, (v_).y, (v_).w, (v_).x)
#define v_yzwx(t_, v_) make_v4(t_, (v_).y, (v_).z, (v_).w, (v_).x)
#define v_xwyz(t_, v_) make_v4(t_, (v_).x, (v_).w, (v_).y, (v_).z)
#define v_xwzy(t_, v_) make_v4(t_, (v_).x, (v_).w, (v_).z, (v_).y)
#define v_zwyx(t_, v_) make_v4(t_, (v_).z, (v_).w, (v_).y, (v_).x)
#define v_ywzx(t_, v_) make_v4(t_, (v_).y, (v_).w, (v_).z, (v_).x)
#define v_wxyz(t_, v_) make_v4(t_, (v_).w, (v_).x, (v_).y, (v_).z)
#define v_wxzy(t_, v_) make_v4(t_, (v_).w, (v_).x, (v_).z, (v_).y)
#define v_wzyx(t_, v_) make_v4(t_, (v_).w, (v_).z, (v_).y, (v_).x)
#define v_wyzx(t_, v_) make_v4(t_, (v_).w, (v_).y, (v_).z, (v_).x)

#define vf_xy(v_)   v_xy(v2f, v_)
#define vf_xz(v_)   v_xz(v2f, v_)
#define vf_xw(v_)   v_xw(v2f, v_)
#define vf_yz(v_)   v_yz(v2f, v_)
#define vf_yw(v_)   v_yw(v2f, v_)
#define vf_zw(v_)   v_zw(v2f, v_)
#define vf_yx(v_)   v_yx(v2f, v_)
#define vf_zx(v_)   v_zx(v2f, v_)
#define vf_wx(v_)   v_wx(v2f, v_)
#define vf_zy(v_)   v_zy(v2f, v_)
#define vf_wy(v_)   v_wy(v2f, v_)
#define vf_wz(v_)   v_wz(v2f, v_)
#define vf_xyz(v_)  v_xyz(v3f, v_)
#define vf_xzy(v_)  v_xzy(v3f, v_)
#define vf_zyx(v_)  v_zyx(v3f, v_)
#define vf_yzx(v_)  v_yzx(v3f, v_)
#define vf_xyzw(v_) v_xyzw(v4f, v_)
#define vf_xzyw(v_) v_xzyw(v4f, v_)
#define vf_zyxw(v_) v_zyxw(v4f, v_)
#define vf_yzxw(v_) v_yzxw(v4f, v_)
#define vf_xywz(v_) v_xywz(v4f, v_)
#define vf_xzwy(v_) v_xzwy(v4f, v_)
#define vf_zywx(v_) v_zywx(v4f, v_)
#define vf_yzwx(v_) v_yzwx(v4f, v_)
#define vf_xwyz(v_) v_xwyz(v4f, v_)
#define vf_xwzy(v_) v_xwzy(v4f, v_)
#define vf_zwyx(v_) v_zwyx(v4f, v_)
#define vf_ywzx(v_) v_ywzx(v4f, v_)
#define vf_wxyz(v_) v_wxyz(v4f, v_)
#define vf_wxzy(v_) v_wxzy(v4f, v_)
#define vf_wzyx(v_) v_wzyx(v4f, v_)
#define vf_wyzx(v_) v_wyzx(v4f, v_)

#define vi_xy(t_)   v_xy(v2i, v_)
#define vi_xz(t_)   v_xz(v2i, v_)
#define vi_xw(t_)   v_xw(v2i, v_)
#define vi_yz(t_)   v_yz(v2i, v_)
#define vi_yw(t_)   v_yw(v2i, v_)
#define vi_zw(t_)   v_zw(v2i, v_)
#define vi_yx(t_)   v_yx(v2i, v_)
#define vi_zx(t_)   v_zx(v2i, v_)
#define vi_wx(t_)   v_wx(v2i, v_)
#define vi_zy(t_)   v_zy(v2i, v_)
#define vi_wy(t_)   v_wy(v2i, v_)
#define vi_wz(t_)   v_wz(v2i, v_)
#define vi_xyz(v_)  v_xyz(v3i, v_)
#define vi_xzy(v_)  v_xzy(v3i, v_)
#define vi_zyx(v_)  v_zyx(v3i, v_)
#define vi_yzx(v_)  v_yzx(v3i, v_)
#define vi_xyzw(v_) v_xyzw(v4i, v_)
#define vi_xzyw(v_) v_xzyw(v4i, v_)
#define vi_zyxw(v_) v_zyxw(v4i, v_)
#define vi_yzxw(v_) v_yzxw(v4i, v_)
#define vi_xywz(v_) v_xywz(v4i, v_)
#define vi_xzwy(v_) v_xzwy(v4i, v_)
#define vi_zywx(v_) v_zywx(v4i, v_)
#define vi_yzwx(v_) v_yzwx(v4i, v_)
#define vi_xwyz(v_) v_xwyz(v4i, v_)
#define vi_xwzy(v_) v_xwzy(v4i, v_)
#define vi_zwyx(v_) v_zwyx(v4i, v_)
#define vi_ywzx(v_) v_ywzx(v4i, v_)
#define vi_wxyz(v_) v_wxyz(v4i, v_)
#define vi_wxzy(v_) v_wxzy(v4i, v_)
#define vi_wzyx(v_) v_wzyx(v4i, v_)
#define vi_wyzx(v_) v_wyzx(v4i, v_)

#define vu_xy(v_)   v_xy(v2u, v_)
#define vu_xz(v_)   v_xz(v2u, v_)
#define vu_xw(v_)   v_xw(v2u, v_)
#define vu_yz(v_)   v_yz(v2u, v_)
#define vu_yw(v_)   v_yw(v2u, v_)
#define vu_zw(v_)   v_zw(v2u, v_)
#define vu_yx(v_)   v_yx(v2u, v_)
#define vu_zx(v_)   v_zx(v2u, v_)
#define vu_wx(v_)   v_wx(v2u, v_)
#define vu_zy(v_)   v_zy(v2u, v_)
#define vu_wy(v_)   v_wy(v2u, v_)
#define vu_wz(v_)   v_wz(v2u, v_)
#define vu_xyz(v_)  v_xyz(v3u, v_)
#define vu_xzy(v_)  v_xzy(v3u, v_)
#define vu_zyx(v_)  v_zyx(v3u, v_)
#define vu_yzx(v_)  v_yzx(v3u, v_)
#define vu_xyzw(v_) v_xyzw(v4u, v_)
#define vu_xzyw(v_) v_xzyw(v4u, v_)
#define vu_zyxw(v_) v_zyxw(v4u, v_)
#define vu_yzxw(v_) v_yzxw(v4u, v_)
#define vu_xywz(v_) v_xywz(v4u, v_)
#define vu_xzwy(v_) v_xzwy(v4u, v_)
#define vu_zywx(v_) v_zywx(v4u, v_)
#define vu_yzwx(v_) v_yzwx(v4u, v_)
#define vu_xwyz(v_) v_xwyz(v4u, v_)
#define vu_xwzy(v_) v_xwzy(v4u, v_)
#define vu_zwyx(v_) v_zwyx(v4u, v_)
#define vu_ywzx(v_) v_ywzx(v4u, v_)
#define vu_wxyz(v_) v_wxyz(v4u, v_)
#define vu_wxzy(v_) v_wxzy(v4u, v_)
#define vu_wzyx(v_) v_wzyx(v4u, v_)
#define vu_wyzx(v_) v_wyzx(v4u, v_)

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

/* From https://github.com/Immediate-Mode-UI/Nuklear/blob/master/src/nuklear_color.c */
force_inline v4f rgba_to_hsva(v4f rgba) {
	f32 chroma;
	f32 K = 0.0f;
	if (rgba.g < rgba.b) {
		const float t = rgba.g; rgba.g = rgba.b; rgba.b = t;
		K = -1.f;
	}
	if (rgba.r < rgba.g) {
		const float t = rgba.r; rgba.r = rgba.g; rgba.g = t;
		K = -2.f/6.0f - K;
	}
	chroma = rgba.r - ((rgba.g < rgba.b) ? rgba.g : rgba.b);
	
	v4f r;
	
	r.x = fabsf(K + (rgba.g - rgba.b)/(6.0f * chroma + 1e-20f));
	r.y = chroma / (rgba.r + 1e-20f);
	r.z = rgba.r;
	r.w = rgba.a;
	
	return r;
}

force_inline v4f hsva_to_rgba(v4f hsva) {
	int i;
	float p, q, t, f;

	v4f out;

	if (hsva.s <= 0.0f) {
		out.r = hsva.v; out.g = hsva.v; out.b = hsva.v; out.a = hsva.a;
		return out;
	}

	hsva.h = hsva.h / (60.0f/360.0f);
	i = (i32)hsva.h;
	f = hsva.h - (f32)i;
	p = hsva.v * (1.0f - hsva.s);
	q = hsva.v * (1.0f - (hsva.s * f));
	t = hsva.v * (1.0f - hsva.s * (1.0f - f));
	
	switch (i) {
		case 0: default: out.r = hsva.v; out.g = t; out.b = p; break;
		case 1: out.r = q; out.g = hsva.v; out.b = p; break;
		case 2: out.r = p; out.g = hsva.v; out.b = t; break;
		case 3: out.r = p; out.g = q; out.b = hsva.v; break;
		case 4: out.r = t; out.g = p; out.b = hsva.v; break;
		case 5: out.r = hsva.v; out.g = p; out.b = q; break;
	}

	out.a = hsva.a;

	return out;
}

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

/* v_dot */
#define v2_dot(a_, b_) \
	((a_).x * (b_).x + (a_).y * (b_).y)

#define v3_dot(a_, b_) \
	((a_).x * (b_).x + (a_).y * (b_).y + (a_).z * (b_).z)

#define v4_dot(a_, b_) \
	((a_).x * (b_).x + (a_).y * (b_).y + (a_).z * (b_).z + (a_).w * (b_).w)

/* v_abs */
#define v2_abs(v_) _Generic((v_), \
		v2f: make_v2f(fabsf((v_).x), fabsf((v_).y)), \
		v2i: make_v2f(fabsf((v_).x), fabsf((v_).y)), \
		v2u: make_v2f(fabsf((v_).x), fabsf((v_).y))  \
	)

#define v3_abs(v_) _Generic((v_), \
		v3f: make_v3f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z)), \
		v3i: make_v3f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z)), \
		v3u: make_v3f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z))  \
	)

#define v4_abs(v_) _Generic((v_), \
		v4f: make_v4f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z), fabsf((v_).w)), \
		v4i: make_v4f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z), fabsf((v_).w)), \
		v4u: make_v4f(fabsf((v_).x), fabsf((v_).y), fabsf((v_).z), fabsf((v_).w))  \
	)

/* Quaternion. */
#define make_quat(a_, s_) make_v4f((a_).x, (a_).y, (a_).z, s_)

#define quat_identity()	make_quat(make_v3f(0.0f, 0.0f, 0.0f), 1.0f)

force_inline quat quat_scale(quat q, f32 s) {
	return (quat) { q.x * s, q.y * s, q.z * s, q.w };
}

force_inline f32 quat_normal(quat q) {
	return v4_dot(q, q);
}

force_inline quat quat_conjugate(quat q) {
	return (quat) {
		-q.x,
		-q.y,
		-q.z,
		 q.w
	};
}

force_inline quat quat_normalised(quat q) {
	return quat_scale(q, 1.0f / sqrtf(quat_normal(q)));
}

force_inline quat quat_mul(quat a, quat b) {
	return quat_normalised((quat) {
		 a.x * b.w +
		 a.y * b.z -
		 a.z * b.y +
		 a.w * b.x,

		-a.x * b.z +
		 a.y * b.w +
		 a.z * b.x +
		 a.w * b.y,

		 a.x * b.y -
		 a.y * b.x +
		 a.z * b.w +
		 a.w * b.z,

		-a.x * b.x -
		 a.y * b.y -
		 a.z * b.z +
		 a.w * b.w
	});
}

/* Multiply without normalising. */
force_inline quat quat_mul_dn(quat a, quat b) {
	return (quat) {
		 a.x * b.w +
		 a.y * b.z -
		 a.z * b.y +
		 a.w * b.x,

		-a.x * b.z +
		 a.y * b.w +
		 a.z * b.x +
		 a.w * b.y,

		 a.x * b.y -
		 a.y * b.x +
		 a.z * b.w +
		 a.w * b.z,

		-a.x * b.x -
		 a.y * b.y -
		 a.z * b.z +
		 a.w * b.w
	};
}

force_inline quat quat_rotate(f32 angle, v3f axis) {
	quat q;

	angle = to_rad(angle);

	f32 half_angle = 0.5f * angle;
	f32 sin_ha = sinf(half_angle);

	q.w = cosf(half_angle);
	q.x = sin_ha * axis.x;
	q.y = sin_ha * axis.y;
	q.z = sin_ha * axis.z;

	return q;
}

force_inline quat euler(v3f a) {
	quat p = quat_rotate(a.x, make_v3f(1.0f, 0.0f, 0.0f));
	quat y = quat_rotate(a.y, make_v3f(0.0f, 1.0f, 0.0f));
	quat r = quat_rotate(a.z, make_v3f(0.0f, 0.0f, 1.0f));

	return quat_mul(quat_mul(p, y), r);
}

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
		{ hw_,  0.0f,   0.0f, 0.0f }, \
		{ 0.0f, -(hh_), 0.0f, 0.0f }, \
		{ 0.0f, 0.0f,   1.0f, 0.0f }, \
		{ hw_,  hh_,    0.0f, 1.0f }, \
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

force_inline m4f m4f_translation(v3f v) {
	return (m4f) { {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f },
		{ v.x,  v.y,  v.z,  1.0f },
	} };
}

force_inline m4f m4f_scale(v3f v) {
	return (m4f) { {
		{ v.x,  0.0f, 0.0f, 0.0f },
		{ 0.0f, v.y,  0.0f, 0.0f },
		{ 0.0f, 0.0f, v.z,  0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	} };
}

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
	r.m[3][0] = -v3_dot(s, c);
	r.m[3][1] = -v3_dot(u, c);
	r.m[3][2] =  v3_dot(f, c);

	return r;
}

force_inline v4f m4f_transform(m4f m, v4f p) {
	return make_v4f(
		m.m[0][0] * p.x + m.m[1][0] * p.y + m.m[2][0] * p.z + m.m[3][0] * p.w,
		m.m[0][1] * p.x + m.m[1][1] * p.y + m.m[2][1] * p.z + m.m[3][1] * p.w,
		m.m[0][2] * p.x + m.m[1][2] * p.y + m.m[2][2] * p.z + m.m[3][2] * p.w,
		m.m[0][3] * p.x + m.m[1][3] * p.y + m.m[2][3] * p.z + m.m[3][3] * p.w);
}

force_inline m4f m4f_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
	m4f res = m4f_identity();

	res.m[0][0] = (f32)2 / (r - l);
	res.m[1][1] = (f32)2 / (t - b);
	res.m[2][2] = (f32)2 / (n - f);

	res.m[3][0] = (l + r) / (l - r);
	res.m[3][1] = (b + t) / (b - t);
	res.m[3][2] = (f + n) / (f - n);

	return res;
}

force_inline m4f m4f_persp(f32 fov, f32 aspect, f32 near_clip, f32 far_clip) {
	m4f r = m4f_identity();

	const f32 q = 1.0f / tanf(to_rad(fov) / 2.0f);
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
#endif
