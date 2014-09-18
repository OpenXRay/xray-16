#pragma once


class CPHMesh;
class CODEGeom;
class CPHGeometryBits
{
	public:
	static	void init_geom( CODEGeom &g );
	static	void init_geom( CPHMesh &g  );
	static	void set_ignore_static( CODEGeom &g  );
};