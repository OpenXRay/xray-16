#pragma once

//----------------------------------------------------------------------------
IC MagicBox3::MagicBox3 ()
{
    // no initialization for efficiency
}

IC MagicBox3::MagicBox3 (const Fmatrix &m, const Fvector &half_size)
{
	m_kCenter		= m.c;
	m_akAxis[0]		= m.i;
	m_akAxis[1]		= m.j;
	m_akAxis[2]		= m.k;
	m_afExtent[0]	= half_size.x;
	m_afExtent[1]	= half_size.y;
	m_afExtent[2]	= half_size.z;
}

//----------------------------------------------------------------------------
IC Fvector& MagicBox3::Center ()
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
IC const Fvector& MagicBox3::Center () const
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
IC Fvector& MagicBox3::Axis (int i)
{
    VERIFY( 0 <= i && i < 3 );
    return m_akAxis[i];
}
//----------------------------------------------------------------------------
IC const Fvector& MagicBox3::Axis (int i) const
{
    VERIFY( 0 <= i && i < 3 );
    return m_akAxis[i];
}
//----------------------------------------------------------------------------
IC Fvector* MagicBox3::Axes ()
{
    return m_akAxis;
}
//----------------------------------------------------------------------------
IC const Fvector* MagicBox3::Axes () const
{
    return m_akAxis;
}
//----------------------------------------------------------------------------
IC float& MagicBox3::Extent (int i)
{
    VERIFY( 0 <= i && i < 3 );
    return m_afExtent[i];
}
//----------------------------------------------------------------------------
IC const float& MagicBox3::Extent (int i) const
{
    VERIFY( 0 <= i && i < 3 );
    return m_afExtent[i];
}
//----------------------------------------------------------------------------
IC float* MagicBox3::Extents ()
{
    return m_afExtent;
}
//----------------------------------------------------------------------------
IC const float* MagicBox3::Extents () const
{
    return m_afExtent;
}
//----------------------------------------------------------------------------

