#pragma once

enum Estate
{
    clbNone = 0,
    clbNearUp,
    clbNearDown,
    clbClimbingUp,
    clbClimbingDown,
    clbDepart,
    clbNoLadder,
    clbNoState
};
class IPhysicsShellHolder;
class IElevatorState
{
public:
    virtual Estate State() = 0;
    virtual void NetRelcase(IPhysicsShellHolder* O) = 0;

protected:
#if defined(WINDOWS)
    virtual ~IElevatorState() = 0 {}
#elif defined(LINUX)
    virtual ~IElevatorState() {}
#endif
};
