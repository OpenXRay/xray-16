////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_shared_str.cpp
//	Created 	: 19.12.2007
//  Modified 	: 19.12.2007
//	Author		: Dmitriy Iassenev
//	Description : string property for shared_str implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "property_string_shared_str.hpp"
#include "engine_include.hpp"

property_string_shared_str::property_string_shared_str(XRay::Editor::engine_base* engine, shared_str& value)
    : m_engine(engine), m_value(&value)
{
}

property_string_shared_str::~property_string_shared_str() { this->!property_string_shared_str(); }
property_string_shared_str::!property_string_shared_str() {}
System::Object ^ property_string_shared_str::GetValue() { return (to_string(m_engine->value(*m_value))); }
void property_string_shared_str::SetValue(System::Object ^ object)
{
    LPSTR result = to_string(safe_cast<System::String ^>(object));
    m_engine->value(result, *m_value);
    free(result);
}
