#pragma once

#include "xrCore/FTimer.h"
#include "xrCDB/xrCDB.h" // build_callback
#include "PhysicsExternalCommon.h"

class CObjectSpace;
class CObjectList;
class CGameMtlLibrary;
class CPhysicsShell;
class CPHCondition;
class CPHAction;

class IPHWorld
{
public:
    struct PHWorldStatistics
    {
        CStatTimer Collision; // collision
        CStatTimer Core; // integrate
        CStatTimer MovCollision; // movement+collision

        PHWorldStatistics() { FrameStart(); }
        void FrameStart()
        {
            Collision.FrameStart();
            Core.FrameStart();
            MovCollision.FrameStart();
        }

        void FrameEnd()
        {
            Collision.FrameEnd();
            Core.FrameEnd();
            MovCollision.FrameEnd();
        }
    };

    virtual ~IPHWorld() {}
    virtual float Gravity() = 0;
    virtual void SetGravity(float g) = 0;
    virtual bool Processing() = 0;
    virtual u32 CalcNumSteps(u32 dTime) = 0;
    virtual u64& StepsNum() = 0;

    virtual float FrameTime() = 0;
    virtual void Freeze() = 0;
    virtual void UnFreeze() = 0;
    virtual void Step() = 0;
    virtual void SetStep(float s) = 0;
    virtual void StepNumIterations(int num_it) = 0;
    virtual void set_default_contact_shotmark(ObjectContactCallbackFun* f) = 0;
    virtual void set_default_character_contact_shotmark(ObjectContactCallbackFun* f) = 0;
    virtual void set_step_time_callback(PhysicsStepTimeCallback* cb) = 0;
    virtual void AddCall(CPHCondition* c, CPHAction* a) = 0;
    virtual const PHWorldStatistics& GetStats() = 0;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;
#ifdef DEBUG
    virtual u16 ObjectsNumber() = 0;
    virtual u16 UpdateObjectsNumber() = 0;
    virtual void OnRender() = 0;
#endif
};

XRPHYSICS_API IPHWorld* physics_world();
XRPHYSICS_API void create_physics_world(bool mt, CObjectSpace* os, CObjectList* lo);
XRPHYSICS_API void destroy_physics_world();
XRPHYSICS_API void destroy_object_space(CObjectSpace*& os);
