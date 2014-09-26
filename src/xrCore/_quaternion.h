#ifndef __Q__
#define __Q__

/***************************************************************************
	The quatern module contains basic support for a quaternion object.

	quaternions are an extension of complex numbers that allows an
	expression for rotation that can be easily interpolated.  Quaternions are also
	more numericaly stable for repeated rotations than matrices.


	A quaternion is a 4 element 'vector3'  [w,x,y,z] where:

	q = w + xi + yj + zk
	i*i = -1
	j*j = -1
	k*k = -1
	i*j = -j*i = k
	j*k = -k*j = i
	k*i = -i*k = j
	q' (conjugate) = w - xi - yj - zk
	||q|| (magnitude) = sqrt(q*q') = sqrt(w*w + x*x + y*y + z*z)
	unit quaternion ||q|| == 1; this implies  q' == qinverse
	quaternions are associative (q1*q2)*q3 == q1*(q2*q3)
	quaternions are not commutative  q1*q2 != q2*q1
	qinverse (inverse (1/q) ) = q'/(q*q')

	q can be expressed by w + xi + yj + zk or [w,x,y,z]
	or as in this implementation (s,v) where s=w, and v=[x,y,z]

	quaternions can represent a rotation.  The rotation is an angle t, around a
	unit vector3 u.   q=(s,v);  s= cos(t/2);   v= u*sin(t/2).

	quaternions can apply the rotation to a point.  let the point be p [px,py,pz],
	and let P be a quaternion(0,p).  Protated = q*P*qinverse
	( Protated = q*P*q' if q is a unit quaternion)

	concatenation rotations is similar to matrix concatenation.  given two rotations
	q1 and q2,  to rotate by q1, then q2:  let qc = (q2*q1), then the combined
	rotation is given by qc*P*qcinverse (= qc*P*qc' if q is a unit quaternion)

	multiplication:
	q1 = w1 + x1i + y1j + z1k
	q2 = w2 + x2i + y2j + z2k
	q1*q2 = q3 =
			(w1*w2 - x1*x2 - y1*y2 - z1*z2)     {w3}
	        (w1*x2 + x1*w2 + y1*z2 - z1*y2)i	{x3}
			(w1*y2 - x1*z2 + y1*w2 + z1*x2)j    {y3}
			(w1*z2 + x1*y2 + y1*x2 + z1*w2)k	{z3}

	also,
	q1 = (s1,v1) = [s1,(x1,y1,z1)]
	q2 = (s2,v2) = [s2,(x2,y2,z2)]
	q1*q2 = q3	=	(s1*s2 - dot_product(v1,v2),			{s3}
					(s1*v2 + s2*v1 + cross_product(v1,v2))	{v3}

	interpolation - it is possible (and sometimes reasonable) to interpolate between
	two quaternions by interpolating each component.  This does not quarantee a
	resulting unit quaternion, and will result in an animation with non-linear
	rotational velocity.

	spherical interpolation: (slerp) treat the quaternions as vectors
	find the angle between them (w = arccos(q1 dot q2) ).
	given 0<=t<=1,  q(t) = q1*(sin((1-t)*w)/sin(w) + q2 * sin(t*w)/sin(w).
	since q == -q, care must be taken to rotate the proper way.

	this implementation uses the notation quaternion q = (quatS,quatV)
	  where quatS is a scalar, and quatV is a 3 element vector3.

	***************************************************************************

	Quaternions are really strange mathematical objects, just like complex
	numbers except that instead of just a real and imaginary part, you have
	three imaginary components, so every quaternion is of the form a + bi +
	cj + dk, where i, j, and k when squared equal -1.  The odd thing about
	these numbers is that they don't obey the commutative law of
	multiplication pq != qp if p and q are quaternions.  They're multiplied
	by the distributive law, and by the rules: i^2 = j^2 = k^2 = -1, i*j = k
	= -j*i, j*k = i = -k*j, and k*i = j = -i*k.  For rotations in graphics,
	you're going to be interested in the unit quaternions, quaternions for
	which sqrt(a^2 + b^2 + c^2 + d^2) = 1, as in this form:

	cos(phi/2) + b*sin(phi/2)*i + c*sin(phi/2)*j + d*sin(phi/2)*k

	This corresponds to a rotation of an angle phi about the axis [ b c d ]
	(which is a unit vector3, of course).  A unit quaternion can also be
	thought of as a point on the surface of a four-dimensional hypersphere,
	so if you try to interpolate between two unit quaternions, you can get
	an intermediate rotation.  Gamasutra describes Shoemake's spherical
	linear interpolation method, but I think getting the logarithm of the
	quaternions and performing linear interpolation is easier.  The
	logarithm of a quaternion is given by

	ln(a + bi + cj + dk) = log(sqrt(a^2 + b^2 + c^2 + d^2))
	+ i*b*arctan(r/a)/r
	+ j*c*arctan(r/a)/r
	+ k*d*arctan(r/a)/r

	where r = sqrt(b^2 + c^2 + d^2)

	Note that the first component will always be zero for unit quaternions. 
	Linearly interpolate each of the components of the logarithm of both
	quaternions then perform the quaternion exponential on the result, given
	by

	exp(a + bi + cj + dk) = exp(a)*cos(r) +
	i*exp(a)*b*sin(r)/r +
	j*exp(a)*c*sin(r)/r +
	k*exp(a)*d*sin(r)/r

	where r is the same factor as above.  This finds an intermediate
	rotation.  Now, to actually use the quaternion rotation q on a point
	p=[x y z], you compute the quaternion product s' = qsq^-1 where q is the
	unit quaternion a + bi + cj + dk for the rotation you want, q^-1 is a -
	bi - cj - dk, and s = xi +  yj + zk.  s' will be of the form x'i + y'j +
	z'k, so the rotated point is p'=[x' y' z'].  The quaternion triple
	product above is equivalent to a rotation matrix, as a lot of tedious
	algebra can show.  The proof is left as an exercise for the reader :-).

	For more information, you can check the Gamasutra article referred to
	above, and Section 21.1 and Exercise 21.7 in Foley, et. al.'s "Computer
	Graphics: Principles and Practice".  I had to implement a lot of this
	for my software renderer library...

***************************************************************************/

