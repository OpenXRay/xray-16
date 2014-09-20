#pragma once
class INetReader;
struct XRLC_LIGHT_API _TCF {
	Fvector2			uv	[3];

	void	barycentric	(Fvector2 &P, float &u, float &v, float &w);
	IC void	barycentric	(Fvector2 &P, Fvector &B)		{	barycentric(P,B.x,B.y,B.z); }
	IC bool	isInside	(float u, float v, float w)		{	return (u>=0 && u<=1) && (v>=0 && v<=1) && (w>=0 && w<=1); }
	IC bool	isInside	(Fvector &B)					{	return	isInside	(B.x,B.y,B.z); }
	IC bool	isInside	(Fvector2 &P, Fvector &B)		{	barycentric(P,B);	return isInside(B);	}
	void	read		( INetReader	&r );
	void	write		( IWriter	&w ) const ;
	bool	similar		(  const _TCF &_tc, float eps = EPS ) const;
};