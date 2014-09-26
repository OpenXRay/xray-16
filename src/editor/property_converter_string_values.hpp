////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_string_values.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter string values class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONVERTER_STRING_VALUES_HPP_INCLUDED
#define PROPERTY_CONVERTER_STRING_VALUES_HPP_INCLUDED

public ref class property_converter_string_values : public System::ComponentModel::StringConverter {
private:
	typedef System::ComponentModel::StringConverter			inherited;

protected:
	typedef System::ComponentModel::ITypeDescriptorContext	ITypeDescriptorContext;
	typedef System::Collections::ICollection				ICollection;
	typedef Flobbster::Windows::Forms::PropertyBag			PropertyBag;
	typedef PropertyBag::PropertySpecDescriptor				PropertySpecDescriptor;
	typedef System::Type									Type;

public:
	typedef System::ComponentModel::TypeConverter::StandardValuesCollection	StandardValuesCollection;

public:
	virtual bool GetStandardValuesSupported				(ITypeDescriptorContext^ context) override;
	virtual bool GetStandardValuesExclusive				(ITypeDescriptorContext^ context) override;
	virtual StandardValuesCollection ^GetStandardValues	(ITypeDescriptorContext^ context) override;
	virtual	bool CanConvertFrom							(ITypeDescriptorContext^ context, Type^ source_type) override;
}; // ref class property_converter_string_values

public ref class property_converter_string_values_no_enter : public property_converter_string_values {
public:
	virtual	bool CanConvertFrom							(ITypeDescriptorContext^ context, Type^ source_type) override
	{
		return	(false);
	}
}; // ref class property_converter_string_values_no_enter

#endif // ifndef PROPERTY_CONVERTER_STRING_VALUES_HPP_INCLUDED