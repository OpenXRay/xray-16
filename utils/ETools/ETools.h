#ifndef EToolsH
#define EToolsH

#ifdef ETOOLS_EXPORTS
#define ETOOLS_API __declspec( dllexport )
#else
#define ETOOLS_API __declspec( dllimport )
#endif

#include "../../xrCDB/xrCDB.h"
class IKinematics;
extern "C" {
	// fast functions
	namespace ETOOLS{
		ETOOLS_API bool 				__stdcall	TestRayTriA			(const Fvector& C, const Fvector& D, Fvector** p, float& u, float& v, float& range, bool bCull);
		ETOOLS_API bool 				__stdcall	TestRayTriB			(const Fvector& C, const Fvector& D, Fvector* p, float& u, float& v, float& range, bool bCull);
		ETOOLS_API bool 				__stdcall	TestRayTri2			(const Fvector& C, const Fvector& D, Fvector* p, float& range);
		
		typedef  void					__stdcall	pb_callback			(void* user_data, float& val);
		ETOOLS_API void					__stdcall	SimplifyCubeMap		(u32* src_data, u32 src_width, u32 src_height, u32* dst_data, u32 dst_width, u32 dst_height, float sample_factor=1.f, pb_callback cb=0, void* pb_data=0);

		ETOOLS_API CDB::Collector*		__stdcall	create_collector	();
		ETOOLS_API void					__stdcall	destroy_collector	(CDB::Collector*&);
		ETOOLS_API void					__stdcall	collector_add_face_d(CDB::Collector* CL, const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy);
		ETOOLS_API void					__stdcall	collector_add_face_pd(CDB::Collector* CL,const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy, float eps = EPS);
		ETOOLS_API CDB::CollectorPacked*__stdcall	create_collectorp	(const Fbox &bb, int apx_vertices=5000, int apx_faces=5000);
		ETOOLS_API void					__stdcall	destroy_collectorp	(CDB::CollectorPacked*&);
		ETOOLS_API void					__stdcall	collectorp_add_face_d(CDB::CollectorPacked* CL, const Fvector& v0, const Fvector& v1, const Fvector& v2, u32 dummy);

		ETOOLS_API CDB::COLLIDER*		__stdcall	get_collider		();
		ETOOLS_API CDB::MODEL*			__stdcall	create_model_cl		(CDB::Collector*);
		ETOOLS_API CDB::MODEL*			__stdcall	create_model_clp	(CDB::CollectorPacked*);
		ETOOLS_API CDB::MODEL*			__stdcall	create_model		(Fvector* V, int Vcnt, CDB::TRI* T, int Tcnt);
		ETOOLS_API void					__stdcall	destroy_model		(CDB::MODEL*&);
		ETOOLS_API CDB::RESULT*			__stdcall	r_begin				();
		ETOOLS_API CDB::RESULT*			__stdcall	r_end				(); 
		ETOOLS_API int					__stdcall	r_count				();
		ETOOLS_API void					__stdcall	ray_options			(u32 flags);
		ETOOLS_API void					__stdcall	ray_query			(const CDB::MODEL *m_def, const Fvector& r_start,  const Fvector& r_dir, float r_range);
		ETOOLS_API void					__stdcall	ray_query_m			(const Fmatrix& inv_parent, const CDB::MODEL *m_def, const Fvector& r_start,  const Fvector& r_dir, float r_range);
		ETOOLS_API void					__stdcall	box_options			(u32 flags);
		ETOOLS_API void					__stdcall	box_query			(const CDB::MODEL *m_def, const Fvector& b_center, const Fvector& b_dim);
		ETOOLS_API void					__stdcall	box_query_m			(const Fmatrix& inv_parent, const CDB::MODEL *m_def, const Fbox& src);

		ETOOLS_API int					__stdcall	ogg_enc				(const char* in_fn, const char* out_fn, float quality, void* comment, int comment_size);
		
		
		ETOOLS_API bool					__stdcall	intersect			(const Fmatrix &object_transform, const IKinematics& K, const Fvector& origin, const Fvector &direction, u16 &bone_id,  float &dist, Fvector &norm);
	};
};

#endif // EToolsH