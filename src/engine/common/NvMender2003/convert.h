#ifndef	_CONVERT_H_
#define	_CONVERT_H_
IC D3DXVECTOR3& cv_vector ( D3DXVECTOR3	&l, const Fvector& r  )
{
	l.x = r.x;
	l.y = r.y;
	l.z = r.z;
	return l;
}

IC Fvector&  cv_vector (  Fvector& l, const D3DXVECTOR3	&r  )
{
	l.x = r.x;
	l.y = r.y;
	l.z = r.z;
	return l;
}




#endif