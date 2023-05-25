#pragma once

// refs
class CWayPoint;

enum
{
    estWayModeWay,
    estWayModePoint
};

class TUI_ControlWayPointAdd : public TUI_CustomControl
{
public:
    TUI_ControlWayPointAdd(int st, int act, ESceneToolBase *parent);
    virtual ~TUI_ControlWayPointAdd() { ; }
    virtual bool Start(TShiftState _Shift);
    virtual void OnEnter();
};
