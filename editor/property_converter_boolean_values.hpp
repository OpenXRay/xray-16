////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_boolean_values.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter boolean values class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONVERTER_BOOLEAN_VALUES_HPP_INCLUDED
#define PROPERTY_CONVERTER_BOOLEAN_VALUES_HPP_INCLUDED

public ref class property_converter_boolean_values : public System::ComponentModel::StringConverter {
private:
	typedef System::ComponentModel::StringConverter			inherited;
	typedef System::Collections::ICollection				ICollection;
	typedef Flobbster::Windows::Forms::PropertyBag			PropertyBag;
	typedef PropertyBag::PropertySpecDescriptor				PropertySpecDescriptor;
	typedef System::ComponentModel::ITypeDescriptorContext	ITypeDescriptorContext;
	typedef System::Globalization::CultureInfo				CultureInfo;
	typedef System::Object									Object;
	typedef System::Type									Type;

public:
	typedef System::ComponentModel::TypeConverter::StandardValuesCollection	StandardValuesCollection;

public:
	virtual bool						GetStandardValuesSupported	(ITypeDescriptorContext^ context) override;
	virtual bool						GetStandardValuesExclusive	(ITypeDescriptorContext^ context) override;
	virtual StandardValuesCollection^	GetStandardValues			(ITypeDescriptorContext^ context) override;
	virtual Object^						ConvertTo					(
											ITypeDescriptorContext^ context,
											CultureInfo^ culture,
											Object^ value,
											Type^ destination_type
										) override;
}; // ref class property_converter_boolean_values

#endif // ifndef PROPERTY_CONVERTER_BOOLEAN_VALUES_HPP_INCLUDED