#define UNIT_TOLERANCE			0.001f
	// Quaternion magnitude must be closer than this tolerance to 1.0 to be
	// considered a unit quaternion

#define QZERO_TOLERANCE			0.00001f
	// quaternion magnitude must be farther from this tolerance to 0.0 to be
	// normalized

#define TRACE_QZERO_TOLERANCE	0.1f
	// trace of matrix must be greater than this to be used for converting a matrix
	// to a quaternion.

#define AA_QZERO_TOLERANCE		0.0001f
#define QEPSILON				0.00001f

template <class T>
struct XRCORE_API _quaternion {
public:
	typedef T				TYPE;
	typedef _quaternion<T>	Self;
	typedef Self&			SelfRef;
	typedef const Self&		SelfCRef;
private:
	IC T _asin_(T x)
	{
		const T c1 = 0.892399f;
		const T c3 = 1.693204f;
		const T c5 =-3.853735f;
		const T c7 = 2.838933f;
		
		const T x2 = x * x;
		const T d = x * (c1 + x2 * (c3 + x2 * (c5 + x2 * c7)));
		
		return d;
	}
	IC T _acos_(T x)
	{
		return PI_DIV_2 - _asin_(x);
	}
public:
	T x,y,z,w;

	IC	SelfRef	set(T W, T X, T Y, T Z)	// don't normalize
	{	x=X; y=Y; z=Z; w=W;			return *this; }
	IC	SelfRef	set(SelfCRef Q)				// don't normalize
	{	set(Q.w, Q.x, Q.y, Q.z);	return *this; }

	IC SelfRef	set(const _matrix<T>& m);

	// multiplies q1 * q2, and places the result in *this.
	// no failure. 	renormalization not automatic

/*
	q1*q2 = q3 =
		(w1*w2 - x1*x2 - y1*y2 - z1*z2)     {w3}
		(w1*x2 + x1*w2 + y1*z2 - z1*y2)i	{x3}
		(w1*y2 - x1*z2 + y1*w2 + z1*x2)j    {y3}
		(w1*z2 + x1*y2 - y1*x2 + z1*w2)k	{z3}
*/
	IC	SelfRef	mul(SelfCRef q1l, SelfCRef q2l)
	{
		VERIFY( q1l.isValid() );
		VERIFY( q2l.isValid() );

		w  =	(  (q1l.w*q2l.w) - (q1l.x*q2l.x)
				- (q1l.y*q2l.y) - (q1l.z*q2l.z) );

		x  =	(  (q1l.w*q2l.x) + (q1l.x*q2l.w)
			+ (q1l.y*q2l.z) - (q1l.z*q2l.y) );

		y  =	(  (q1l.w*q2l.y) - (q1l.x*q2l.z)
			+ (q1l.y*q2l.w) + (q1l.z*q2l.x) );

		z  = (  (q1l.w*q2l.z) + (q1l.x*q2l.y)
			- (q1l.y*q2l.x) + (q1l.z*q2l.w) );
		return *this; 
	}

