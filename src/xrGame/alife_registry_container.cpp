////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container.cpp
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_registry_container.h"
#include "Common/object_interfaces.h"
#include "alife_space.h"
#include "Common/object_type_traits.h"

template <typename TContainer, typename TList>
class RegistryHelper
{
private:
	ASSERT_TYPELIST(TList);
    using Head = typename TList::Head;
    using Tail = typename TList::Tail;

    static const bool isSerializable = object_type_traits::is_base_and_derived<ISerializable, Head>::value;

    template<bool serializable>
	static void LoadHead(TContainer *self, IReader &reader) {}

	template<>
	static void LoadHead<true>(TContainer *self, IReader &reader) { self->Head::load(reader); }

    template<bool serializable>
    static void SaveHead(TContainer *self, IWriter &writer) {}

    template<>
    static void SaveHead<true>(TContainer *self, IWriter &writer) { self->Head::save(writer); }

	static void LoadHead(TContainer *self, IReader &reader) { LoadHead<isSerializable>(self, reader); }

    static void SaveHead(TContainer *self, IWriter &writer) { SaveHead<isSerializable>(self, writer); }
    
	template<typename TListTail>
	static void Load(TContainer *self, IReader &reader)
	{ RegistryHelper<TContainer, Tail>::Load(self, reader); }

	template<>
	static void Load<Loki::NullType>(TContainer *self, IReader &reader) {}

    template<typename TListTail>
    static void Save(TContainer *self, IWriter &writer)
    { RegistryHelper<TContainer, Tail>::Save(self, writer); }

    template<>
    static void Save<Loki::NullType>(TContainer *self, IWriter &writer) {}

public:
	static void Load(TContainer *self, IReader &reader)
	{
        Load<Tail>(self, reader);
        LoadHead(self, reader);
	}

    static void Save(TContainer *self, IWriter &writer)
    {
        Save<Tail>(self, writer);
        SaveHead(self, writer);
    }
};

void CALifeRegistryContainer::load(IReader &file_stream)
{
	R_ASSERT2					(file_stream.find_chunk(REGISTRY_CHUNK_DATA),"Can't find chunk REGISTRY_CHUNK_DATA!");
    RegistryHelper<CALifeRegistryContainer, TYPE_LIST>::Load(this, file_stream);
}

void CALifeRegistryContainer::save(IWriter &memory_stream)
{
	memory_stream.open_chunk	(REGISTRY_CHUNK_DATA);
    RegistryHelper<CALifeRegistryContainer, TYPE_LIST>::Save(this, memory_stream);
	memory_stream.close_chunk	();
}
