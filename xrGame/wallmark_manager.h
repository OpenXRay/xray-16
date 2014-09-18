#pragma once



// Igor DEFINE_VECTOR(ref_shader, SHADER_VECTOR, SHADER_VECTOR_IT);
class CWalmarkManager
{
private:
	FactoryPtr<IWallMarkArray>		m_wallmarks;
	Fvector							m_pos;
public:
	CObject*						m_owner;
				CWalmarkManager		()																																					;
				~CWalmarkManager	()																																					;
		void	Load				(LPCSTR section)																																	;
		void	Clear				()																																					;
		void	AddWallmark			(const Fvector& dir, const Fvector& start_pos, float range, float wallmark_size,IWallMarkArray &wallmarks_vector,int t);
		void	PlaceWallmarks		( const Fvector& start_pos);
		
		void	__stdcall StartWorkflow	();
};