	IC	SelfRef	add(SelfCRef q1, SelfCRef q2)
	{
		x  =	q1.x+q2.x;
		y  =	q1.y+q2.y;
		z  =	q1.z+q2.z;
		w  =	q1.w+q2.w;
		return *this; 
	}
	IC	SelfRef	sub(SelfCRef q1, SelfCRef q2)
	{
		x  =	q1.x-q2.x;
		y  =	q1.y-q2.y;
		z  =	q1.z-q2.z;
		w  =	q1.w-q2.w;
		return *this; 
	}

	IC	SelfRef	add(SelfCRef q)
	{
		x  +=	q.x;
		y  +=	q.y;
		z  +=	q.z;
		w  +=	q.w;
		return *this; 
	}
	IC	SelfRef	sub(SelfCRef q)
	{
		x  -=	q.x;
		y  -=	q.y;
		z  -=	q.z;
		w  -=	q.w;
		return *this; 
	}

	// validates numerical stability
	IC	const BOOL	isValid(void) const 
	{
		if ((w * w) < 0.0f)	return false;
		if ((x * x) < 0.0f)	return false;
		if ((y * y) < 0.0f)	return false;
		if ((z * z) < 0.0f)	return false;
		return true;
	}

	// checks for Unit-length quanternion
	IC	const BOOL	isUnit(void) 
	{
		T m  =  magnitude();

		if (( m < 1.0+UNIT_TOLERANCE ) && ( m > 1.0-UNIT_TOLERANCE ))
			return true;
		return false;
	}

	// normalizes Q to be a unit geQuaternion
	IC	SelfRef	normalize(void) 
	{
		T	m,one_over_magnitude;

		m =  _sqrt(magnitude());

		if (( m < QZERO_TOLERANCE ) && ( m > -QZERO_TOLERANCE ))
			return *this;

		one_over_magnitude = 1.0f / m;

		w *= one_over_magnitude;
		x *= one_over_magnitude;
		y *= one_over_magnitude;
		z *= one_over_magnitude;
		return *this; 
	}

	// inversion
	IC	SelfRef	inverse(SelfCRef Q)
	{	return set(Q.w,-Q.x,-Q.y,-Q.z);	}
	IC	SelfRef	inverse()
	{	return set(w,-x,-y,-z);	}
	IC	SelfRef	inverse_with_w(SelfCRef Q)
	{	return set(-Q.w,-Q.x,-Q.y,-Q.z);	}
	IC	SelfRef	inverse_with_w()
	{	return set(-w,-x,-y,-z);	}

	// identity - no rotation
	IC	SelfRef	identity(void)
	{	return set(1.f,0.f,0.f,0.f);		}

	// square length
	IC	T	magnitude(void) {
		return w*w + x*x + y*y + z*z;
	}

	// makes unit rotation
	IC	SelfRef	rotationYawPitchRoll(T _x, T _y, T _z) 
	{
		T fSinYaw   = _sin(_x*.5f);
		T fCosYaw   = _cos(_x*.5f);
		T fSinPitch = _sin(_y*.5f);
		T fCosPitch = _cos(_y*.5f);
		T fSinRoll  = _sin(_z*.5f);
		T fCosRoll  = _cos(_z*.5f);

		x = fSinRoll * fCosPitch * fCosYaw - fCosRoll * fSinPitch * fSinYaw;
		y = fCosRoll * fSinPitch * fCosYaw + fSinRoll * fCosPitch * fSinYaw;
		z = fCosRoll * fCosPitch * fSinYaw - fSinRoll * fSinPitch * fCosYaw;
		w = fCosRoll * fCosPitch * fCosYaw + fSinRoll * fSinPitch * fSinYaw;
		return *this;
	}

	// makes unit rotation
	IC	SelfRef	rotationYawPitchRoll(const Fvector &ypr)
	{	return rotationYawPitchRoll(ypr.x,ypr.y,ypr.z);	}

	// set a quaternion from an axis and a rotation around the axis
	IC	SelfRef	rotation(Fvector &axis, T angle)
	{
		T	sinTheta;

		w		= _cos(angle*0.5f);
		sinTheta= _sin(angle*0.5f);
		x = sinTheta * axis.x;
		y = sinTheta * axis.y;
		z = sinTheta * axis.z;
		return *this;
	}

	// gets an axis and angle of rotation around the axis from a quaternion
	// returns TRUE if there is an axis.
	// returns FALSE if there is no axis (and Axis is set to 0,0,0, and Theta is 0)

