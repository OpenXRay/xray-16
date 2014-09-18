#pragma once

#include "base_basis.h"
#include "base_color.h"
struct Shader_xrLC;
class INetReader;
class XRLC_LIGHT_API base_Face
{
public: 
	base_basis				basis_tangent		[3];
	base_basis				basis_binormal		[3];

	u16						dwMaterial;			// index of material
	u16						dwMaterialGame;		// unique-id of game material (must persist up to game-CForm saving)

	struct					{
		u16					bSplitted			:		1;
		u16					bProcessed			:		1;
		u16					bOpaque				:		1;	// For ray-tracing speedup
		u16					bLocked				:		1;	// For tesselation
	}						flags;

	virtual	const Shader_xrLC&	Shader			( )const;
	virtual void			CacheOpacity		( );
	virtual Fvector2*		getTC0				( ) = 0;

	base_Face();
	virtual ~base_Face() = 0; 

	virtual	void			read	(INetReader	&r );
	virtual	void			write	(IWriter	&w )const;
};		


class XRLC_LIGHT_API base_Vertex
{
public: 
	Fvector					P;
	Fvector					N;
	base_color				C;			// all_lighting info
	int						handle;		// used in mesh-processing/optimization/conversion
public:

	base_Vertex()			{ }
	virtual ~base_Vertex()	= 0;

	virtual	void			read	(INetReader	&r );
	virtual	void			write	(IWriter	&w )const;
};
