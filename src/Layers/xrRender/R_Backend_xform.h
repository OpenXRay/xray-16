#ifndef r_backend_xformH
#define r_backend_xformH
#pragma once

class ECORE_API	R_xforms
{
public:
	Fmatrix			m_w;		// Basic	- world
	Fmatrix			m_invw;		// derived	- world2local, cached
	Fmatrix			m_v;		// Basic	- view
	Fmatrix			m_p;		// Basic	- projection
	Fmatrix			m_wv;		// Derived	- world2view
	Fmatrix			m_vp;		// Derived	- view2projection
	Fmatrix			m_wvp;		// Derived	- world2view2projection

	R_constant*		c_w;
	R_constant*		c_invw;
	R_constant*		c_v;
	R_constant*		c_p;
	R_constant*		c_wv;
	R_constant*		c_vp;
	R_constant*		c_wvp;
private:
	bool			m_bInvWValid;
public:
	R_xforms		();
	void			unmap		();
	void			set_W		(const Fmatrix& m);
	void			set_V		(const Fmatrix& m);
	void			set_P		(const Fmatrix& m);
	IC const Fmatrix&	get_W	()					{ return m_w;	}
	IC const Fmatrix&	get_V	()					{ return m_v;	}
	IC const Fmatrix&	get_P	()					{ return m_p;	}
	IC void			set_c_w		(R_constant* C);
	IC void			set_c_invw	(R_constant* C);
	IC void			set_c_v		(R_constant* C);
	IC void			set_c_p		(R_constant* C);
	IC void			set_c_wv	(R_constant* C);
	IC void			set_c_vp	(R_constant* C);
	IC void			set_c_wvp	(R_constant* C);
private:
	void			apply_invw	();
};
#endif
