#pragma once
#include "alife_abstract_registry.h"

class CMapLocation;
struct SLocationKey : public IPureSerializeObject<IReader,IWriter>,public IPureDestroyableObject {
	shared_str		spot_type;
	u16				object_id;
	CMapLocation*	location;
	bool			actual;
	SLocationKey (shared_str s, u16 id):spot_type(s),object_id(id),location(NULL),actual(true){};
	SLocationKey ():spot_type(NULL),object_id(0),location(NULL),actual(true){};

	bool operator < (const SLocationKey& key)const
	{
		if(actual == key.actual)
			return location<key.location;
		else
			return  actual > key.actual;
	} //move non-actual to tail
	
	virtual void save								(IWriter &stream);
	virtual void load								(IReader &stream);
	virtual void destroy							();
};

DEFINE_VECTOR (SLocationKey, Locations, Locations_it);

struct CMapLocationRegistry : public CALifeAbstractRegistry<u16, Locations> {
	virtual void save(IWriter &stream);
};
