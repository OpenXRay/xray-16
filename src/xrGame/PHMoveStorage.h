#ifndef PHMOVESTORAGE_H
#define PHMOVESTORAGE_H
#include "phgeometryowner.h"
//DEFINE_VECTOR(dReal *&,POSITIONS_STORAGE,POSITIONS_I);

class CPHPositionsPairs
{
	GEOM_I geom;
public:
	CPHPositionsPairs(GEOM_I i)
	{
		geom=i;
	}
	void Positions(const Fvector *&p0,const Fvector *&p1);
	IC CPHPositionsPairs& operator ++	()
	{
		++geom;
		return *this;
	}
	IC dGeomID dGeom()
	{
		return (*geom)->geometry_transform();
	}
	IC CPHPositionsPairs& operator ++	(int)
	{
		geom++;
		return *this;
	}
	IC CPHPositionsPairs& operator =	(const CPHPositionsPairs& right)
	{
		geom=right.geom;
	}
	IC bool operator ==	(const CPHPositionsPairs& right ) const
	{
		return geom==right.geom;
	}
	IC bool operator !=	(const CPHPositionsPairs& right ) const
	{
		return geom!=right.geom;
	}
};

class CPHMoveStorage
{
	GEOM_STORAGE m_trace_geometries;
public:
	typedef CPHPositionsPairs iterator;
	IC	iterator	begin	()					{return	CPHPositionsPairs(m_trace_geometries.begin());}
	IC	iterator	end		()					{return	CPHPositionsPairs(m_trace_geometries.end());}
	IC	bool		empty	()const				{return m_trace_geometries.empty();}
		void		add		(CODEGeom* g)		{m_trace_geometries.push_back(g);}
		void		clear	()					{m_trace_geometries.clear();}
};

#endif