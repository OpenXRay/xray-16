

IC void CPHElement::InverceLocalForm(Fmatrix& m)
{
	m.identity();
	m.c.set(m_mass_center);
	m.invert();
}

IC void CPHElement::MulB43InverceLocalForm(Fmatrix& m) const
{
	Fvector ic;ic.set(m_mass_center);
	ic.invert();
	m.transform_dir(ic);
	m.c.add(ic);
}