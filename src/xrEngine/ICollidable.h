#pragma once

class ENGINE_API ICollisionForm;

class ICollidable
{
public:
    virtual ~ICollidable() = 0;
    virtual void SetCForm(ICollisionForm *cform) = 0;
    virtual ICollisionForm *GetCForm() const = 0;
};

inline ICollidable::~ICollidable() {}
// XXX: merge into CObject
class ENGINE_API CollidableBase : public ICollidable
{
public:
    CollidableBase();
    virtual ~CollidableBase();

    virtual void SetCForm(ICollisionForm *cform) override { CForm = cform; }
    virtual ICollisionForm *GetCForm() const override { return CForm; }

protected:
    ICollisionForm *CForm;
};
