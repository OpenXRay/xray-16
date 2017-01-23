#pragma once

class MagicBox3 {
public:
					MagicBox3		();
					MagicBox3		(const Fmatrix &m, const Fvector &half_size);

	Fvector&		Center			();
	const Fvector&	Center			() const;

	Fvector&		Axis			(int i);
	const Fvector&	Axis			(int i) const;
	Fvector*		Axes			();
	const Fvector*	Axes			() const;

	float&			Extent			(int i);
	const float&	Extent			(int i) const;
	float*			Extents			();
	const float*	Extents			() const;

	void			ComputeVertices	(Fvector *akVertex) const;

	bool			intersects		(const MagicBox3 &box) const;

protected:
	Fvector m_kCenter;
	Fvector m_akAxis[3];
	float	m_afExtent[3];
};

#include "magic_box3_inline.h"