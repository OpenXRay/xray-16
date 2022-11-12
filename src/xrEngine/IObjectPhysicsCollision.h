#pragma once

class IPhysicsShell;
class IPhysicsElement;
class IObjectPhysicsCollision
{
public:
    virtual ~IObjectPhysicsCollision() = default;
    virtual const IPhysicsShell* physics_shell() const = 0;
    virtual const IPhysicsElement* physics_character() const = 0; // depricated
};
