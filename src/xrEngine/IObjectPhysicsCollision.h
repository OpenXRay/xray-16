#pragma once

class IPhysicsShell;
class IPhysicsElement;
#if defined(WINDOWS)
xr_pure_interface IObjectPhysicsCollision
{
public:
    virtual const IPhysicsShell* physics_shell() const = 0;
    virtual const IPhysicsElement* physics_character() const = 0; // depricated
};
#endif
