////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_abstract_registry_inline.h
//	Created 	: 30.06.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife abstract registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <typename _index_type, typename _data_type>
#define CSALifeAbstractRegistry CALifeAbstractRegistry<_index_type,_data_type>

TEMPLATE_SPECIALIZATION
IC	CSALifeAbstractRegistry::CALifeAbstractRegistry	()
{
}

TEMPLATE_SPECIALIZATION
CSALifeAbstractRegistry::~CALifeAbstractRegistry		()
{
	delete_data		(m_objects);
}

TEMPLATE_SPECIALIZATION
void CSALifeAbstractRegistry::save					(IWriter &memory_stream)
{
	save_data		(m_objects,memory_stream);
}

TEMPLATE_SPECIALIZATION
void CSALifeAbstractRegistry ::load					(IReader &file_stream)
{
	load_data		(m_objects,file_stream);
}

TEMPLATE_SPECIALIZATION
IC	const typename CSALifeAbstractRegistry::OBJECT_REGISTRY &CSALifeAbstractRegistry::objects	() const
{
	return			(m_objects);
}

TEMPLATE_SPECIALIZATION
IC	void CSALifeAbstractRegistry::add				(const _index_type &index, _data_type &data, bool no_assert)
{
	const_iterator	I = objects().find(index);
	if (I != objects().end()) {
		THROW2		(no_assert,"Specified object has been already found in the specified registry!");
		return;
	}
	m_objects.insert(std::make_pair(index,data));
}

TEMPLATE_SPECIALIZATION
IC	void CSALifeAbstractRegistry::remove			(const _index_type &index, bool no_assert)
{
	iterator		I = m_objects.find(index);
	if (I == objects().end()) {
		THROW2		(no_assert,"Specified object hasn't been found in the specified registry!");
		return;
	}
	m_objects.erase	(I);
}

TEMPLATE_SPECIALIZATION
IC	_data_type *CSALifeAbstractRegistry::object	(const _index_type &index, bool no_assert)
{
	iterator		I = m_objects.find(index);
	if (I == objects().end()) {
		THROW2		(no_assert,"Specified object hasn't been found in the specified registry!");
		return		(0);
	}
	return			(&(*I).second);
}

#undef TEMPLATE_SPECIALIZATION