#include "stdafx.h"
#pragma hdrstop

class Miniball;
class Basis;

// Basis
// -----
class Basis 
{
private:
	enum { d = 3 }		eDimensions;

	// data members
	int					m, s;			// size and number of support vectors

	Fvector				q0;

	float				z[d+1];
	float				f[d+1];
	Fvector				v[d+1];
	Fvector				a[d+1];
	Fvector				c[d+1];
	float				sqr_r[d+1];

	Fvector*			current_c;      // vectors to some c[j]
	float				current_sqr_r;
public:
	Basis();

	// access
	const Fvector*		center			() const;
	float				squared_radius	() const;
	int                 size			() const;
	int                 support_size	() const;
	float				excess			(const Fvector& p) const;

	// modification
	void                reset			(); // generates empty sphere with m=s=0
	bool                push			(const Fvector& p);
	void                pop				();
};

// Miniball
// --------
class Miniball
{
public:
	// types
	typedef xr_list<Fvector>			VectorList;
	typedef VectorList::iterator        It;
	typedef VectorList::const_iterator  Cit;

private:
	// data members
	VectorList	L;						// STL list keeping the gVectors
	Basis		B;						// basis keeping the current ball
	It          support_end;			// past-the-end iterator of support set

	// private methods
	void		mtf_mb					(It k);
	void		pivot_mb				(It k);
	void		move_to_front			(It j);
	float		max_excess				(It t, It i, It& pivot) const;
	float		abs						(float r) const {return (r>0)? r: (-r);}
	float		sqr						(float r) const {return r*r;}
public:
	// construction
	Miniball() {}
	void        check_in				(const Fvector& p);
	void        build					();

	// access
	Fvector     center					() const;
	float		squared_radius			() const;
	int         num_points				() const;
	Cit         points_begin			() const;
	Cit         points_end				() const;
	int         nr_support_gVectors		() const;
	Cit         support_points_begin	() const;
	Cit         support_points_end		() const;
};

// Miniball
// --------

void Miniball::check_in (const Fvector& p)
{
	L.push_back(p);
}

void Miniball::build ()
{
	B.reset();
	support_end = L.begin();

	// @@ pivotting or not ?
	if ( 1 )
		pivot_mb	(L.end());
	else
		mtf_mb		(L.end());
}

void Miniball::mtf_mb (It i)
{
	support_end = L.begin();

	if ((B.size())== 4) 
		return;

	for (It k=L.begin(); k!=i;) 
	{
		It j=k++;
		if (B.excess(*j) > 0) 
		{
			if (B.push(*j)) 
			{
				mtf_mb (j);
				B.pop();
				move_to_front(j);
			}
		}
	}
}

void Miniball::move_to_front (It j)
{
	if (support_end == j)
		support_end++;
	L.splice (L.begin(), L, j);
}


void Miniball::pivot_mb (It i)
{
	It t = ++L.begin();
	mtf_mb (t);
	float max_e, old_sqr_r = 0;
	do 
	{
		It pivot = L.begin();
		max_e = max_excess (t, i, pivot);
		if (max_e > 0) 
		{
			t = support_end;
			if (t==pivot) ++t;
			old_sqr_r = B.squared_radius();
			B.push (*pivot);
			mtf_mb (support_end);
			B.pop();
			move_to_front (pivot);
		}
	} while ((max_e > 0) && (B.squared_radius() > old_sqr_r));
}

float Miniball::max_excess (It t, It i, It& pivot) const
{
	const	Fvector* pCenter = B.center();
	float				sqr_r	= B.squared_radius();

	float e, max_e = 0;

	for (It k=t; k!=i; ++k)
	{
		const Fvector & point = (*k);
		e = -sqr_r;

		e += point.distance_to_sqr(*pCenter);

		if (e > max_e)
		{
			max_e = e;
			pivot = k;
		}
	}

	return max_e;
}

Fvector Miniball::center () const
{
	return *((Fvector *)B.center());
}

float Miniball::squared_radius () const
{
	return B.squared_radius();
}

int Miniball::num_points () const
{
	return L.size();
}

Miniball::Cit Miniball::points_begin () const
{
	return L.begin();
}

Miniball::Cit Miniball::points_end () const
{
	return L.end();
}

int Miniball::nr_support_gVectors () const
{
	return B.support_size();
}

Miniball::Cit Miniball::support_points_begin () const
{
	return L.begin();
}

Miniball::Cit Miniball::support_points_end () const
{
	return support_end;
}


//----------------------------------------------------------------------
// Basis
//---------------------------------------------------------------------
const Fvector* Basis::center () const
{
	return current_c;
}

float Basis::squared_radius() const
{
	return current_sqr_r;
}

int Basis::size() const
{
	return m;
}

int Basis::support_size() const
{
	return s;
}

float Basis::excess (const Fvector& p) const
{
	float e = -current_sqr_r;
	e += p.distance_to_sqr(*current_c);
	return e;
}

void Basis::reset ()
{
	m = s = 0;
	// we misuse c[0] for the center of the empty sphere
	c[0].set(0,0,0);
	current_c = c;
	current_sqr_r = -1;
}


Basis::Basis ()
{
	reset();
}


void Basis::pop ()
{
	--m;
}

bool Basis::push (const Fvector& p)
{
	if (m==0)
	{
		q0 = p;
		c[0] = q0;
		sqr_r[0] = 0;
	}
	else
	{
		int i;
		const float eps = 1e-16f;

		// set v_m to Q_m
		v[m].sub(p,q0);

		// compute the a_{m,i}, i< m
		for (i=1; i<m; ++i)
		{
			a[m][i] =  v[i].dotproduct(v[m]);
			a[m][i] *= (2.f/z[i]);
		}

		// update v_m to Q_m-\bar{Q}_m
		for (i=1; i<m; ++i)
		{
			v[m].mad(v[m],v[i],-a[m][i]);
		}

		// compute z_m
		z[m]=0;
		z[m] += v[m].square_magnitude();
		z[m]*=2;

		// reject push if z_m too small
		if (z[m]<eps*current_sqr_r)
		{
			return false;
		}

		// update c, sqr_r
		float e = -sqr_r[m-1];
		e += p.distance_to_sqr(c[m-1]);

		f[m]=e/z[m];

		c[m].mad(c[m-1],v[m],f[m]);

		sqr_r[m] = sqr_r[m-1] + e*f[m]/2;
	}

	current_c = c+m;
	current_sqr_r = sqr_r[m];
	s = ++m;
	return true;
}

void Fsphere_compute(Fsphere& dest, const Fvector *verts, int count)
{
	Miniball mb;

	for(int i=0;i<count;i++)
		mb.check_in(verts[i]);

	mb.build	();

	dest.P.set	(mb.center());
	dest.R =	( _sqrt( mb.squared_radius() ));
}
