#ifndef TLSpriteH
#define TLSpriteH

#define TO_REAL(_X_, _S_)	((_X_)+1.f)*float(_S_/2)

typedef struct _TLpolygon {
public:
	union{
		struct{
			FVF::TL 	lb, lt, rb, rt;
		};
		FVF::TL			m[4];
	};

	IC	void*	d3d		(void)									{ return this; }
	IC	void	setpos	(Frect &r)								{ lb.p.x=r.x1;	lb.p.y=r.y2;lt.p.x=r.x1;lt.p.y=r.y1;rb.p.x=r.x2;rb.p.y=r.y2;rt.p.x=r.x2;rt.p.y=r.y1;}
	IC	void	setpos	(float x1, float y1, float x2, float y2){ lb.p.x=x1;	lb.p.y=y2;	lt.p.x=x1;	lt.p.y=y1;	rb.p.x=x2;	rb.p.y=y2;	rt.p.x=x2;	rt.p.y=y1;}
	IC	void	setdepth(float z = 0.f, float rhw = 1.f)		{ lb.p.z=z;		rb.p.z= z;	lt.p.z = z;	rt.p.z = z;	lb.p.w = rhw; rb.p.w = rhw; lt.p.w = rhw; rt.p.w = rhw;};
	IC	void	setcolor(u32 c)								{ lb.color=c; rb.color=c; lt.color=c; rt.color=c; };
	IC	void	settex	(Frect &r)								{ lb.uv.x = r.x1; 	lb.uv.y = r.y2; lt.uv.x = r.x1; lt.uv.y = r.y1; rb.uv.x = r.x2; rb.uv.y = r.y2;	rt.uv.x = r.x2; rt.uv.y = r.y1; }
	IC	void	settex	(float x1, float y1, float x2, float y2){ lb.uv.x=x1;		lb.uv.y=y2;		lt.uv.x=x1;		lt.uv.y=y1;		rb.uv.x=x2;		rb.uv.y=y2;		rt.uv.x=x2;		rt.uv.y=y1;}

	IC	void	to_real	(int w, int h)
	{
		lt.p.x=lb.p.x=TO_REAL(lb.p.x, w);	rb.p.x=rt.p.x=TO_REAL(rb.p.x, w);
		lt.p.y=rt.p.y=TO_REAL(lt.p.y, h);	lb.p.y=rb.p.y=TO_REAL(lb.p.y, h);
	}

	IC	void	to_real	(Frect &r, int w, int h)
	{
		lt.p.x=lb.p.x=TO_REAL(r.x1, w);	lt.p.y=rt.p.y=TO_REAL(r.y1, h);
		rb.p.x=rt.p.x=TO_REAL(r.x2, w);	lb.p.y=rb.p.y=TO_REAL(r.y2, h);
	}

	IC	void	add	(float dx, float dy)
	{
		lb.p.x+=dx;	lb.p.y+=dy;	lt.p.x+=dx;	lt.p.y+=dy;
		rb.p.x+=dx;	rb.p.y+=dy;	rt.p.x+=dx;	rt.p.y+=dy;
	}

	IC	void	add	(Frect &r, float dx, float dy)
	{
		lb.p.x=r.x1+dx;	lb.p.y=r.y2+dy;	lt.p.x=r.x1+dx;	lt.p.y=r.y1+dy;
		rb.p.x=r.x2+dx;	rb.p.y=r.y2+dy;	rt.p.x=r.x2+dx;	rt.p.y=r.y1+dy;
	}

	IC	void	sub	(float dx, float dy)
	{
		lb.p.x-=dx;	lb.p.y-=dy;	lt.p.x-=dx;	lt.p.y-=dy;
		rb.p.x-=dx;	rb.p.y-=dy;	rt.p.x-=dx;	rt.p.y-=dy;
	}

	IC	void	sub	(Frect &r, float dx, float dy)
	{
		lb.p.x=r.x1-dx;	lb.p.y=r.y2-dy;	lt.p.x=r.x1-dx;	lt.p.y=r.y1-dy;
		rb.p.x=r.x2-dx;	rb.p.y=r.y2-dy;	rt.p.x=r.x2-dx;	rt.p.y=r.y1-dy;
	}
} FTLpolygon;

class	ECORE_API CTLSprite
{
	FTLpolygon		mesh;
public:
					CTLSprite	( );
	virtual			~CTLSprite	( );
	void			Render		( Fvector &pos, u32 color, float radius, float angle );
	IC void			Render		( Fvector &pos, u32 color, float radius, float angle, const Fvector2& lt, const Fvector2& rb ){
    	mesh.settex	(lt.x,lt.y,rb.x,rb.y);
    	Render		(pos,color,radius,angle);
    }
	void			Render		( Fvector &pos, u32 color, float radius, const Fvector& D );
	IC void			Render		( Fvector &pos, u32 color, float radius, const Fvector& D, const Fvector2& lt, const Fvector2& rb){
    	mesh.settex	(lt.x,lt.y,rb.x,rb.y);
    	Render		(pos,color,radius,D);
    }
	void			Render		( Fvector &pos, float radius, bool bFixedSize, u32 clr=0xffffffff );
};

#endif //__TLSPRITE_H__
