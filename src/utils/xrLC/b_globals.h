#pragma once

const u32 c_VB_maxVertices = 65535; // count
const u32 c_vCacheSize = 24; // entries
const u32 c_SS_LowVertLimit = 64; // polys
const u32 c_SS_HighVertLimit = 2 * 1024; // polys
const u32 c_SS_maxsize = 32; // meters
const u32 c_PM_FaceLimit = 128; // face-limit
const float c_PM_MetricLimit_static = 0.10f; // vertex-count-simplification-limit
const float c_PM_MetricLimit_mu = 0.05f; // vertex-count-simplification-limit

// XXX: The following two 'exact_normalize' are also declared in xrCore/_vector3d.h
bool exact_normalize(Fvector3& a);
bool exact_normalize(float* a);

struct SBuildOptions
{
    BOOL b_skipinvalid;
    BOOL b_radiosity;
    BOOL b_noise;
    BOOL b_net_light;
    SBuildOptions() : b_skipinvalid(FALSE), b_radiosity(FALSE), b_noise(FALSE), b_net_light(FALSE) {}
};
extern SBuildOptions g_build_options;
