////////////////////////////////////////////////////////////////////////////
//	Module 		: property_string_shared_str.hpp
//	Created 	: 19.12.2007
//  Modified 	: 19.12.2007
//	Author		: Dmitriy Iassenev
//	Description : string property for shared_str implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_STRING_SHARED_STR_HPP_INCLUDED
#define PROPERTY_STRING_SHARED_STR_HPP_INCLUDED

#include "property_holder_include.hpp"

namespace XRay
{
namespace Editor
{
class engine_base;
} // namespace editor
} // namespace XRay

public ref class property_string_shared_str : public XRay::SdkControls::IProperty
{
public:
    property_string_shared_str(XRay::Editor::engine_base* engine, shared_str& value);
    virtual ~property_string_shared_str();
    !property_string_shared_str();
    virtual System::Object ^ GetValue();
    virtual void SetValue(System::Object ^ object);

private:
    XRay::Editor::engine_base* m_engine;
    shared_str* m_value;
}; // ref class property_string_shared_str

#endif // ifndef PROPERTY_STRING_SHARED_STR_HPP_INCLUDED
