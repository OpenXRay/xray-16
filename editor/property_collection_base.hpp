////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_base.hpp
//	Created 	: 08.01.2008
//  Modified 	: 08.01.2008
//	Author		: Dmitriy Iassenev
//	Description : property collection base class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_BASE_HPP_INCLUDED
#define PROPERTY_COLLECTION_BASE_HPP_INCLUDED

#include "property_holder_include.hpp"

ref class property_container;
ref class property_collection_editor;
ref class property_collection_converter;

[System::ComponentModel::EditorAttribute(property_collection_editor::typeid,System::Drawing::Design::UITypeEditor::typeid)]
[System::ComponentModel::TypeConverter(property_collection_converter::typeid)]
public ref class property_collection_base abstract :
	public property_value,
	public System::Collections::IList
{
public:
	typedef editor::property_holder_collection			collection_type;
	typedef editor::property_holder						property_holder;
	typedef System::Collections::IEnumerator			IEnumerator;
	typedef System::Array								Array;
	typedef System::Object								Object;

public:
							property_collection_base	();
	virtual					~property_collection_base	();
							!property_collection_base	();
	virtual Object			^get_value					();
	virtual void			set_value					(Object ^object);

public:
	virtual	void			CopyTo						(Array^ items, int index);
	virtual	IEnumerator^	GetEnumerator				();

public:
	property bool IsSynchronized {
		virtual	bool		get							();
	}
	property Object^ SyncRoot {
		virtual	Object^		get							();
	}
	property int Count {
		virtual	int			get							();
	}

public:
	virtual	int				Add							(Object^ value);
	virtual	void			Clear						();
	virtual	bool			Contains					(Object^ value);
	virtual	int				IndexOf						(Object^ value);
	virtual	void			Insert						(int index, Object^ value);
	virtual	void			Remove						(Object^ value);
	virtual	void			RemoveAt					(int index);

public:
	property bool IsFixedSize {
		virtual	bool		get							();
	}
	property bool IsReadOnly {
		virtual	bool		get							();
	}
	property Object^ default[int] {
		virtual	Object^		get							(int index);
		virtual	void		set							(int index, Object^ value);
	}

public:
				property_container^	create				();

protected:
	virtual	collection_type*collection					() = 0;
}; // ref class property_collection_base

#endif // ifndef PROPERTY_COLLECTION_BASE_HPP_INCLUDED