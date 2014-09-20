////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_thunderbolts_thunderbolt_id.cpp
//	Created 	: 04.01.2008
//  Modified 	: 04.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment thunderbolts thunderbolt identifier class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_thunderbolts_thunderbolt_id.hpp"
#include "ide.hpp"
#include "editor_environment_thunderbolts_manager.hpp"

using editor::environment::thunderbolts::thunderbolt_id;
using editor::environment::thunderbolts::manager;

thunderbolt_id::thunderbolt_id			(manager const& manager, shared_str const& id) :
	m_manager						(manager),
	m_id							(id),
	m_property_holder				(0)
{
}

thunderbolt_id::~thunderbolt_id			()
{
	if (!Device.editor())
		return;

	::ide().destroy					(m_property_holder);
}

LPCSTR const* thunderbolt_id::collection()
{
	return							(&*m_manager.thunderbolts_ids().begin());
}

u32 thunderbolt_id::collection_size		()
{
	return							(m_manager.thunderbolts_ids().size());
}

void thunderbolt_id::fill				(editor::property_holder_collection* collection)
{
	VERIFY							(!m_property_holder);
	m_property_holder				= ::ide().create_property_holder(m_id.c_str(), collection, this);

	typedef editor::property_holder::string_collection_getter_type	collection_getter_type;
	collection_getter_type			collection_getter;
	collection_getter.bind			(this, &thunderbolt_id::collection);

	typedef editor::property_holder::string_collection_size_getter_type	collection_size_getter_type;
	collection_size_getter_type		collection_size_getter;
	collection_size_getter.bind		(this, &thunderbolt_id::collection_size);

	m_property_holder->add_property	(
		"thunderbolt",
		"properties",
		"this option is resposible for thunderbolt",
		m_id.c_str(),
		m_id,
		collection_getter,
		collection_size_getter,
		editor::property_holder::value_editor_combo_box,
		editor::property_holder::cannot_enter_text
	);
}

thunderbolt_id::property_holder_type* thunderbolt_id::object	()
{
	return							(m_property_holder);
}

#endif // #ifdef INGAME_EDITOR