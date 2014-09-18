////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_integer_enum.hpp
//	Created 	: 12.12.2007
//  Modified 	: 12.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property converter integer enum class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONVERTER_INTEGER_ENUM_HPP_INCLUDED
#define PROPERTY_CONVERTER_INTEGER_ENUM_HPP_INCLUDED

public ref class property_converter_integer_enum : public System::ComponentModel::StringConverter {
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
}; // ref class property_converter_integer_enum

#endif // ifndef PROPERTY_CONVERTER_INTEGER_ENUM_HPP_INCLUDED