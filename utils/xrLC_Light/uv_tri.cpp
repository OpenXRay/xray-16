#include "stdafx.h"

#include "uv_tri.h"
#include "xrface.h"



void UVtri ::read( INetReader	&r )
{
	_TCF::read( r );
	VERIFY( read_faces );
	owner = 0;
	read_faces->read( r, owner );
}
void UVtri ::write( IWriter	&w ) const
{
	_TCF::write( w );
	VERIFY( owner );
	VERIFY( write_faces );
	write_faces->write( w, owner );
}

bool	UVtri::similar	( const UVtri &uv, float eps/*eps = EPS*/ ) const
{
	return uv.owner == owner && _TCF::similar( uv, eps );
}