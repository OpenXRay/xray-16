#ifndef XRMUMODEL_REFERENCE_H
#define XRMUMODEL_REFERENCE_H

#include "base_color.h"
#include "serialize.h"

class xrMU_Model;
namespace CDB { class CollectorPacked; }
class INetReader;
class IWriter;
class XRLC_LIGHT_API xrMU_Reference
{
public:
	xrMU_Model*				model;
    Fmatrix					xform;
    Flags32					flags;
	u16						sector;

	xr_vector<base_color>	color;

	base_color_c			c_scale;
	base_color_c			c_bias;
public:
							xrMU_Reference		(): model(0), sector(u16(-1)), flags(Flags32().assign(0)), xform(Fidentity){}

	void					Load				( IReader& fs, xr_vector<xrMU_Model*>& mu_models );
	void					calc_lighting		();

	void					export_cform_game	(CDB::CollectorPacked& CL);
	void					export_cform_rcast	(CDB::CollectorPacked& CL);

	void					read				( INetReader	&r );
	void					write				( IWriter	&w ) const ;
	void					receive_result		( INetReader	&r );
	void					send_result			( IWriter	&w ) const;
	static	xrMU_Reference* read_create()		{ return xr_new<xrMU_Reference>(); };


//	void					export_ogf			();
};

typedef  vector_serialize< t_read<xrMU_Reference, get_id_standart<xrMU_Reference> >  >		tread_mu_refs;
typedef  vector_serialize< t_write<xrMU_Reference, get_id_standart<xrMU_Reference> > >		twrite_mu_refs;

extern	tread_mu_refs		*read_mu_refs		;
extern	twrite_mu_refs		*write_mu_refs		;
#endif