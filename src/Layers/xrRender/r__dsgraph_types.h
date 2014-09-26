#pragma once

#include "../../xrCore/fixedmap.h"

//#ifndef USE_MEMORY_MONITOR
#	define USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
//#endif // USE_MEMORY_MONITOR

#ifdef USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#	include	"../../xrCore/doug_lea_allocator.h"
	extern doug_lea_allocator	g_render_lua_allocator;

	template <class T>
	class doug_lea_alloc {
	public:
		typedef	size_t		size_type;
		typedef ptrdiff_t	difference_type;
		typedef T*			pointer;
		typedef const T*	const_pointer;
		typedef T&			reference;
		typedef const T&	const_reference;
		typedef T			value_type;

	public:
		template<class _Other>	
		struct rebind			{	typedef doug_lea_alloc<_Other> other;	};
	public:
								pointer					address			(reference _Val) const					{	return (&_Val);	}
								const_pointer			address			(const_reference _Val) const			{	return (&_Val);	}
														doug_lea_alloc	()										{	}
														doug_lea_alloc	(const doug_lea_alloc<T>&)				{	}
		template<class _Other>							doug_lea_alloc	(const doug_lea_alloc<_Other>&)			{	}
		template<class _Other>	doug_lea_alloc<T>&		operator=		(const doug_lea_alloc<_Other>&)			{	return (*this);	}
								pointer					allocate		(size_type n, const void* p=0) const	{	return (T*)g_render_lua_allocator.malloc_impl(sizeof(T)*(u32)n);	}
								void					deallocate		(pointer p, size_type n) const			{	g_render_lua_allocator.free_impl	((void*&)p);				}
								void					deallocate		(void* p, size_type n) const			{	g_render_lua_allocator.free_impl	(p);				}
								char*					__charalloc		(size_type n)							{	return (char*)allocate(n); }
								void					construct		(pointer p, const T& _Val)				{	std::_Construct(p, _Val);	}
								void					destroy			(pointer p)								{	std::_Destroy(p);			}
								size_type				max_size		() const								{	size_type _Count = (size_type)(-1) / sizeof (T);	return (0 < _Count ? _Count : 1);	}
	};

	template<class _Ty,	class _Other>	inline	bool operator==(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)		{	return (true);							}
	template<class _Ty, class _Other>	inline	bool operator!=(const doug_lea_alloc<_Ty>&, const doug_lea_alloc<_Other>&)		{	return (false);							}

	struct doug_lea_allocator_wrapper {
		template <typename T>
		struct helper {
			typedef doug_lea_alloc<T>	result;
		};

		static	void	*alloc		(const u32 &n)	{	return g_render_lua_allocator.malloc_impl((u32)n);	}
		template <typename T>
		static	void	dealloc		(T *&p)			{	g_render_lua_allocator.free_impl((void*&)p);	}
	};

#	define render_alloc				doug_lea_alloc
	typedef doug_lea_allocator_wrapper	render_allocator;

#else // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER
#	define render_alloc				xalloc
	typedef xr_allocator			render_allocator;
#endif // USE_DOUG_LEA_ALLOCATOR_FOR_RENDER

class dxRender_Visual;

// #define	USE_RESOURCE_DEBUGGER

namespace	R_dsgraph
{
	// Elementary types
	struct _NormalItem	{
		float				ssa;
		dxRender_Visual*		pVisual;
	};

	struct _MatrixItem	{
		float				ssa;
		IRenderable*		pObject;
		dxRender_Visual*		pVisual;
		Fmatrix				Matrix;				// matrix (copy)
	};

	struct _MatrixItemS	: public _MatrixItem
	{
		ShaderElement*		se;
	};

	struct _LodItem		{
		float				ssa;
		dxRender_Visual*		pVisual;
	};

#ifdef USE_RESOURCE_DEBUGGER
	typedef	ref_vs						vs_type;
	typedef	ref_ps						ps_type;
#	if defined(USE_DX10) || defined(USE_DX11)
		typedef	ref_gs						gs_type;
#		ifdef USE_DX11
		typedef	ref_hs						hs_type;
		typedef	ref_ds						ds_type;
#		endif
#	endif	//	USE_DX10
#else
	#if defined(USE_DX10) || defined(USE_DX11)	//	DX10 needs shader signature to propperly bind deometry to shader
		typedef	SVS*					vs_type;
		typedef	ID3DGeometryShader*		gs_type;
		#ifdef USE_DX11
			typedef	ID3D11HullShader*		hs_type;
			typedef	ID3D11DomainShader*		ds_type;
		#endif
	#else	//	USE_DX10
		typedef	ID3DVertexShader*		vs_type;
	#endif	//	USE_DX10
		typedef	ID3DPixelShader*		ps_type;
#endif

