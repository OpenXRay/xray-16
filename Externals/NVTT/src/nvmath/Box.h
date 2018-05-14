// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_BOX_H
#define NV_MATH_BOX_H

#include <nvmath/Vector.h>

#include <float.h> // FLT_MAX

namespace nv
{

/// Axis Aligned Bounding Box.
class Box
{
public:

	/// Default ctor.
	Box() { };

	/// Copy ctor.
	Box( const Box & b ) : m_mins(b.m_mins), m_maxs(b.m_maxs) { }

	/// Init ctor.
	Box( Vector3::Arg mins, Vector3::Arg maxs ) : m_mins(mins), m_maxs(maxs) { }

	// Cast operators.
	operator const float * () const { return reinterpret_cast<const float *>(this); }

	/// Min corner of the box.
	Vector3 mins() const { return m_mins; }

	/// Max corner of the box.
	Vector3 maxs() const { return m_maxs; }

	/// Clear the bounds.
	void clearBounds()
	{
		m_mins.set(FLT_MAX, FLT_MAX, FLT_MAX);
		m_maxs.set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	}

	/// Build a cube centered on center and with edge = 2*dist
	void cube(Vector3::Arg center, float dist)
	{
		setCenterExtents(center, Vector3(dist, dist, dist));
	}

	/// Build a box, given center and extents.
	void setCenterExtents(Vector3::Arg center, Vector3::Arg extents)
	{
		m_mins = center - extents;
		m_maxs = center + extents;
	}

	/// Get box center.
	Vector3 center() const
	{
		return (m_mins + m_maxs) * 0.5f;
	}

	/// Return extents of the box.
	Vector3 extents() const
	{
		return (m_maxs - m_mins) * 0.5f;
	}

	/// Return extents of the box.
	scalar extents(uint axis) const
	{
		nvDebugCheck(axis < 3);
		if (axis == 0) return (m_maxs.x() - m_mins.x()) * 0.5f;
		if (axis == 1) return (m_maxs.y() - m_mins.y()) * 0.5f;
		if (axis == 2) return (m_maxs.z() - m_mins.z()) * 0.5f;
		nvAssume(false);
		return 0.0f;
	}

	/// Add a point to this box.
	void addPointToBounds(Vector3::Arg p)
	{
		m_mins = min(m_mins, p);
		m_maxs = max(m_maxs, p);
	}

	/// Add a box to this box.
	void addBoxToBounds(const Box & b)
	{
		m_mins = min(m_mins, b.m_mins);
		m_maxs = max(m_maxs, b.m_maxs);
	}

	/// Translate box.
	void translate(Vector3::Arg v)
	{
		m_mins += v;
		m_maxs += v;
	}

	/// Scale the box.
	void scale(float s)
	{
		m_mins *= s;
		m_maxs *= s;
	}

	/// Get the area of the box.
	float area() const
	{
		const Vector3 d = extents();
		return 8.0f * (d.x()*d.y() + d.x()*d.z() + d.y()*d.z());
	}	

	/// Get the volume of the box.
	float volume() const
	{
		Vector3 d = extents();
		return 8.0f * (d.x() * d.y() * d.z());
	}
	
	/// Return true if the box contains the given point.
	bool contains(Vector3::Arg p) const
	{
		return 
			m_mins.x() < p.x() && m_mins.y() < p.y() && m_mins.z() < p.z() &&
			m_maxs.x() > p.x() && m_maxs.y() > p.y() && m_maxs.z() > p.z();
	}

private:

	Vector3 m_mins;
	Vector3 m_maxs;
};



} // nv namespace


#endif // NV_MATH_BOX_H
