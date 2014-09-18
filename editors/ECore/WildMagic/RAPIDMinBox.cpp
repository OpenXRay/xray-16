#include "stdafx.h"
#pragma hdrstop

struct moment
{
    float     A;
    Fvector   m;
    Fmatrix33 s;
};

struct accum
{
    float     A;
    Fvector   m;
    Fmatrix33 s;
};

void compute_moment(moment &M, Fvector& p, Fvector& q, Fvector& r)
{
    Fvector u, v, w;

    // compute the area of the triangle
    u.sub(q,p);
    v.sub(r,p);
    w.crossproduct(u,v);
    M.A = 0.5f * w.magnitude();

    if (M.A == 0.0f){
        // This triangle has zero area.  The second order components
        // would be eliminated with the usual formula, so, for the
        // sake of robustness we use an alternative form.  These are the
        // centroid and second-order components of the triangle's vertices.

        // centroid
        M.m.x = (p.x + q.x + r.x) /3;
        M.m.y = (p.y + q.y + r.y) /3;
        M.m.z = (p.z + q.z + r.z) /3;

        // second-order components
        M.s.m[0][0] = (p.x*p.x + q.x*q.x + r.x*r.x);
        M.s.m[0][1] = (p.x*p.y + q.x*q.y + r.x*r.y);
        M.s.m[0][2] = (p.x*p.z + q.x*q.z + r.x*r.z);
        M.s.m[1][1] = (p.y*p.y + q.y*q.y + r.y*r.y);
        M.s.m[1][2] = (p.y*p.z + q.y*q.z + r.y*r.z);
        M.s.m[2][2] = (p.z*p.z + q.z*q.z + r.z*r.z);
        M.s.m[2][1] = M.s.m[1][2];
        M.s.m[1][0] = M.s.m[0][1];
        M.s.m[2][0] = M.s.m[0][2];

        return;
    }

    // get the centroid
    M.m.x = (p.x + q.x + r.x)/3;
    M.m.y = (p.y + q.y + r.y)/3;
    M.m.z = (p.z + q.z + r.z)/3;

    // get the second order components -- note the weighting by the area
    M.s.m[0][0] = M.A*(9*M.m[0]*M.m[0]+p[0]*p[0]+q[0]*q[0]+r[0]*r[0])/12;
    M.s.m[0][1] = M.A*(9*M.m[0]*M.m[1]+p[0]*p[1]+q[0]*q[1]+r[0]*r[1])/12;
    M.s.m[1][1] = M.A*(9*M.m[1]*M.m[1]+p[1]*p[1]+q[1]*q[1]+r[1]*r[1])/12;
    M.s.m[0][2] = M.A*(9*M.m[0]*M.m[2]+p[0]*p[2]+q[0]*q[2]+r[0]*r[2])/12;
    M.s.m[1][2] = M.A*(9*M.m[1]*M.m[2]+p[1]*p[2]+q[1]*q[2]+r[1]*r[2])/12;
    M.s.m[2][2] = M.A*(9*M.m[2]*M.m[2]+p[2]*p[2]+q[2]*q[2]+r[2]*r[2])/12;
    M.s.m[2][1] = M.s.m[1][2];
    M.s.m[1][0] = M.s.m[0][1];
    M.s.m[2][0] = M.s.m[0][2];
}

void compute_moments(moment *M, Fvector* vertices, int num_tris)
{
    int i;

  // first collect all the moments, and obtain the area of the
  // smallest nonzero area triangle.

    float Amin = 0.0f;
    int zero = 0;
    for(i=0; i<num_tris; i++){
        compute_moment(M[i], vertices[i*3+0], vertices[i*3+1], vertices[i*3+2]);
        if (M[i].A == 0.0f){
    	    zero = 1;
	    }else{
	        if (Amin == 0.0f) Amin = M[i].A;
    	    else if (M[i].A < Amin) Amin = M[i].A;
	    }
    }

    if (zero){
      // if there are any zero area triangles, go back and set their area

      // if ALL the triangles have zero area, then set the area thingy
      // to some arbitrary value.
        if (Amin == 0.0f) Amin = 1.0f;

        for(i=0; i<num_tris; i++){
    	    if (M[i].A == 0.0f) M[i].A = Amin;
	    }
    }
}

void clear_accum(accum &a)
{
    a.m.set(0.f,0.f,0.f);
    a.s.m[0][0] = a.s.m[0][1] = a.s.m[0][2] = 0.0;
    a.s.m[1][0] = a.s.m[1][1] = a.s.m[1][2] = 0.0;
    a.s.m[2][0] = a.s.m[2][1] = a.s.m[2][2] = 0.0;
    a.A = 0.0;
}

void accum_moment(accum &a, moment &b)
{
    a.m.x += b.m.x * b.A;
    a.m.y += b.m.y * b.A;
    a.m.z += b.m.z * b.A;

    a.s.m[0][0] += b.s.m[0][0];
    a.s.m[0][1] += b.s.m[0][1];
    a.s.m[0][2] += b.s.m[0][2];
    a.s.m[1][0] += b.s.m[1][0];
    a.s.m[1][1] += b.s.m[1][1];
    a.s.m[1][2] += b.s.m[1][2];
    a.s.m[2][0] += b.s.m[2][0];
    a.s.m[2][1] += b.s.m[2][1];
    a.s.m[2][2] += b.s.m[2][2];

    a.A += b.A;
}

