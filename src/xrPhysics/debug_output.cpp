#include "StdAfx.h"
#include "debug_output.h"
#ifdef DEBUG

static class DebugOutputEmptyImpl : public IDebugOutput
{
    Flags32 m1;
    Flags32 m2;

    virtual const Flags32& ph_dbg_draw_mask() const { return m1; };
    virtual const Flags32& ph_dbg_draw_mask1() const { return m2; }
    virtual void DBG_DrawStatBeforeFrameStep() {}
    virtual void DBG_DrawStatAfterFrameStep() {}
    // virtual	void DBG_RenderUpdate( )												=0;
    virtual void DBG_OpenCashedDraw() {}
    virtual void DBG_ClosedCashedDraw(u32 remove_time) {}
    // virtual	void DBG_DrawPHAbstruct( SPHDBGDrawAbsract*	a )							=0;
    virtual void DBG_DrawPHObject(const CPHObject* obj) {}
    virtual void DBG_DrawContact(const dContact& c) {}
    virtual void DBG_DrawTri(CDB::RESULT* T, u32 c) {}
    virtual void DBG_DrawTri(CDB::TRI* T, const Fvector* V_verts, u32 c) {}
    virtual void DBG_DrawLine(const Fvector& p0, const Fvector& p1, u32 c) {}
    virtual void DBG_DrawAABB(const Fvector& center, const Fvector& AABB, u32 c) {}
    virtual void DBG_DrawOBB(const Fmatrix& m, const Fvector h, u32 c) {}
    virtual void DBG_DrawPoint(const Fvector& p, float size, u32 c) {}
    virtual void DBG_DrawMatrix(const Fmatrix& m, float size, u8 a = 255) {}
    // virtual	void DBG_DrawRotationX( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    // virtual	void DBG_DrawRotationY( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    // virtual	void DBG_DrawRotationZ( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid =
    // false,
    // u32 tessel = 7 ) = 0;
    virtual void _cdecl DBG_OutText(LPCSTR s, ...) {}
    // virtual	void DBG_TextOutSet( float x, float y )									=0;
    // virtual	void DBG_TextSetColor( u32 color )										=0;
    // virtual	void DBG_DrawBind( IGameObject &O )											=0;
    // virtual	void DBG_PhysBones( IGameObject &O )										=0;
    // virtual	void DBG_DrawBones( IGameObject &O )										=0;
    virtual void DBG_DrawFrameStart() {}
    virtual void PH_DBG_Render() {}
    virtual void PH_DBG_Clear() {}
    virtual LPCSTR PH_DBG_ObjectTrackName() { return "none"; }
    // virtual	bool			draw_frame								()=0;
    u32 tries_num;
    virtual u32& dbg_tries_num() { return tries_num; }
    u32 saved_tries_for_active_objects;
    virtual u32& dbg_saved_tries_for_active_objects() { return saved_tries_for_active_objects; }
    u32 total_saved_tries;
    virtual u32& dbg_total_saved_tries() { return total_saved_tries; }
    u32 reused_queries_per_step;
    virtual u32& dbg_reused_queries_per_step() { return reused_queries_per_step; }
    u32 new_queries_per_step;
    virtual u32& dbg_new_queries_per_step() { return new_queries_per_step; }
    u32 bodies_num;
    virtual u32& dbg_bodies_num() { return bodies_num; }
    u32 joints_num;
    virtual u32& dbg_joints_num() { return joints_num; }
    u32 islands_num;
    virtual u32& dbg_islands_num() { return islands_num; }
    u32 contacts_num;
    virtual u32& dbg_contacts_num() { return contacts_num; }
    float vel_collid_damage_to_display;
    virtual float dbg_vel_collid_damage_to_display() { return vel_collid_damage_to_display; }
    virtual void DBG_ObjAfterPhDataUpdate(CPHObject* obj) {}
    virtual void DBG_ObjBeforePhDataUpdate(CPHObject* obj) {}
    virtual void DBG_ObjAfterStep(CPHObject* obj) {}
    virtual void DBG_ObjBeforeStep(CPHObject* obj) {}
    virtual void DBG_ObjeAfterPhTune(CPHObject* obj) {}
    virtual void DBG_ObjBeforePhTune(CPHObject* obj) {}
    virtual void DBG_ObjAfterCollision(CPHObject* obj) {}
    virtual void DBG_ObjBeforeCollision(CPHObject* obj) {}
} dbg_output_empty;

IDebugOutput* ph_debug_output = &dbg_output_empty;
#endif
