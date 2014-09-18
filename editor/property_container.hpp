////////////////////////////////////////////////////////////////////////////
//	Module 		: property_container.hpp
//	Created 	: 07.12.2007
//  Modified 	: 07.12.2007
//	Author		: Dmitriy Iassenev
//	Description : property container class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_CONTAINER_HPP_INCLUDED
#define PROPERTY_CONTAINER_HPP_INCLUDED

interface class property_value;
class property_holder;
ref class property_container_converter;

interface class property_container_holder;

[System::ComponentModel::TypeConverterAttribute(property_container_converter::typeid)]
public ref class property_container :
	public Flobbster::Windows::Forms::PropertyBag,
	public property_container_interface
{
public:
	typedef Flobbster::Windows::Forms::PropertySpec				PropertySpec;
	typedef Flobbster::Windows::Forms::PropertySpecEventArgs	PropertySpecEventArgs;
	typedef System::Collections::IDictionary					IDictionary;
	typedef System::Collections::ArrayList						ArrayList;
	typedef System::Object										Object;
	typedef System::String										String;

public:
								property_container	(property_holder* holder, property_container_holder^ container_holder);
	virtual						~property_container	();
								!property_container	();
	property_holder&			holder				();
	property_container_holder%	container_holder	();
			void				add_property		(PropertySpec^ description, property_value^ value);
	virtual	property_value^		value				(PropertySpec^ description);
	IDictionary%				properties			();
	ArrayList%					ordered_properties	();
			void				clear				();

private:
			void				get_value_handler	(Object^ sender, PropertySpecEventArgs^ e);
			void				set_value_handler	(Object^ sender, PropertySpecEventArgs^ e);
			bool				equal_category		(String^ new_category, String^ old_category);
			String^				update_categories	(String^ new_category);
			void				try_update_name		(PropertySpec^ description, String^ name);
			void				update_names		(String^ name);

private:
	IDictionary^				m_properties;
	ArrayList^					m_categories;
	ArrayList^					m_ordered_properties;
	property_holder*			m_holder;
	property_container_holder^	m_container_holder;
}; // class property_container

#endif // ifndef PROPERTY_CONTAINER_HPP_INCLUDED