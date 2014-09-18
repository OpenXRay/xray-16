#include "stdafx.h"
#ifdef	DEBUG
std::string get_string( const Fvector& v )
{
	return make_string	("( %f, %f, %f )", v.x, v.y, v.z );
}
std::string get_string( const Fmatrix& dop )
{
	return make_string	("\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n",
		dop.i.x, dop.i.y, dop.i.z, dop._14_,
		dop.j.x, dop.j.y, dop.j.z, dop._24_,
		dop.k.x, dop.k.y, dop.k.z, dop._34_,
		dop.c.x, dop.c.y, dop.c.z, dop._44_
	);
}
std::string get_string(const Fbox &box)
{
	return make_string( "[ min: %s - max: %s ]", get_string( box.min ).c_str(), get_string( box.max ).c_str() );
}
std::string get_string( bool v )
{
	return v ? std::string( "true" ) : std::string( "false" );
}



std::string dump_string( LPCSTR name, const Fvector &v )
{
	return make_string( "%s : (%f,%f,%f) ", name, v.x, v.y, v.z );
}

void dump( LPCSTR name, const Fvector &v )
{
	Msg( "%s", dump_string( name, v ).c_str() );
}

std::string dump_string( LPCSTR name, const Fmatrix &form )
{
	return 
	make_string( "%s, _14_=%f \n", dump_string( make_string( "%s.i, ", name ).c_str(), form.i ).c_str( ) , form._14_ )	+ 
	make_string( "%s, _24_=%f \n", dump_string( make_string( "%s.j, ", name ).c_str(), form.j ).c_str( ) , form._24_ )	+  
	make_string( "%s, _34_=%f \n", dump_string( make_string( "%s.k, ", name ).c_str(), form.k ).c_str( ) , form._34_  ) +  
	make_string( "%s, _44_=%f \n", dump_string( make_string( "%s.c, ", name ).c_str(), form.c ).c_str( ) , form._44_ );  
}

void dump( LPCSTR name, const Fmatrix &form )
{
	Msg( "%s", dump_string( name, form ) );
	//Msg( "%s, _14_=%f ", dump_string( make_string( "%s.i, ", name ).c_str(), form.i ).c_str( ) , form._14_ );  
	//Msg( "%s, _24_=%f ", dump_string( make_string( "%s.j, ", name ).c_str(), form.j ).c_str( ) , form._24_ );  
	//Msg( "%s, _34_=%f ", dump_string( make_string( "%s.k, ", name ).c_str(), form.k ).c_str( ) , form._34_  );  
	//Msg( "%s, _44_=%f ", dump_string( make_string( "%s.c, ", name ).c_str(), form.c ).c_str( ) , form._44_ );  
}

#endif