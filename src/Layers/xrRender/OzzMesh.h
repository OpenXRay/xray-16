#pragma once

#include "FVisual.h"

#include <entt/entt.hpp>

namespace ozz::sample { struct Mesh; }

namespace XRay::Animation { class OzzKinematics; }

namespace xray::render::fg
{
class OzzMesh : public Fvisual
{
public:
    OzzMesh();
    ~OzzMesh() override;

    void Copy(dxRender_Visual* pFrom) override;

    void LoadFromOzzMesh(XRay::Animation::OzzKinematics* parent, const ozz::sample::Mesh& mesh);

    void SetParent(XRay::Animation::OzzKinematics* parent) { m_Parent = parent; }
    XRay::Animation::OzzKinematics* GetParent() const { return m_Parent; }

private:
    void DestroyEcsEntity();

    XRay::Animation::OzzKinematics* m_Parent = nullptr;
};
}
