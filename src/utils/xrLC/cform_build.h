#pragma once

struct	cform_FailFace
{
	Fvector	P[3];
	u32		props;
};
union	cform_mergeprops	{
	u32			props;			// 4b
	struct {
		u16		material;		// 2b
		u16		sector;			// 2b
	};
};

#pragma warning(disable:4267)
#pragma warning(disable:4995)
#pragma warning(disable:4244)

#define FLT_MIN	flt_min
#define FLT_MAX	flt_max

// OpenMesh
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/Types/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Tools/Decimater/DecimaterT.hh>
#include <OpenMesh/Tools/Decimater/ModNormalFlippingT.hh>
#include <OpenMesh/Tools/Decimater/ModQuadricT.hh>
#include <OpenMesh/Tools/Decimater/ModProgMeshT.hh>
#include <OpenMesh/Tools/Decimater/ModIndependentSetsT.hh>
#include <OpenMesh/Tools/Decimater/ModRoundnessT.hh>

using namespace		CDB;
using namespace		OpenMesh;

//t-defs
struct		MyTraits : public OpenMesh::DefaultTraits	
{
	FaceTraits	{
private:
	u32		props_;
public:
	FaceT() : props_(0)					{ }

	const	u32 props() const			{ return props_;	}
	void	set_props(const u32 _p)		{ props_ = _p;		}
	};

	// HalfedgeAttributes( OpenMesh::Attributes::None );
};
typedef		TriMesh_ArrayKernelT	< MyTraits >			_mesh;			// Mesh type
typedef		Decimater::DecimaterT	< _mesh >				_decimater;		// Decimater type
typedef		Decimater::ModQuadricT	< _decimater >::Handle	_HModQuadric;	// Decimation Module Handle type
