////////////////////////////////////////////////////////////////////////////
//	Module 		: property_collection_editor.hpp
//	Created 	: 24.12.2007
//  Modified 	: 25.12.2007
//	Author		: Dmitriy Iassenev
//	Description : collection property editor implementation class
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_EDITOR_HPP_INCLUDED
#define PROPERTY_COLLECTION_EDITOR_HPP_INCLUDED

ref class property_collection;
ref class property_container;

public ref class property_collection_editor : public System::ComponentModel::Design::CollectionEditor {
public:
	typedef System::ComponentModel::Design::CollectionEditor	inherited;
	typedef System::ComponentModel::ITypeDescriptorContext		ITypeDescriptorContext;
	typedef System::IServiceProvider							IServiceProvider;
	typedef System::EventArgs									EventArgs;
	typedef System::Object										Object;
	typedef System::String										String;
	typedef System::Type										Type;

public:
							property_collection_editor	(Type^ type);
	virtual	Object^			EditValue					(
								ITypeDescriptorContext^ context,
								IServiceProvider^ provider,
								Object ^value
							) override;

protected:
	virtual Type^			CreateCollectionItemType	() override;
	virtual	Object^			CreateInstance				(Type^ type) override;
	virtual String^			GetDisplayText				(Object^ value) override;
	virtual CollectionForm^	CreateCollectionForm		() override;

private:
			void			on_move						(Object^ sender, EventArgs^ e);

private:
	CollectionForm^			m_collection_form;
}; // ref class property_collection_editor

#endif // ifndef PROPERTY_COLLECTION_EDITOR_HPP_INCLUDED