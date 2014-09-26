#pragma once
#ifdef	DEBUG

	enum
{
	phDbgDrawContacts			=		1<<0,
	phDbgDrawEnabledAABBS		=		1<<1,
	phDBgDrawIntersectedTries	=		1<<2,
	phDbgDrawSavedTries			=		1<<3,
	phDbgDrawTriTrace			=		1<<4,
	phDBgDrawNegativeTries		=		1<<5,
	phDBgDrawPositiveTries		=		1<<6,
	phDbgDrawTriTestAABB		=		1<<7,
	phDBgDrawTriesChangesSign	=		1<<8,
	phDbgDrawTriPoint			=		1<<9,
	phDbgDrawExplosionPos		=		1<<10,
	phDbgDrawObjectStatistics	=		1<<11,
	phDbgDrawMassCenters		=		1<<12,
	phDbgDrawDeathActivationBox =		1<<14,
	phHitApplicationPoints		=		1<<15,
	phDbgDrawCashedTriesStat	=		1<<16,
	phDbgDrawCarDynamics		=		1<<17,
	phDbgDrawCarPlots			=		1<<18,
	phDbgLadder					=		1<<19,
	phDbgDrawExplosions			=		1<<20,
	phDbgDrawCarAllTrnsm		=		1<<21,
	phDbgDrawZDisable			=		1<<22,
	phDbgAlwaysUseAiPhMove		=		1<<23,
	phDbgNeverUseAiPhMove		=		1<<24,
	phDbgDispObjCollisionDammage=		1<<25,
	phDbgIK						=		1<<26,
	phDbgDrawIKGoal				=		1<<27,
	phDbgIKLimits				=		1<<28,
	phDbgCharacterControl		=		1<<29,
	phDbgDrawRayMotions			=		1<<30,
	phDbgTrackObject			=		1<<31

};
///ph_dbg_draw_mask1 ne pereputat by blin!
enum
{
	ph_m1_DbgTrackObject		=		1<<0,
	ph_m1_DbgActorRestriction	=		1<<1,
	phDbgIKOff					=		1<<2,
	phDbgHitAnims				=		1<<3,
	phDbgDrawIKLimits			=		1<<4,
	phDbgDrawIKPredict			=		1<<5,
	phDbgDrawIKSHiftObject		=		1<<6,
	phDbgDrawIKCollision		=		1<<7,
	phDbgDrawIKBlending			=		1<<8
};

enum 
{
	dbg_track_obj_blends_bp_0			= 1<< 0,
	dbg_track_obj_blends_bp_1			= 1<< 1,
	dbg_track_obj_blends_bp_2			= 1<< 2,
	dbg_track_obj_blends_bp_3			= 1<< 3,
	dbg_track_obj_blends_motion_name	= 1<< 4,
	dbg_track_obj_blends_time			= 1<< 5,
	dbg_track_obj_blends_ammount		= 1<< 6,
	dbg_track_obj_blends_mix_params		= 1<< 7,
	dbg_track_obj_blends_flags			= 1<< 8,
	dbg_track_obj_blends_state			= 1<< 9,
	dbg_track_obj_blends_dump			= 1<< 10
};
struct dContact;
class CPHObject;
class	IDebugOutput
{
public:
virtual	const	Flags32		&ph_dbg_draw_mask						()const	= 0;
virtual	const	Flags32		&ph_dbg_draw_mask1						()const = 0;


virtual	void DBG_DrawStatBeforeFrameStep( )										=0;
virtual	void DBG_DrawStatAfterFrameStep( )										=0;
//virtual	void DBG_RenderUpdate( )												=0; 
virtual	void DBG_OpenCashedDraw( )												=0;
virtual	void DBG_ClosedCashedDraw( u32 remove_time )							=0;
//virtual	void DBG_DrawPHAbstruct( SPHDBGDrawAbsract*	a )							=0;
virtual	void DBG_DrawPHObject( const CPHObject *obj )							=0;
virtual	void DBG_DrawContact ( const dContact &c )								=0;
virtual	void DBG_DrawTri( CDB::RESULT *T, u32 c )								=0;
virtual	void DBG_DrawTri(CDB::TRI *T, const Fvector *V_verts, u32 c )			=0;
virtual	void DBG_DrawLine( const Fvector &p0, const Fvector &p1, u32 c )		=0;
virtual	void DBG_DrawAABB( const Fvector &center, const Fvector& AABB, u32 c )	=0;
virtual	void DBG_DrawOBB( const Fmatrix &m, const Fvector h, u32 c )			=0;
virtual	void DBG_DrawPoint( const Fvector& p, float size, u32 c )				=0;
virtual	void DBG_DrawMatrix( const Fmatrix &m, float size, u8 a=255 )			=0;
//virtual	void DBG_DrawRotationX( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid = false, u32 tessel = 7 ) = 0;
//virtual	void DBG_DrawRotationY( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid = false, u32 tessel = 7 ) = 0;
//virtual	void DBG_DrawRotationZ( const Fmatrix &m, float ang0, float ang1, float size, u32 ac, bool solid = false, u32 tessel = 7 ) = 0;
virtual	void _cdecl DBG_OutText( LPCSTR s,... )									=0;
//virtual	void DBG_TextOutSet( float x, float y )									=0;
//virtual	void DBG_TextSetColor( u32 color )										=0;
//virtual	void DBG_DrawBind( CObject &O )											=0;
//virtual	void DBG_PhysBones( CObject &O )										=0;
//virtual	void DBG_DrawBones( CObject &O )										=0;
virtual	void DBG_DrawFrameStart( )												=0;
virtual	void PH_DBG_Render( )													=0;
virtual	void PH_DBG_Clear( )													=0;
virtual	LPCSTR PH_DBG_ObjectTrackName( )										=0;

//virtual	bool			draw_frame								()=0;
virtual	u32				&dbg_tries_num							()=0;
virtual	u32				&dbg_saved_tries_for_active_objects		()=0;
virtual	u32				&dbg_total_saved_tries					()=0;
virtual	u32 			&dbg_reused_queries_per_step			()=0;
virtual	u32 			&dbg_new_queries_per_step				()=0;
virtual	u32 			&dbg_bodies_num							()=0;
virtual	u32 			&dbg_joints_num							()=0;
virtual	u32 			&dbg_islands_num						()=0;
virtual	u32 			&dbg_contacts_num						()=0;

virtual		float		dbg_vel_collid_damage_to_display()								=0;

virtual		void DBG_ObjAfterPhDataUpdate	( CPHObject *obj )=0;
virtual		void DBG_ObjBeforePhDataUpdate	( CPHObject *obj )=0;
virtual		void DBG_ObjAfterStep			( CPHObject *obj )=0;
virtual		void DBG_ObjBeforeStep			( CPHObject *obj )=0;
virtual		void DBG_ObjeAfterPhTune		( CPHObject *obj )=0;
virtual		void DBG_ObjBeforePhTune		( CPHObject *obj )=0;
virtual		void DBG_ObjAfterCollision		( CPHObject *obj )=0;
virtual		void DBG_ObjBeforeCollision		( CPHObject *obj )=0;


};

extern XRPHYSICS_API	IDebugOutput	*ph_debug_output; 

IC	IDebugOutput	&debug_output()
{
	VERIFY(ph_debug_output);
	return *ph_debug_output;
}
 
#endif