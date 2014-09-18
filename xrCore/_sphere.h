#ifndef _F_SPHERE_H_
#define _F_SPHERE_H_

template <class T>
struct _sphere {
	_vector3<T>	P;
	T			R;
public:
	IC void		set(const _vector3<T> &_P, T _R)	{ P.set(_P); R = _R; }
	IC void		set(const _sphere<T> &S)			{ P.set(S.P); R=S.R; }
	IC void		identity()							{ P.set(0,0,0); R=1; }

	enum ERP_Result{
		rpNone			= 0,
		rpOriginInside	= 1,
		rpOriginOutside	= 2,
		fcv_forcedword = u32(-1)
	};
	// Ray-sphere intersection
	ICF ERP_Result intersect (const _vector3<T>& S, const _vector3<T>& D, T range, int& quantity, T afT[2]) const
	{
		// set up quadratic Q(t) = a*t^2 + 2*b*t + c
		_vector3<T> kDiff;  kDiff.sub	(S,P);
		T fA				= range*range;
		T fB				= kDiff.dotproduct(D)		* range;
		T fC				= kDiff.square_magnitude()	- R*R;
		ERP_Result result	= rpNone;

		T fDiscr			= fB*fB - fA*fC;
		if ( fDiscr < (T)0.0 ){
			quantity		= 0;
		} else if ( fDiscr > (T)0.0 ){
			T fRoot 		= _sqrt(fDiscr);
			T fInvA 		= ((T)1.0)/fA;
			afT[0]			= range*(-fB - fRoot)*fInvA;
			afT[1]			= range*(-fB + fRoot)*fInvA;
			if ( afT[0] >= (T)0.0 )		{	quantity	= 2;					result = rpOriginOutside;	}
			else if ( afT[1] >= (T)0.0 ){	quantity	= 1; afT[0] = afT[1];	result = rpOriginInside;	}
			else							quantity	= 0;
		} else {
			afT[0]			= range*(-fB/fA);
			if ( afT[0] >= (T)0.0 )		{	quantity	= 1;					result = rpOriginOutside;	}
			else							quantity	= 0;
		}
		return result;
	}
/*
			int				quantity;
			float			afT[2];
			Fsphere::ERP_Result	result	= sS.intersect(ray.pos,ray.fwd_dir,range,quantity,afT);

			if (Fsphere::rpOriginInside || ((result==Fsphere::rpOriginOutside)&&(afT[0]<range))){
				if (b_nearest)				{ 
					switch(result){
					case Fsphere::rpOriginInside:	range	= afT[0]<range?afT[0]:range;	break;
					case Fsphere::rpOriginOutside:	range	= afT[0];						break;
					}
					range2			=range*range; 
				}
*/
	ICF ERP_Result intersect_full	(const _vector3<T>& start, const _vector3<T>& dir, T& dist) const
	{
		int				quantity;
		float			afT[2];
		Fsphere::ERP_Result	result	= intersect(start,dir,dist,quantity,afT);

		if (result == Fsphere::rpOriginInside || ((result==Fsphere::rpOriginOutside)&&(afT[0]<dist))){
			switch(result){
				case Fsphere::rpOriginInside:	dist	= afT[0]<dist?afT[0]:dist;		break;
				case Fsphere::rpOriginOutside:	dist	= afT[0];						break;
			}
		}
		return			result;
	}

	ICF ERP_Result intersect	(const _vector3<T>& start, const _vector3<T>& dir, T& dist) const
	{
		int				quantity;
		T				afT[2];
		ERP_Result		result	= intersect(start,dir,dist,quantity,afT);
		if (rpNone!=result){
			VERIFY		(quantity>0);
			if (afT[0]<dist){
				dist	= afT[0];
				return	result;
			}
		}
		return			rpNone;
	}

	IC ERP_Result intersect2(const _vector3<T>& S, const _vector3<T>& D, T& range) const	
    {
		_vector3<T> Q;	Q.sub(P,S);
	
		T R2	= R*R;
		T c2	= Q.square_magnitude	();
		T v		= Q.dotproduct			(D);
		T d		= R2 - (c2 - v*v);

		if		(d > 0.f)
		{
			T _range	= v - _sqrt(d);
			if (_range<range)	{
				range = _range;
				return (c2<R2)?rpOriginInside:rpOriginOutside;
			}
		}
		return rpNone;
	}
	ICF BOOL		intersect(const _vector3<T>& S, const _vector3<T>& D) const	
	{
		_vector3<T> Q;	Q.sub(P,S);
	
		T c = Q.magnitude	();
		T v = Q.dotproduct	(D);
		T d = R*R - (c*c - v*v);
		return (d > 0);
	}
	ICF BOOL		intersect(const _sphere<T>& S) const
	{	
		T SumR = R+S.R;
		return P.distance_to_sqr(S.P) < SumR*SumR;
	}
	IC BOOL		contains(const _vector3<T>& PT) const 
	{
		return P.distance_to_sqr(PT) <= (R*R+EPS_S);
	}
	
	// returns true if this wholly contains the argument sphere
	IC BOOL		contains(const _sphere<T>& S) const	
	{
		// can't contain a sphere that's bigger than me !
		const T RDiff		= R - S.R;
		if ( RDiff < 0 )	return false;

		return ( P.distance_to_sqr(S.P) <= RDiff*RDiff );
	}

	// return's volume of sphere
	IC T		volume	() const
	{
		return T( PI_MUL_4 / 3 ) * (R*R*R);
	}
};

typedef _sphere<float>	Fsphere;
typedef _sphere<double> Dsphere;

template <class T>
BOOL	_valid			(const _sphere<T>& s)		{ return _valid(s.P) && _valid(s.R);	}

void	XRCORE_API		Fsphere_compute		(Fsphere& dest, const Fvector *verts, int count);

#endif