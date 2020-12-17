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
    virtual ~IElevatorState() = 0;
};

inline IElevatorState::~IElevatorState() = default;
