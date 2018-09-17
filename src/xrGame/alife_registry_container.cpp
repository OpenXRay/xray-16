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

template <typename, typename>
struct RegistryHelper;

template <typename TContainer>
struct RegistryHelper<TContainer, Loki::Typelist<>>
{
    static void Save(TContainer*, IWriter&) {};
    static void Load(TContainer*, IReader&) {};
};

template <typename TContainer, typename T, typename... Ts>
struct RegistryHelper<TContainer, Loki::Typelist<T, Ts...>>
{
    static constexpr bool isSerializable = object_type_traits::is_base_and_derived<ISerializable, T>::value;

    static void Save(TContainer* self, IWriter& writer)
    {
        if constexpr (isSerializable)
            self->T::save(writer);
        RegistryHelper<TContainer, Loki::Typelist<Ts...>>::Save(self, writer);
    };

    static void Load(TContainer* self, IReader& reader)
    {
        if constexpr (isSerializable)
            self->T::load(reader);
        RegistryHelper<TContainer, Loki::Typelist<Ts...>>::Load(self, reader);
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
