#pragma once

struct L_point {
	int x,y;
	IC void set(int _x, int _y)
	{	x=_x; y=_y;	}
	IC void set(L_point &P)
	{	set(P.x,P.y); }
};
struct L_rect {
	L_point	a,b;		// min,max
	int		iArea;
	
	IC void	set	(L_rect &R)
	{
		a.set	(R.a);
		b.set	(R.b);
		iArea	= R.iArea;
	};
	IC void init(int ax, int ay, int bx, int by)
	{	a.set(ax,ay); b.set(bx,by); }
	IC void	calc_area()
	{	iArea = SizeX()*SizeY(); 	};
	IC bool	PointInside(L_point &P)
	{	return (P.x>=a.x && P.x<=b.x && P.y>=a.y && P.y<=b.y); 	};
	IC bool	Intersect(L_rect &R)
	{
		if (R.b.x < a.x) return false;
		if (R.b.y < a.y) return false;
		if (R.a.x > b.x) return false;
		if (R.a.y > b.y) return false;
		return true;
	};
	IC void	GetAB(L_point &A, L_point &B)
	{
		A.x = b.x; A.y = a.y;
		B.x = a.x; B.y = b.y;
	};
	IC void	Invalidate()
	{
		a.set(SHRT_MAX,SHRT_MAX);
		b.set(SHRT_MIN,SHRT_MIN);
	}
	IC void	Merge(L_rect &R) 
	{
		if (R.a.x<a.x) a.x=R.a.x;
		if (R.a.y<a.y) a.y=R.a.y;
		if (R.b.x>b.x) b.x=R.b.x;
		if (R.b.y>b.y) b.y=R.b.y;
	}
	IC int	SizeX()
	{	return b.x-a.x+1; }
	IC int	SizeY()
	{	return b.y-a.y+1; }
};