	IC	BOOL	get_axis_angle(Fvector &axis, T &angle)
	{
		T s = _sqrt(x*x + y*y + z*z);
		if ( s > EPS_S ) 	{
			T OneOverSinTheta = 1.f/s;
			axis.x	= OneOverSinTheta * x;
			axis.y	= OneOverSinTheta * y;
			axis.z	= OneOverSinTheta * z;
			angle	= 2.0f * atan2(s,w);
			return	true;
		} else 	{
			axis.x	= axis.y = axis.z = 0.0f;
			angle	= 0.0f;
			return	false;
		}
	}

	// spherical interpolation between q0 and q1.   0<=t<=1
	// resulting quaternion is 'between' q0 and q1
	// with t==0 being all q0, and t==1 being all q1.
	// returns a quaternion with a positive W - always takes shortest route
	// through the positive W domain.
	ICF	SelfRef	slerp(SelfCRef Q0, SelfCRef Q1, T tm)
	{
		T Scale0,Scale1,sign;

#ifdef DEBUG		
		if (!( ( T(0) <= tm ) && ( tm <= T(1) ) ) )
			Debug.fatal(DEBUG_INFO,"Quaternion::slerp - invalid 'tm' arrived: %f",tm);
#endif
		
		T cosom =	(Q0.w * Q1.w) + (Q0.x * Q1.x) + (Q0.y * Q1.y) + (Q0.z * Q1.z);
		
		if (cosom < 0) 	{
			cosom	= -cosom;
			sign	= -1.f;
		} else {
			sign	= 1.f;
		}
		
		if ( (1.0f - cosom) > EPS ) {
			T	omega	= _acos_( cosom );
			T	i_sinom = 1.f / _sin( omega );
			T	t_omega	= tm*omega;
			Scale0 = _sin( omega - 	t_omega ) * i_sinom;
			Scale1 = _sin( t_omega			) * i_sinom;
		} else  {
			// has numerical difficulties around cosom == 0
			// in this case degenerate to linear interpolation
			Scale0 = 1.0f - tm;
			Scale1 = tm;
		}
		Scale1 *= sign;
		
		x = Scale0 * Q0.x + Scale1 * Q1.x;
		y = Scale0 * Q0.y + Scale1 * Q1.y;
		z = Scale0 * Q0.z + Scale1 * Q1.z;
		w = Scale0 * Q0.w + Scale1 * Q1.w;
		return *this;
	}

	// return TRUE if quaternions differ elementwise by less than Tolerance.
	IC	BOOL	cmp(SelfCRef Q, T Tolerance=0.0001f)
	{
		if (	// they are the same but with opposite signs
			(	(_abs(x + Q.x) <= Tolerance )
			&&  (_abs(y + Q.y) <= Tolerance )
			&&  (_abs(z + Q.z) <= Tolerance )
			&&  (_abs(w + Q.w) <= Tolerance )
			)
			||  // they are the same with same signs
			(	(_abs(x - Q.x) <= Tolerance )
			&&  (_abs(y - Q.y) <= Tolerance )
			&&  (_abs(z - Q.z) <= Tolerance )
			&&  (_abs(w - Q.w) <= Tolerance )
			)
			)
			return true;
		else
			return false;
	}
	IC	SelfRef	ln(SelfCRef Q)
	{
		T n	 = Q.x*Q.x+Q.y*Q.y+Q.z*Q.z;
		T r  = _sqrt(n);
		T t  = (r>EPS_S)?atan2f(r,Q.w)/r: 0.f;
		x = t*Q.x;
		y = t*Q.y;
		z = t*Q.z;
		w = .5f*_log(n+Q.w*Q.w);
		return *this;
	}
	IC	SelfRef	exp(SelfCRef Q)
	{
		T r  = _sqrt(Q.x*Q.x+Q.y*Q.y+Q.z*Q.z);
		T et = expf(Q.w);
		T s  = (r>=EPS_S)? et*_sin(r)/r: 0.f;
		x = s*Q.x;
		y = s*Q.y;
		z = s*Q.z;
		w = et*_cos(r);
		return *this;
	}
};

typedef _quaternion<float>	Fquaternion;
typedef _quaternion<double>	Dquaternion;

template <class T>
BOOL	_valid			(const _quaternion<T>& s)	{ return _valid(s.x) && _valid(s.y) && _valid(s.z) && _valid(s.w);	}

#undef UNIT_TOLERANCE
#undef QZERO_TOLERANCE
#undef TRACE_QZERO_TOLERANCE
#undef AA_QZERO_TOLERANCE
#undef QEPSILON

#endif
