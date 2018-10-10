////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_registry_container.cpp
//	Created 	: 01.07.2004
//  Modified 	: 01.07.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife registry container class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_registry_container.h"
#include "Common/object_interfaces.h"
#include "alife_space.h"
#include "Common/object_type_traits.h"

template <typename TContainer, typename TList>
struct RegistryHelper;

template <typename TContainer>
struct RegistryHelper<TContainer, Loki::NullType>
{
    static void Save(TContainer*, IWriter&) {};
    static void Load(TContainer*, IReader&) {};
};

template <typename TContainer, typename Head, typename Tail>
struct RegistryHelper<TContainer, Loki::Typelist<Head, Tail>>
{
    static constexpr bool isSerializable =
        object_type_traits::is_base_and_derived<ISerializable, Head>::value;

    static void Save(TContainer* self, IWriter& writer)
    {
        if constexpr (isSerializable)
            self->Head::save(writer);
        RegistryHelper<TContainer, Tail>::Save(self, writer);
    };

    static void Load(TContainer* self, IReader& reader)
    {
        if constexpr (isSerializable)
            self->Head::load(reader);
        RegistryHelper<TContainer, Tail>::Load(self, reader);
    }
};

void CALifeRegistryContainer::load(IReader& file_stream)
{
    R_ASSERT2(file_stream.find_chunk(REGISTRY_CHUNK_DATA), "Can't find chunk REGISTRY_CHUNK_DATA!");
    RegistryHelper<CALifeRegistryContainer, TYPE_LIST>::Load(this, file_stream);
}

void CALifeRegistryContainer::save(IWriter& memory_stream)
{
    memory_stream.open_chunk(REGISTRY_CHUNK_DATA);
    RegistryHelper<CALifeRegistryContainer, TYPE_LIST>::Save(this, memory_stream);
    memory_stream.close_chunk();
}
