#pragma once

#include "xrCore/_matrix.h"

#include <ozz/base/maths/quaternion.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>

#include <array>

namespace XRay
{
namespace Animation
{
namespace detail
{
using Matrix4 = std::array<std::array<float, 4>, 4>;

constexpr Matrix4 kXrayToOzz = {
    std::array<float, 4>{ -1.f, 0.f,  0.f, 0.f },
    std::array<float, 4>{ 0.f, 1.f,  0.f, 0.f },
    std::array<float, 4>{ 0.f, 0.f, -1.f, 0.f },
    std::array<float, 4>{ 0.f, 0.f,  0.f, 1.f }
};

constexpr Matrix4 kOzzToXray = kXrayToOzz;

inline Matrix4 Multiply(const Matrix4& lhs, const Matrix4& rhs)
{
    Matrix4 result{};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float value = 0.f;
            for (int k = 0; k < 4; ++k)
                value += lhs[static_cast<size_t>(row)][static_cast<size_t>(k)] *
                         rhs[static_cast<size_t>(k)][static_cast<size_t>(col)];
            result[static_cast<size_t>(row)][static_cast<size_t>(col)] = value;
        }
    }
    return result;
}

inline Matrix4 ChangeBasis(const Matrix4& matrix, const Matrix4& basis, const Matrix4& basis_inverse)
{
    return Multiply(Multiply(basis, matrix), basis_inverse);
}

inline ozz::math::Float3 ExtractTranslation(const Matrix4& matrix)
{
    return { matrix[0][3], matrix[1][3], matrix[2][3] };
}

inline ozz::math::Quaternion ExtractQuaternion(const Matrix4& matrix)
{
    const float m00 = matrix[0][0];
    const float m01 = matrix[0][1];
    const float m02 = matrix[0][2];
    const float m10 = matrix[1][0];
    const float m11 = matrix[1][1];
    const float m12 = matrix[1][2];
    const float m20 = matrix[2][0];
    const float m21 = matrix[2][1];
    const float m22 = matrix[2][2];

    float qw = 0.f;
    float qx = 0.f;
    float qy = 0.f;
    float qz = 0.f;

    const float trace = m00 + m11 + m22;
    if (trace > 0.f)
    {
        const float s = std::sqrt(trace + 1.f) * 2.f;
        qw = 0.25f * s;
        qx = (m21 - m12) / s;
        qy = (m02 - m20) / s;
        qz = (m10 - m01) / s;
    }
    else if (m00 > m11 && m00 > m22)
    {
        const float s = std::sqrt(1.f + m00 - m11 - m22) * 2.f;
        qw = (m21 - m12) / s;
        qx = 0.25f * s;
        qy = (m01 + m10) / s;
        qz = (m02 + m20) / s;
    }
    else if (m11 > m22)
    {
        const float s = std::sqrt(1.f + m11 - m00 - m22) * 2.f;
        qw = (m02 - m20) / s;
        qx = (m01 + m10) / s;
        qy = 0.25f * s;
        qz = (m12 + m21) / s;
    }
    else
    {
        const float s = std::sqrt(1.f + m22 - m00 - m11) * 2.f;
        qw = (m10 - m01) / s;
        qx = (m02 + m20) / s;
        qy = (m12 + m21) / s;
        qz = 0.25f * s;
    }

    const float length_sq = qx * qx + qy * qy + qz * qz + qw * qw;
    const float inv_length = length_sq > 0.f ? 1.f / std::sqrt(length_sq) : 1.f;
    return ozz::math::Quaternion(qx * inv_length, qy * inv_length, qz * inv_length, qw * inv_length);
}

inline Matrix4 LoadOzzMatrix(const ozz::math::Float4x4& matrix)
{
    Matrix4 result{};
    for (int col = 0; col < 4; ++col)
    {
        float column[4];
        ozz::math::StorePtrU(matrix.cols[col], column);
        for (int row = 0; row < 4; ++row)
            result[static_cast<size_t>(row)][static_cast<size_t>(col)] = column[row];
    }
    return result;
}

inline Fmatrix ToFmatrix(const Matrix4& matrix)
{
    Fmatrix result;
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result.m[col][row] = matrix[static_cast<size_t>(row)][static_cast<size_t>(col)];
    return result;
}

inline Matrix4 FromFmatrix(const Fmatrix& matrix)
{
    Matrix4 result{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            result[static_cast<size_t>(row)][static_cast<size_t>(col)] = matrix.m[col][row];
    return result;
}

inline std::array<float, 3> ApplyBasis(const Matrix4& basis, const std::array<float, 3>& vector)
{
    std::array<float, 3> result{};
    for (int row = 0; row < 3; ++row)
    {
        result[static_cast<size_t>(row)] = basis[static_cast<size_t>(row)][0] * vector[0] +
            basis[static_cast<size_t>(row)][1] * vector[1] +
            basis[static_cast<size_t>(row)][2] * vector[2];
    }
    return result;
}
} // namespace detail

inline Fmatrix ConvertOzzMatrixToXRay(const ozz::math::Float4x4& matrix)
{
    const auto ozz = detail::LoadOzzMatrix(matrix);
    const auto xray = detail::ChangeBasis(ozz, detail::kOzzToXray, detail::kXrayToOzz);
    return detail::ToFmatrix(xray);
}

inline ozz::math::Float4x4 ConvertXRayMatrixToOzz(const Fmatrix& matrix)
{
    const auto xray = detail::FromFmatrix(matrix);
    const auto ozz_matrix = detail::ChangeBasis(xray, detail::kXrayToOzz, detail::kOzzToXray);

    ozz::math::Float4x4 result;
    for (int col = 0; col < 4; ++col)
    {
        float column[4];
        for (int row = 0; row < 4; ++row)
            column[row] = ozz_matrix[static_cast<size_t>(row)][static_cast<size_t>(col)];
        result.cols[col] = ozz::math::simd_float4::LoadPtrU(column);
    }
    return result;
}

inline ozz::math::Float4x4 RestSoaToOzzMatrix(const ozz::math::SoaTransform& rest, int lane)
{
    float tx[4], ty[4], tz[4];
    float qx[4], qy[4], qz[4], qw[4];
    float sx[4], sy[4], sz[4];

    ozz::math::StorePtrU(rest.translation.x, tx);
    ozz::math::StorePtrU(rest.translation.y, ty);
    ozz::math::StorePtrU(rest.translation.z, tz);
    ozz::math::StorePtrU(rest.rotation.x, qx);
    ozz::math::StorePtrU(rest.rotation.y, qy);
    ozz::math::StorePtrU(rest.rotation.z, qz);
    ozz::math::StorePtrU(rest.rotation.w, qw);
    ozz::math::StorePtrU(rest.scale.x, sx);
    ozz::math::StorePtrU(rest.scale.y, sy);
    ozz::math::StorePtrU(rest.scale.z, sz);

    const ozz::math::Float3 translation(tx[lane], ty[lane], tz[lane]);
    const ozz::math::Quaternion rotation(qx[lane], qy[lane], qz[lane], qw[lane]);
    const ozz::math::Float3 scale(sx[lane], sy[lane], sz[lane]);

    return ozz::math::Float4x4::FromAffine(translation, rotation, scale);
}

inline Fmatrix RestSoaToXRayMatrix(const ozz::math::SoaTransform& rest, int lane)
{
    return ConvertOzzMatrixToXRay(RestSoaToOzzMatrix(rest, lane));
}

inline ozz::math::Float3 ExtractTranslation(const ozz::math::Float4x4& matrix)
{
    return detail::ExtractTranslation(detail::LoadOzzMatrix(matrix));
}

inline ozz::math::Quaternion ExtractQuaternion(const ozz::math::Float4x4& matrix)
{
    return detail::ExtractQuaternion(detail::LoadOzzMatrix(matrix));
}

inline ozz::math::Float3 ExtractTranslation(const Fmatrix& matrix)
{
    return detail::ExtractTranslation(detail::FromFmatrix(matrix));
}

inline ozz::math::Quaternion ExtractQuaternion(const Fmatrix& matrix)
{
    return detail::ExtractQuaternion(detail::FromFmatrix(matrix));
}
} // namespace Animation
} // namespace XRay
