////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover.h
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover storage class
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_STORAGE_H_INCLUDED
#define SMART_COVER_STORAGE_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover.h"

namespace smart_cover {

class storage : 
	private debug::make_final<storage>, 
	private boost::noncopyable 
{
public:
	typedef xr_vector<smart_cover::description*>		Descriptions;
	typedef cover::DescriptionPtr						DescriptionPtr;

private:
	Descriptions	m_descriptions;

public:
							~storage		();
			DescriptionPtr	description		(shared_str const &table_id);
			void			collect_garbage	();
};

} //namespace smart_cover

#endif //SMART_COVER_STORAGE_H_INCLUDED