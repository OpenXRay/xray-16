#pragma once

#include "xrCore/xrCore.h"

namespace XRay
{
namespace ManagedApi
{
namespace Core
{
using namespace System;
using System::Runtime::InteropServices::OutAttribute;
using System::Runtime::InteropServices::StructLayoutAttribute;
using System::Runtime::InteropServices::LayoutKind;
using System::Runtime::InteropServices::FieldOffsetAttribute;

[StructLayout(LayoutKind::Sequential, Size = sizeof(Fcolor))] public value struct ColorF
{
public:
    float r, g, b, a;
};

[StructLayout(LayoutKind::Sequential, Size = sizeof(Fvector2))] public value struct Vector2F
{
    float x, y;
};

[StructLayout(LayoutKind::Sequential, Size = sizeof(Fvector3))] public value struct Vector3F
{
    float x, y, z;
};

//[FieldOffset(offsetof(Fvector4, x))]
[StructLayout(LayoutKind::Sequential, Size = sizeof(Fvector4))] public value struct Vector4F
{
    float x, y, z, w;
};

[StructLayout(LayoutKind::Sequential, Size = sizeof(Ivector2))] public value struct Vector2I
{
    int x, y;
};

[StructLayout(LayoutKind::Sequential, Size = sizeof(Ivector3))] public value struct Vector3I
{
    int x, y, z;
};

[StructLayout(LayoutKind::Sequential, Size = sizeof(Ivector4))] public value struct Vector4I
{
    int x, y, z, w;
};
}
}
}
