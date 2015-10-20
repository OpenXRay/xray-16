// Stats.h: interface for the CStats class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STATS_H__4C8D1860_0EE2_11D4_B4E3_4854E82A090D__INCLUDED_)
#define AFX_STATS_H__4C8D1860_0EE2_11D4_B4E3_4854E82A090D__INCLUDED_
#pragma once

class ENGINE_API CGameFont;

DECLARE_MESSAGE(Stats);

class ENGINE_API CStats : public pureRender
{
public:
    CGameFont *statsFont;
    float fMem_calls;
    u32 dwMem_calls;
    //CStatTimer EngineTotal;
    //float fShedulerLoad;
    //CStatTimer Scheduler;    
    //CStatTimer Sound; // total time taken by sound subsystem (accurate only in single-threaded mode)
    //CStatTimer Input; // total time taken by input subsystem (accurate only in single-threaded mode)
    //struct ParticleStatistics
    //{
    //    u32 Starting;
    //    u32 Active;
    //    u32 Destroying;
    //} Particles;
    //struct {
    //    CStatTimer Collision; // collision
    //    CStatTimer Core; // integrate
    //    CStatTimer MovCollision; // movement+collision
    //} Physics;
    //struct {
    //    CStatTimer Think; // thinking
    //    CStatTimer Range; // query: range
    //    CStatTimer Path; // query: path
    //    CStatTimer Node; // query: node
    //    CStatTimer Vis; // visibility detection - total
    //    CStatTimer VisQuery; // visibility detection - portal traversal and frustum culling
    //    CStatTimer VisRayTests; // visibility detection - ray casting
    //} AI;
    // IRender::Stats
    //CStatTimer clRAY; // total: ray-testing
    //CStatTimer clBOX; // total: box query
    //CStatTimer clFRUSTUM; // total: frustum query
    //struct {
    //    CStatTimer ClientSend;
    //    CStatTimer ClientRecv;
    //    CStatTimer ServerUpdate;
    //    CStatTimer ClientCompressor;
    //    CStatTimer ServerCompressor;
    //} Net;
    //struct {
    //    CStatTimer Test0; // debug counter
    //    CStatTimer Test1; // debug counter
    //    CStatTimer Test2; // debug counter
    //    CStatTimer Test3; // debug counter
    //} Dbg;

    shared_str eval_line_1;
    shared_str eval_line_2;
    shared_str eval_line_3;

    void Show(void);
    virtual void OnRender();
    void OnDeviceCreate(void);
    void OnDeviceDestroy(void);
private:
    void FilteredLog(const char *s);
public:
    xr_vector <shared_str> errors;
    CRegistrator <pureStats> seqStats; // XXX: not used
public:
    CStats();
    ~CStats();

    IC CGameFont* Font() { return statsFont; }
};

enum
{
    st_sound = (1 << 0),
    st_sound_min_dist = (1 << 1),
    st_sound_max_dist = (1 << 2),
    st_sound_ai_dist = (1 << 3),
    st_sound_info_name = (1 << 4),
    st_sound_info_object = (1 << 5),
};

extern Flags32 g_stats_flags;

#endif // !defined(AFX_STATS_H__4C8D1860_0EE2_11D4_B4E3_4854E82A090D__INCLUDED_)
