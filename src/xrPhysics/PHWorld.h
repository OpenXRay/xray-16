#pragma once

#include "Physics.h"
#include "PHUpdateObject.h"
#include "IPHWorld.h"
#include "Common/Noncopyable.hpp"
#include "physics_scripted.h"
#include "xrEngine/pure.h"
// refs
struct SGameMtlPair;
// class	CPHCommander;
// class	CPHCondition;
// class	CPHAction;
struct SPHNetState;
class CPHSynchronize;
typedef xr_vector<std::pair<CPHSynchronize*, SPHNetState>> V_PH_WORLD_STATE;
class CPHMesh
{
    dGeomID Geom;

public:
    dGeomID GetGeom() { return Geom; }
    void Create(dSpaceID space, dWorldID world);
    void Destroy();
};

#define PHWORLD_SOUND_CACHE_SIZE 8

////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CObjectSpace;
class CObjectList;
class CPHWorld : public pureFrame,
                 public IPHWorld,
                 public cphysics_scripted,
                 private Noncopyable
#ifdef DEBUG
                 ,
                 public pureRender
#endif
{
private:
    PHWorldStatistics stats;
    double m_start_time;
    u32 m_delay;
    u32 m_previous_delay;
    u32 m_reduce_delay;
    u32 m_update_delay_count;
    bool b_world_freezed;
    bool b_processing;
    bool b_exist;
    static const u32 update_delay = 1;
    ///	dSpaceID					Space														;

    CPHMesh Mesh;
    PH_OBJECT_STORAGE m_objects;
    PH_OBJECT_STORAGE m_freezed_objects;
    PH_OBJECT_STORAGE m_recently_disabled_objects;
    PH_UPDATE_OBJECT_STORAGE m_update_objects;
    PH_UPDATE_OBJECT_STORAGE m_freezed_update_objects;
    dGeomID m_motion_ray;
    // CPHCommander				*m_commander;
    IPHWorldUpdateCallbck* m_update_callback;
    CObjectSpace* m_object_space;
    CObjectList* m_level_objects;
    CRenderDeviceBase* m_device;
    ;

public:
    xr_vector<ISpatial*> r_spatial;

public:
    u64 m_steps_num;

private:
    u16 m_steps_short_num;

public:
    double m_frame_sum;
    dReal m_previous_frame_time;
    bool b_frame_mark;
    dReal m_frame_time;
    float m_update_time;
    u16 disable_count;
    float m_gravity;

private:
    ContactCallbackFun* m_default_contact_shotmark;
    ContactCallbackFun* m_default_character_contact_shotmark;
    PhysicsStepTimeCallback* physics_step_time_callback;

public:
    CPHWorld();
    virtual ~CPHWorld(){};

    // IC	dSpaceID					GetSpace						()			{return Space;}	;
    IC bool Exist() { return b_exist; }
    void Create(bool mt, CObjectSpace* os, CObjectList* lo, CRenderDeviceBase* dv);
    void SetGravity(float g);
    IC float Gravity() { return m_gravity; }
    void AddObject(CPHObject* object);
    void AddUpdateObject(CPHUpdateObject* object);
    void AddRecentlyDisabled(CPHObject* object);
    void RemoveFromRecentlyDisabled(PH_OBJECT_I i);
    void RemoveObject(PH_OBJECT_I i);
    void RemoveUpdateObject(PH_UPDATE_OBJECT_I i);
    dGeomID GetMeshGeom() { return Mesh.GetGeom(); }
    IC dGeomID GetMotionRayGeom() { return m_motion_ray; }
    void SetStep(float s);
    void Destroy();
    IC float FrameTime(bool frame_mark) { return b_frame_mark == frame_mark ? m_frame_time : m_previous_frame_time; }
    void FrameStep(dReal step = 0.025f);
    void Step();
    void StepTouch();
    void CutVelocity(float l_limit, float a_limit);
    void GetState(V_PH_WORLD_STATE& state);
    void Freeze();
    void UnFreeze();
    void AddFreezedObject(CPHObject* obj);
    void RemoveFreezedObject(PH_OBJECT_I i);
    bool IsFreezed();
    IC bool Processing() { return b_processing; }
    u32 CalcNumSteps(u32 dTime);
    u16 ObjectsNumber();
    u16 UpdateObjectsNumber();
    IC u16 StepsShortCnt() { return m_steps_short_num; }
    u64& StepsNum() { return m_steps_num; }
    float FrameTime() { return m_frame_time; }
    ContactCallbackFun* default_contact_shotmark() { return m_default_contact_shotmark; }
    ContactCallbackFun* default_character_contact_shotmark() { return m_default_character_contact_shotmark; }
    void set_default_contact_shotmark(ContactCallbackFun* f) { m_default_contact_shotmark = f; }
    void set_default_character_contact_shotmark(ContactCallbackFun* f) { m_default_character_contact_shotmark = f; }
    void NetRelcase(CPhysicsShell* s);
    CObjectSpace& ObjectSpace()
    {
        VERIFY(m_object_space);
        return *m_object_space;
    }
    CObjectList& LevelObjects()
    {
        VERIFY(m_level_objects);
        return *m_level_objects;
    }
    CRenderDeviceBase& Device()
    {
        VERIFY(m_device);
        return *m_device;
    }

//	void						AddCall							(CPHCondition*c,CPHAction*a);
#ifdef DEBUG
    virtual void OnRender();
#endif
    virtual void OnFrame();
    virtual const PHWorldStatistics& GetStats() override { return stats; }
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

private:
    void StepNumIterations(int num_it);
    iphysics_scripted& get_scripted() { return *this; }
    void set_step_time_callback(PhysicsStepTimeCallback* cb) { physics_step_time_callback = cb; }
    void set_update_callback(IPHWorldUpdateCallbck* cb)
    {
        VERIFY(cb);
        m_update_callback = cb;
    }
};
extern XRPHYSICS_API CPHWorld* ph_world;
IC CPHWorld& inl_ph_world() { return *ph_world; }
