#pragma once

#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"
#include "xrCore/_vector2.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_fbox.h"

namespace xray::render
{
class DeveloperSceneVertex
{
public:
    Fvector position;
    Fvector normal;
    Fvector tangent;
    Fvector binormal;
    Fvector2 uv;
};

class DeveloperSceneMaterial
{
public:
    xr_string name;
    xr_string shaderName;
    Fvector color = { 1.0f, 1.0f, 1.0f };
    float metallic = 0.0f;
    float roughness = 0.5f;
    float opacity = 1.0f;
};

class DeveloperSceneMesh
{
public:
    xr_string name;
    xr_vector<DeveloperSceneVertex> vertices;
    xr_vector<u16> indices;
    u32 material = 0;
    bool collision = true;
};

class DeveloperScene
{
public:
    xr_string name;
    Fbox bounds;
    xr_vector<DeveloperSceneMaterial> materials;
    xr_vector<DeveloperSceneMesh> meshes;
};
}
