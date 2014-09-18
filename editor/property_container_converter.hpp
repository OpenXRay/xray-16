////////////////////////////////////////////////////////////////////////////
//	Module 		: property_container_converter.hpp
//	Created 	: 11.12.2007
//  Modified 	: 11.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property container converter class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONTAINER_CONVERTER_HPP_INCLUDED
#define PROPERTY_CONTAINER_CONVERTER_HPP_INCLUDED

public ref class property_container_converter : public System::ComponentModel::ExpandableObjectConverter {
private:
	typedef System::ComponentModel::ExpandableObjectConverter		inherited;
	typedef System::ComponentModel::PropertyDescriptorCollection	PropertyDescriptorCollection;
	typedef System::ComponentModel::ITypeDescriptorContext			ITypeDescriptorContext;
	typedef System::Object											Object;
	typedef System::Attribute										Attribute;
	typedef System::Type											Type;
	typedef System::Globalization::CultureInfo						CultureInfo;

public:
	typedef System::ComponentModel::TypeConverter::StandardValuesCollection	StandardValuesCollection;

public:
	virtual	PropertyDescriptorCollection^	GetProperties			(
												ITypeDescriptorContext^ context,
												Object^ value,
												array<Attribute^>^ attributes
											) override;
	virtual	bool							GetPropertiesSupported	(ITypeDescriptorContext^ context) override;
	virtual	bool							CanConvertTo			(
												ITypeDescriptorContext^ context,
												Type^ destination_type
											) override;
	virtual Object^							ConvertTo				(
												ITypeDescriptorContext^ context,
												CultureInfo^ culture,
												Object^ value,
												Type^ destination_type
											) override;
}; // ref class property_container_converter

#endif // ifndef PROPERTY_CONTAINER_CONVERTER_HPP_INCLUDED