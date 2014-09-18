#ifndef _PLANE
#define _PLANE

template <class T>
class _plane {
public:
	typedef T			TYPE;
	typedef _plane<T>	Self;
	typedef Self&		SelfRef;
	typedef const Self&	SelfCRef;
public:
	_vector3<T>	n;
	T			d;
public:
	IC	SelfRef	set		(Self &P)
	{
		n.set	(P.n);
		d		= P.d;
		return *this;
	}
    IC	 BOOL 	similar (Self &P, T eps_n=EPS, T eps_d=EPS)
	{
    	return (n.similar(P.n,eps_n)&&(_abs(d-P.d)<eps_d));
    }
	ICF	SelfRef	build	(const _vector3<T> &v1, const _vector3<T> &v2, const _vector3<T> &v3) 
	{
		_vector3<T> t1,t2;
		n.crossproduct(t1.sub(v1,v2), t2.sub(v1,v3)).normalize();
		d = -n.dotproduct(v1);
		return *this;
	}
	ICF	SelfRef	build_precise	(const _vector3<T> &v1, const _vector3<T> &v2, const _vector3<T> &v3) 
	{
		_vector3<T> t1,t2;
		n.crossproduct(t1.sub(v1,v2), t2.sub(v1,v3)); exact_normalize(n);
		d = -n.dotproduct(v1);
		return *this;
	}
	ICF	SelfRef	build(const _vector3<T> &_p, const _vector3<T> &_n)
	{
		d			= - n.normalize(_n).dotproduct(_p);
		return *this;
	}
	ICF	SelfRef	build_unit_normal(const _vector3<T> &_p, const _vector3<T> &_n)
	{
		VERIFY		(fsimilar(_n.magnitude(),1,EPS));
		d			= - n.set(_n).dotproduct(_p);
		return *this;
	}
	IC	SelfCRef project(_vector3<T> &pdest, _vector3<T> const& psrc) const
	{
		pdest.mad	(psrc,n,-classify(psrc));
		return *this;
	}
	IC	SelfRef	project(_vector3<T> &pdest, _vector3<T> const& psrc)
	{
		pdest.mad	(psrc,n,-classify(psrc));
		return *this;
	}
	ICF	T		classify(const _vector3<T> &v) const	
	{
		return n.dotproduct(v)+d;
	}
	IC	SelfRef	normalize() 
	{
		T denom = 1.f / n.magnitude();
		n.mul(denom);
		d*=denom;
		return *this;
	}
	IC	T	distance	(const _vector3<T> &v)	
	{
		return _abs(classify(v));
	}
	IC BOOL intersectRayDist(const _vector3<T>& P, const _vector3<T>& D, T& dist)
	{
		T numer = classify(P);
		T denom = n.dotproduct(D);
	
		if (_abs(denom)<EPS_S)  // normal is orthogonal to vector3, cant intersect
			return FALSE;
	
		dist = -(numer / denom);
		return ((dist>0.f)||fis_zero(dist));
	}
	ICF BOOL intersectRayPoint(const _vector3<T>& P, const _vector3<T>& D, _vector3<T>& dest) 
	{
		T numer = classify(P);
		T denom = n.dotproduct(D);

		if (_abs(denom)<EPS_S) return FALSE; // normal is orthogonal to vector3, cant intersect
		else {
	        float dist	= -(numer / denom);
			dest.mad	(P,D,dist);
			return 		((dist>0.f)||fis_zero(dist));
		}
	}
	IC	BOOL	intersect (
		const _vector3<T>& u, const _vector3<T>& v,	// segment
	    _vector3<T>&	isect)                  // intersection point
	{
		T		denom,dist;
		_vector3<T>		t;

		t.sub(v,u);
		denom = n.dotproduct(t);
		if (_abs(denom) < EPS) return false; // they are parallel

		dist = -(n.dotproduct(u) + d) / denom;
		if (dist < -EPS || dist > 1+EPS) return false;
		isect.mad(u,t,dist);
		return true;
	}

	IC	BOOL	intersect_2 (
		const _vector3<T>& u, const _vector3<T>& v,				// segment
	    _vector3<T>& isect)						// intersection point
	{
		T		dist1, dist2;
		_vector3<T>		t;

		dist1		= n.dotproduct(u)+d;
		dist2		= n.dotproduct(v)+d;

		if (dist1*dist2<0.0f)
			return false;

		t.sub		(v,u);
		isect.mad	(u,t,dist1/_abs(dist1-dist2));

		return true;
	}
	IC	SelfRef	transform(_matrix<T>& M)
	{
		// rotate the normal
		M.transform_dir		(n);
		// slide the offset
		d -= M.c.dotproduct	(n);
		return *this;
	}
};

typedef _plane<float>	Fplane;
typedef _plane<double>	Dplane;

template <class T>
BOOL	_valid			(const _plane<T>& s)		{ return _valid(s.n) && _valid(s.d);	}

#endif
