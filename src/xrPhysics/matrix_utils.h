#pragma once
IC float clamp_rotation( Fquaternion &q, float v )
{
	float angl;Fvector ax;
	q.get_axis_angle( ax, angl );
	float abs_angl = _abs( angl );
	if( abs_angl > v )
	{
		if( angl <  0.f ) v = -v;
		q.rotation( ax, v );
		q.normalize( );
	}
	return abs_angl;
}

IC float  clamp_rotation( Fmatrix &m, float v )
{
	Fquaternion q;
	q.set(m);
	float r = clamp_rotation( q, v );
	Fvector c = m.c;
	m.rotation( q );
	m.c = c;
	return r;
}

IC void get_axis_angle( const Fmatrix &m, Fvector &ax, float &angl )
{
	Fquaternion q;
	q.set( m );
	q.get_axis_angle( ax, angl );
}

IC bool clamp_change( Fmatrix& m, const Fmatrix &start, float ml, float ma, float tl, float ta )
{
	Fmatrix diff; diff.mul_43( Fmatrix( ).invert( start ), m );
	float linear_ch	 = diff.c.magnitude( );
	bool ret = linear_ch < tl;

	if( linear_ch > ml )
		diff.c.mul( ml/linear_ch );

	if( clamp_rotation( diff, ma ) > ta )
		ret = false;

	if(!ret)
		m.mul_43( start, diff );
	return ret;
}

IC void get_diff_value( const Fmatrix & m0, const Fmatrix &m1, float &l, float &a )
{
	Fmatrix diff; diff.mul_43( Fmatrix( ).invert( m1 ), m0 );
	l = diff.c.magnitude( );
	Fvector ax; 
	get_axis_angle( diff, ax, a );
	a = _abs( a );
}

IC void	cmp_matrix( bool &eq_linear, bool &eq_angular, const Fmatrix &m0, const Fmatrix &m1, float tl, float ta )
{
	float l,a;
	get_diff_value( m0, m1, l, a );
	eq_linear = l < tl; eq_angular = a < ta;
}

IC bool cmp_matrix( const Fmatrix &m0, const Fmatrix &m1, float tl, float ta )
{
	bool l =false, a =false;
	cmp_matrix( l, a, m0, m1, tl, ta );
	return l && a;
}

IC void angular_diff( Fvector &aw, const Fmatrix &diff, float dt )
{
	aw.set( ( diff._32-diff._23 )/2.f/dt,
			( diff._13-diff._31 )/2.f/dt,
			( diff._21-diff._12 )/2.f/dt
		);
}

IC void linear_diff( Fvector &lv, const Fvector &diff, float dt )
{
	lv.mul( diff, (1.f/dt) );
}

IC void linear_diff( Fvector &lv, const Fvector &mc1, const Fvector &mc0, float dt )
{
	linear_diff( lv, Fvector().sub( mc1, mc0 ), dt );
}

IC void matrix_diff( Fvector &lv, Fvector &aw, const Fmatrix &diff, float dt )
{
	angular_diff( aw, diff, dt );
	linear_diff( lv, diff.c, dt );
}

IC void matrix_diff( Fvector &lv, Fvector &aw, const Fmatrix &m0, const Fmatrix &m1, float dt )
{
	matrix_diff( lv, aw, Fmatrix().mul_43( Fmatrix().invert( m0 ), m1 ), dt );
}