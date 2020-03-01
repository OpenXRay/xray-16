////////////////////////////////////////////////////////////////////////////
//	Module 		: property_integer_values_value_getter.cpp
//	Created 	: 09.01.2008
//  Modified 	: 09.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property integer values value getter class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_integer_values_value_getter.hpp"

using System::Collections::IList;
using System::Collections::ArrayList;
using System::Object;
using System::String;

property_integer_values_value_getter::property_integer_values_value_getter(integer_getter_type const& getter,
    integer_setter_type const& setter, string_collection_getter_type const& collection_getter,
    string_collection_size_getter_type const& collection_size_getter)
    : inherited(getter, setter), m_collection_getter(new string_collection_getter_type(collection_getter)),
      m_collection_size_getter(new string_collection_size_getter_type(collection_size_getter))
{
}

property_integer_values_value_getter::~property_integer_values_value_getter()
{
    this->!property_integer_values_value_getter();
}

property_integer_values_value_getter::!property_integer_values_value_getter()
{
    delete (m_collection_getter);
    delete (m_collection_size_getter);
}

Object ^ property_integer_values_value_getter::GetValue()
{
    int value = safe_cast<int>(inherited::GetValue());
    if (value < 0)
        value = 0;

    int count = collection()->Count;
    if (value >= count)
        value = count - 1;

    return (value);
}

void property_integer_values_value_getter::SetValue(Object ^ object)
{
    String ^ string_value = dynamic_cast<String ^>(object);
    int index = collection()->IndexOf(string_value);
    VERIFY((index >= 0));
    inherited::SetValue(index);
}

IList ^ property_integer_values_value_getter::collection()
{
    ArrayList ^ collection = gcnew ArrayList();
    LPCSTR const* values = (*m_collection_getter)();
    for (u32 i = 0, n = (*m_collection_size_getter)(); i < n; ++i)
        collection->Add(to_string(values[i]));

    return (collection);
}
