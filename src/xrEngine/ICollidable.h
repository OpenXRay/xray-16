#pragma once

class ICollisionForm;

class XR_NOVTABLE ICollidable
{
public:
    virtual ~ICollidable() = 0;
    virtual void SetCForm(ICollisionForm* cform) = 0;
    virtual ICollisionForm* GetCForm() const = 0;
};

inline ICollidable::~ICollidable() = default;

// XXX: merge into IGameObject
class ENGINE_API XR_NOVTABLE CollidableBase : public virtual ICollidable
{
public:
    CollidableBase();
    virtual ~CollidableBase();

    virtual void SetCForm(ICollisionForm* cform) override { CForm = cform; }
    virtual ICollisionForm* GetCForm() const override { return CForm; }
protected:
    ICollisionForm* CForm;
};
