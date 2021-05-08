////////////////////////////////////////////////////////////////////////////
//	Module 		: property_holder.cpp
//	Created 	: 06.12.2007
//  Modified 	: 06.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property holder implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_holder.hpp"
#include "property_container.hpp"

using Flobbster::Windows::Forms::PropertySpec;
using System::String;
using XRay::Editor::property_holder_collection;
using XRay::Editor::property_holder_holder;

typedef property_holder::collection_type collection_type;

property_holder::property_holder(XRay::Editor::engine_base* engine, LPCSTR display_name, property_holder_collection* collection,
    XRay::Editor::property_holder_holder* holder)
    : m_engine(engine), m_display_name(to_string(display_name)), m_collection(collection), m_holder(holder),
      m_disposing(false)
{
    m_container = gcnew property_container(this, nullptr);
}

property_holder::~property_holder()
{
    if (m_disposing)
        return;

    m_disposing = true;
}

void property_holder::on_dispose()
{
    if (m_disposing)
        return;

    VERIFY(m_collection);
    int index = m_collection->index(this);
    if (index < 0)
    {
        m_collection->destroy(this);
        return;
    }

    VERIFY((u32)index < m_collection->size());
    m_collection->erase(index);
}

property_container ^ property_holder::container() { return (m_container); }
XRay::Editor::engine_base& property_holder::engine()
{
    VERIFY(m_engine);
    return (*m_engine);
}

property_holder_holder* property_holder::holder() { return (m_holder); }
String ^ property_holder::display_name() { return (m_display_name); }
collection_type* property_holder::collection() { return (m_collection); }
void property_holder::clear() { m_container->clear(); }
