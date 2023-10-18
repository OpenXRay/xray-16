#pragma once

// Base targets
#define r1_RT_base "$user$base_"
#define r1_RT_base_depth "$user$base_depth"

#define r1_RT_generic "$user$rendertarget"

#define rt_RT_color_map "$user$rendertarget_color_map"
#define rt_RT_distort "$user$distort"

#define r1_RT_depth "$user$depth" // surface

// Used in LightShadows
#define r1_RT_shadow "$user$shadow"
#define r1_RT_temp "$user$temp"
#define rt_RT_temp_zb "$user$temp_zb" // surface

#define r1_RT_async_ss "$user$async_ss" // surface

#define r2_T_sky0 "$user$sky0"
#define r2_T_sky1 "$user$sky1"

#define r2_T_envs0 "$user$env_s0"
#define r2_T_envs1 "$user$env_s1"

#define r2_RT_luminance_cur "$user$tonemap" // --- result

static constexpr auto c_ldynamic_props = "L_dynamic_props";
static constexpr auto c_sbase = "s_base";
static constexpr auto c_ssky0 = "s_sky0";
static constexpr auto c_ssky1 = "s_sky1";
static constexpr auto c_sclouds0 = "s_clouds0";
static constexpr auto c_sclouds1 = "s_clouds1";
