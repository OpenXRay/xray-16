////////////////////////////////////////////////////////////////////////////
//	Module 		: property_converter_float.hpp
//	Created 	: 23.06.2008
//  Modified 	: 23.06.2008
//	Author		: Dmitriy Iassenev
//	Description : property converter float class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONVERTER_FLOAT_HPP_INCLUDED
#define PROPERTY_CONVERTER_FLOAT_HPP_INCLUDED

public ref class property_converter_float : public System::ComponentModel::TypeConverter {
private:
	typedef System::ComponentModel::TypeConverter					inherited;
	typedef System::ComponentModel::PropertyDescriptorCollection	PropertyDescriptorCollection;
	typedef System::ComponentModel::ITypeDescriptorContext			ITypeDescriptorContext;
	typedef System::Object											Object;
	typedef System::Attribute										Attribute;
	typedef System::Globalization::CultureInfo						CultureInfo;
	typedef System::Type											Type;

public:
	virtual	bool	CanConvertTo	(
						ITypeDescriptorContext^ context,
						Type^ destination_type
					) override;
	virtual Object^	ConvertTo		(
						ITypeDescriptorContext^ context,
						CultureInfo^ culture,
						Object^ value,
						Type^ destination_type
					) override;
	virtual	bool	CanConvertFrom	(
						ITypeDescriptorContext^ context,
						Type^ source_type
					) override;
	virtual Object^	ConvertFrom		(
						ITypeDescriptorContext^ context,
						CultureInfo^ culture,
						Object^ value
					) override;
}; // ref class property_converter_float

#endif // ifndef PROPERTY_CONVERTER_FLOAT_HPP_INCLUDED