////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_converter.hpp
//	Created 	: 24.12.2007
//  Modified 	: 25.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property converter implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_CONVERTER_HPP_INCLUDED
#define PROPERTY_COLLECTION_CONVERTER_HPP_INCLUDED

public ref class property_collection_converter : public System::ComponentModel::TypeConverter {
public:
	typedef System::ComponentModel::ITypeDescriptorContext	ITypeDescriptorContext;
	typedef System::Globalization::CultureInfo				CultureInfo;
	typedef System::Type									Type;
	typedef System::Object									Object;

public:
	virtual	bool	CanConvertTo	(
						ITypeDescriptorContext^ context,
						Type^ type
					) override;
	virtual	Object^	ConvertTo		(
						ITypeDescriptorContext^ context,
						CultureInfo^ culture,
						Object^ value,
						Type^ type
					) override;
}; // ref class property_collection_converter

#endif // ifndef PROPERTY_COLLECTION_CONVERTER_HPP_INCLUDED