void mean_from_moment(Fvector& M, moment &m)
{
    M.set(m.m);
}

void mean_from_accum(Fvector& M, accum &a)
{
    M.div(a.m, a.A);
}

void covariance_from_accum(Fmatrix33& C, accum &a)
{
  int i,j;
  for(i=0; i<3; i++)
    for(j=0; j<3; j++)
      C.m[i][j] = a.s.m[i][j] - a.m[i]*a.m[j]/a.A;
}

// not a full sort -- just makes column 1 the largest
int eigen_and_sort1(Fmatrix33& evecs, Fmatrix33& cov)
{
	float t;
	Fvector evals;
	int n;

	n = evecs.Meigen( evals, cov);

	if (evals.z > evals.x){
		if (evals.z > evals.y){
			// 2 is largest, swap with column 0
			t = evecs.m[0][2];
			evecs.m[0][2] = evecs.m[0][0];
			evecs.m[0][0] = t;
			t = evecs.m[1][2];
			evecs.m[1][2] = evecs.m[1][0];
			evecs.m[1][0] = t;
			t = evecs.m[2][2];
			evecs.m[2][2] = evecs.m[2][0];
			evecs.m[2][0] = t;
		}else{
			// 1 is largest, swap with column 0
			t = evecs.m[0][1];
			evecs.m[0][1] = evecs.m[0][0];
			evecs.m[0][0] = t;
			t = evecs.m[1][1];
			evecs.m[1][1] = evecs.m[1][0];
			evecs.m[1][0] = t;
			t = evecs.m[2][1];
			evecs.m[2][1] = evecs.m[2][0];
			evecs.m[2][0] = t;
		}
	}else{
		if (evals.x > evals.y){
			// 0 is largest, do nothing
		}else{
  			// 1 is largest
			t = evecs.m[0][1];
			evecs.m[0][1] = evecs.m[0][0];
			evecs.m[0][0] = t;
			t = evecs.m[1][1];
			evecs.m[1][1] = evecs.m[1][0];
			evecs.m[1][0] = t;
			t = evecs.m[2][1];
			evecs.m[2][1] = evecs.m[2][0];
			evecs.m[2][0] = t;
		}
	}

	// we are returning the number of iterations Meigen took.
	// too many iterations means our chosen orientation is bad.
	return n;
}

void minmax(float &mn, float &mx, float v)
{
	if (v < mn) mn = v;
	else if (v > mx) mx = v;
}

void reaccum_moments(accum &A, int n, moment* RAPID_moment)
{
	clear_accum(A);
	for(int i=0; i<n; i++)
		accum_moment(A, RAPID_moment[i]);
}

BOOL RAPIDMinBox(Fobb& B, Fvector* vertices, u32 v_count)
{
	u32 num_tris 		= v_count/3;

	// Determine initial orientation, mean point, and splitting axis.
	accum 				M;
	Fmatrix33			C;
    
	moment* RAPID_moment= xr_alloc<moment>(num_tris);
    VERIFY				(RAPID_moment);

	compute_moments		(RAPID_moment, vertices, num_tris);

	clear_accum			(M);
	for(u32 i=0; i<num_tris; i++)
		accum_moment	(M, RAPID_moment[i]);

	mean_from_accum		(B.m_translate, M);
	covariance_from_accum(C, M);

	eigen_and_sort1		(B.m_rotate, C);
    
	// do the build
	Fvector 			c;
	Fvector 			minval, maxval;
    B.m_rotate.MTxV		(c,vertices[0]);
    minval.set			(c);
    maxval.set			(c);
	for(i=0; i<v_count; i++){
	    B.m_rotate.MTxV	(c,vertices[i]);
		minmax			(minval.x, maxval.x, c.x);
		minmax			(minval.y, maxval.y, c.y);
		minmax			(minval.z, maxval.z, c.z);
	}

	// With the max and min data, determine the center point and dimensions
	// of the parent box.
    c.add				(minval,maxval);
    c.mul				(0.5f);

	B.m_translate.x 	= c[0] * B.m_rotate.m[0][0] + c[1] * B.m_rotate.m[0][1] + c[2] * B.m_rotate.m[0][2];
	B.m_translate.y 	= c[0] * B.m_rotate.m[1][0] + c[1] * B.m_rotate.m[1][1] + c[2] * B.m_rotate.m[1][2];
	B.m_translate.z 	= c[0] * B.m_rotate.m[2][0] + c[1] * B.m_rotate.m[2][1] + c[2] * B.m_rotate.m[2][2];
    B.m_halfsize.sub	(maxval,minval);
    B.m_halfsize.mul	(0.5f);

	// free the moment list
	xr_free				(RAPID_moment);

    // need for our use (RAPID has OpenGL column<->line)
    B.m_rotate.transpose();
    
	return TRUE;
}
//------------------------------------------------------------------------------