	// NORMAL
	typedef xr_vector<_NormalItem,render_allocator::helper<_NormalItem>::result>			mapNormalDirect;
	struct	mapNormalItems		: public	mapNormalDirect										{	float	ssa;	};
	struct	mapNormalTextures	: public	FixedMAP<STextureList*,mapNormalItems,render_allocator>				{	float	ssa;	};
	struct	mapNormalStates		: public	FixedMAP<ID3DState*,mapNormalTextures,render_allocator>	{	float	ssa;	};
	struct	mapNormalCS			: public	FixedMAP<R_constant_table*,mapNormalStates,render_allocator>			{	float	ssa;	};
#ifdef USE_DX11
	struct	mapNormalAdvStages
	{
		hs_type		hs;
		ds_type		ds;
		mapNormalCS	mapCS;
	};
	struct	mapNormalPS			: public	FixedMAP<ps_type, mapNormalAdvStages,render_allocator>						{	float	ssa;	};
#else
	struct	mapNormalPS			: public	FixedMAP<ps_type, mapNormalCS,render_allocator>						{	float	ssa;	};
#endif	//	USE_DX11
#if defined(USE_DX10) || defined(USE_DX11)
	struct	mapNormalGS			: public	FixedMAP<gs_type, mapNormalPS,render_allocator>						{	float	ssa;	};
	struct	mapNormalVS			: public	FixedMAP<vs_type, mapNormalGS,render_allocator>						{	};
#else	//	USE_DX10
	struct	mapNormalVS			: public	FixedMAP<vs_type, mapNormalPS,render_allocator>						{	};
#endif	//	USE_DX10
	typedef mapNormalVS			mapNormal_T;
	typedef mapNormal_T			mapNormalPasses_T[SHADER_PASSES_MAX];

	// MATRIX
	typedef xr_vector<_MatrixItem,render_allocator::helper<_MatrixItem>::result>	mapMatrixDirect;
	struct	mapMatrixItems		: public	mapMatrixDirect										{	float	ssa;	};
	struct	mapMatrixTextures	: public	FixedMAP<STextureList*,mapMatrixItems,render_allocator>				{	float	ssa;	};
	struct	mapMatrixStates		: public	FixedMAP<ID3DState*,mapMatrixTextures,render_allocator>	{	float	ssa;	};
	struct	mapMatrixCS			: public	FixedMAP<R_constant_table*,mapMatrixStates,render_allocator>			{	float	ssa;	};
#ifdef USE_DX11
	struct	mapMatrixAdvStages
	{
		hs_type		hs;
		ds_type		ds;
		mapMatrixCS	mapCS;
	};
	struct	mapMatrixPS			: public	FixedMAP<ps_type, mapMatrixAdvStages,render_allocator>						{	float	ssa;	};
#else
	struct	mapMatrixPS			: public	FixedMAP<ps_type, mapMatrixCS,render_allocator>						{	float	ssa;	};
#endif	//	USE_DX11
#if defined(USE_DX10) || defined(USE_DX11)
	struct	mapMatrixGS			: public	FixedMAP<gs_type, mapMatrixPS,render_allocator>						{	float	ssa;	};
	struct	mapMatrixVS			: public	FixedMAP<vs_type, mapMatrixGS,render_allocator>						{	};
#else	//	USE_DX10
	struct	mapMatrixVS			: public	FixedMAP<vs_type, mapMatrixPS,render_allocator>						{	};
#endif	//	USE_DX10
	typedef mapMatrixVS			mapMatrix_T;
	typedef mapMatrix_T			mapMatrixPasses_T[SHADER_PASSES_MAX];

	// Top level
	typedef FixedMAP<float,_MatrixItemS,render_allocator>			mapSorted_T;
	typedef mapSorted_T::TNode						mapSorted_Node;

	typedef FixedMAP<float,_MatrixItemS,render_allocator>			mapHUD_T;
	typedef mapHUD_T::TNode							mapHUD_Node;

	typedef FixedMAP<float,_LodItem,render_allocator>				mapLOD_T;
	typedef mapLOD_T::TNode							mapLOD_Node;
};
