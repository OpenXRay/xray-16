#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
inline float pack_model_hemi(float positive, float negative)
{
    if (positive < 0) return -1.f;
    return std::round(std::clamp(positive,0.f,1.f)*1023.f) +
        std::round(std::clamp(negative,0.f,1.f)*1023.f)/1024.f;
}

inline std::array<float,4> model_rotation_quaternion(const float* rows)
{
    if (!rows) return {0,0,0,1};
    float m[3][3];
    for (unsigned col=0;col<3;++col)
    {
        const float length=std::sqrt(rows[col]*rows[col]+rows[4+col]*rows[4+col]+rows[8+col]*rows[8+col]);
        if (!std::isfinite(length) || length < 1e-6f) return {0,0,0,1};
        for (unsigned row=0;row<3;++row) m[row][col]=rows[row*4+col]/length;
    }
    std::array<float,4> q{};
    const float trace=m[0][0]+m[1][1]+m[2][2];
    if (trace>0)
    {
        const float s=std::sqrt(trace+1.f)*2.f;
        q={ (m[2][1]-m[1][2])/s,(m[0][2]-m[2][0])/s,(m[1][0]-m[0][1])/s,s*.25f };
    }
    else
    {
        unsigned i=m[1][1]>m[0][0]?1:0;if(m[2][2]>m[i][i])i=2;
        const unsigned j=(i+1)%3,k=(i+2)%3;
        const float s=std::sqrt(std::max(1.f+m[i][i]-m[j][j]-m[k][k],0.f))*2.f;
        if(s<1e-6f)return {0,0,0,1};
        q[i]=s*.25f;q[j]=(m[j][i]+m[i][j])/s;q[k]=(m[k][i]+m[i][k])/s;
        q[3]=(m[k][j]-m[j][k])/s;
    }
    return q;
}
// One extra mat4 follows the bones in the pose SSBO. Its upper 3x3 rotates
// normals/basis vectors; the remaining components carry six hemi faces.
inline std::array<float, 16> model_pose_constants(const float* rows, const float* negative)
{
    std::array<float,16> result{};
    const float identity[]{1,0,0,-1, 0,1,0,0, 0,0,1,0};
    if (!rows) rows=identity;
    for (unsigned row=0; row<3; ++row)
    {
        for (unsigned col=0; col<3; ++col) result[col*4+row]=rows[row*4+col];
        result[row*4+3]=rows[row*4+3];
        result[12+row]=negative ? negative[row] : 0.f;
    }
    return result;
}
}
