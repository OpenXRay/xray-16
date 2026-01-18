#include "stdafx.h"
#pragma hdrstop // Huh?

#include "xrstring_manager.h"
#include "xrstring_impl.h"
#include "Threading/Lock.hpp"
#include "xrCore/_std_extensions.h"

#include "FS_impl.h"
#include <SDL.h>

bool str_value_cmp::operator()(const str_value* A, const str_value* B) const
{
    return A->dwCRC < B->dwCRC;
}

XRCORE_API str_container* g_pStringContainer = nullptr;

#if 1

struct str_container_impl
{
    Lock cs;
    static constexpr size_t buffer_size = 1024u * 256u;

    xr_vector<str_value*> string_array;
    xr_vector<u32> free_next;
    u32 free_list_head;

    u32 hash_table[buffer_size];
    int num_docs;

    str_container_impl()
        : free_list_head(0)
    {
        ZeroMemory(hash_table, sizeof(hash_table));
        string_array.push_back(nullptr);
        free_next.push_back(0);
    }

    str_value* get_string(u32 index) const
    {
        if (index == 0 || index >= string_array.size())
            return nullptr;
        return string_array[index];
    }

    u32 allocate_index(str_value* value)
    {
        if (free_list_head != 0)
        {
            const u32 index = free_list_head;
            free_list_head = free_next[index];
            free_next[index] = 0;
            string_array[index] = value;
            return index;
        }

        const u32 index = (u32)string_array.size();
        string_array.push_back(value);
        free_next.push_back(0);
        return index;
    }

    void free_index(u32 index)
    {
        if (index == 0 || index >= string_array.size())
            return;

        string_array[index] = nullptr;
        free_next[index] = free_list_head;
        free_list_head = index;
    }

    u32 get_next_index(str_value* value) const
    {
        if (!value)
            return 0;
        return value->next_index;
    }

    u32 get_next_index(const str_value* value) const
    {
        if (!value)
            return 0;
        return value->next_index;
    }

    u32 find(u32 crc, size_t len, const char* str) const
    {
        u32 current_index = hash_table[crc % buffer_size];
        while (current_index != 0)
        {
            str_value* candidate = string_array[current_index];
            if (candidate && candidate->dwCRC == crc && candidate->dwLength == len && !memcmp(candidate->value, str, len))
            {
                return current_index;
            }
            current_index = candidate ? get_next_index(candidate) : 0;
        }
        return 0;
    }

    void insert(str_value* value, u32 index)
    {
        u32 hash_index = value->dwCRC % buffer_size;
        value->next_index = hash_table[hash_index];
        hash_table[hash_index] = index;
    }

    void clean()
    {
        for (size_t i = 0; i < buffer_size; ++i)
        {
            u32 current_index = hash_table[i];
            u32 prev_index = 0;

            while (current_index != 0)
            {
                str_value* value = string_array[current_index];
                if (!value || !value->dwReference)
                {
                    if (prev_index == 0)
                    {
                        hash_table[i] = value ? get_next_index(value) : 0;
                    }
                    else
                    {
                        str_value* prev = string_array[prev_index];
                        prev->next_index = value ? get_next_index(value) : 0;
                    }
                    if (value)
                    {
                        u32 next_index = get_next_index(value);
                        xr_free(value);
                        free_index(current_index);
                        current_index = next_index;
                    }
                    else
                    {
                        current_index = 0;
                    }
                }
                else
                {
                    prev_index = current_index;
                    current_index = value ? get_next_index(value) : 0;
                }
            }
        }
    }

    void verify() const
    {
        Msg("strings verify started");
        for (size_t i = 0; i < buffer_size; ++i)
        {
            u32 current_index = hash_table[i];
            while (current_index != 0)
            {
                const str_value* value = string_array[current_index];
                if (value)
                {
                    const auto crc = crc32(value->value, value->dwLength);
                    string32 crc_str;
                    R_ASSERT3(crc == value->dwCRC, "CorePanic: read-only memory corruption (shared_strings)", xr_itoa(value->dwCRC, crc_str, 16));
                    R_ASSERT3(value->dwLength == xr_strlen(value->value), "CorePanic: read-only memory corruption (shared_strings, internal structures)",
                        value->value);
                }
                current_index = get_next_index(value);
            }
        }
        Msg("strings verify completed");
    }

    void dump(FILE* f) const
    {
        for (size_t i = 0; i < buffer_size; ++i)
        {
            u32 current_index = hash_table[i];
            while (current_index != 0)
            {
                str_value* value = string_array[current_index];
                if (value)
                {
                    fprintf(f, "ref[%4u]-len[%3u]-crc[%8X] : %s\n", value->dwReference, value->dwLength, value->dwCRC, value->value);
                }
                current_index = get_next_index(value);
            }
        }
    }

    void dump(IWriter* f) const
    {
        for (size_t i = 0; i < buffer_size; ++i)
        {
            u32 current_index = hash_table[i];
            string4096 temp;
            while (current_index != 0)
            {
                str_value* value = string_array[current_index];
                if (value)
                {
                    xr_sprintf(temp, sizeof(temp), "ref[%4u]-len[%3u]-crc[%8X] : %s\n", value->dwReference, value->dwLength, value->dwCRC, value->value);
                    f->w_string(temp);
                }
                current_index = get_next_index(value);
            }
        }
    }

    std::pair<size_t, size_t> stat_economy() const
    {
        size_t bytes{}, count{};
        for (size_t i = 0; i < buffer_size; ++i)
        {
            u32 current_index = hash_table[i];
            while (current_index != 0)
            {
                const str_value* value = string_array[current_index];
                if (value)
                {
                    ++count;
                    bytes += (value->dwReference - 1) * (value->dwLength + 1);
                }
                current_index = get_next_index(value);
            }
        }
        return { bytes, count };
    }
};

str_container::str_container() :
    impl(xr_new<str_container_impl>())
#ifdef CONFIG_PROFILE_LOCKS
    , cs(MUTEX_PROFILE_ID(str_container))
#endif
{}

u32 str_container::dock(pcstr value) const
{
    if (nullptr == value)
        return 0;

    impl->cs.Enter();

    const auto s_len = xr_strlen(value);
    const auto s_len_with_zero = s_len + 1;
    VERIFY(sizeof(str_value) + s_len_with_zero < 4096);

    const u32 crc = crc32(value, s_len);

    u32 index = impl->find(crc, s_len, value);

#    ifdef DEBUG
    const bool is_leaked_string = !xr_strcmp(value, "enter leaked string here");
#    endif // DEBUG

    if (index != 0
#    ifdef DEBUG
        && !is_leaked_string
#    endif // DEBUG
    )
    {
        impl->cs.Leave();
        return index;
    }

    str_value* new_str = static_cast<str_value*>(xr_malloc(sizeof(str_value) + s_len_with_zero));

#ifdef DEBUG
    static int num_leaked_string = 0;
    if (is_leaked_string)
    {
        ++num_leaked_string;
        Msg("leaked_string: %d ptr=%p", num_leaked_string, new_str);
    }
#endif // DEBUG

    new_str->dwReference = 0;
    new_str->dwLength = static_cast<u32>(s_len);
    new_str->dwCRC = crc;
    new_str->next_index = 0;
    CopyMemory(new_str->value, value, s_len_with_zero);

    const u32 new_index = impl->allocate_index(new_str);
    impl->insert(new_str, new_index);

#ifdef DEBUG
    if (is_leaked_string)
        Msg("leaked_string: idx=%u ptr=%p", new_index, new_str);
#endif // DEBUG

    impl->cs.Leave();
    return new_index;
}

str_value* str_container::get_string(u32 index) const
{
    return impl->get_string(index);
}

void str_container::clean() const
{
    impl->cs.Enter();
    impl->clean();
    impl->cs.Leave();
}

void str_container::verify() const
{
    impl->cs.Enter();
    impl->verify();
    impl->cs.Leave();
}

void str_container::dump() const
{
    impl->cs.Enter();
    FILE* F = fopen("d:\\$str_dump$.txt", "w");
    impl->dump(F);
    fclose(F);
    impl->cs.Leave();
}

void str_container::dump(IWriter* W) const
{
    impl->cs.Enter();
    impl->dump(W);
    impl->cs.Leave();
}

std::pair<size_t, size_t> str_container::stat_economy() const
{
    impl->cs.Enter();
    const auto [bytes, count] = impl->stat_economy();
    impl->cs.Leave();
    return { bytes, count };
}

str_container::~str_container()
{
    clean();
    // dump ();
    xr_delete(impl);
}

#else // 0/1

struct str_container_impl
{
    typedef xr_multiset<str_value*, str_value_cmp> cdb;
    int num_docs;
    str_container_impl() { num_docs = 0; }
    cdb container;
};

str_container::str_container() :
    impl(xr_new<str_container_impl>())
#ifdef CONFIG_PROFILE_LOCKS
    , cs(MUTEX_PROFILE_ID(str_container))
#endif
{}

str_value* str_container::dock(str_c value)
{
    if (0 == value)
        return 0;

    impl->cs.Enter();

// ++impl->num_docs;
// if ( impl->num_docs == 10000000 )
// {
// Msg("shared_strings");
// g_find_chunk_counter.flush();
// }
//
// //#ifdef FIND_CHUNK_BENCHMARK_ENABLE
// find_chunk_auto_timer timer;
// //#endif // FIND_CHUNK_BENCHMARK_ENABLE

    str_value* result = 0;

    // calc len
    u32 s_len = xr_strlen(value);
    u32 s_len_with_zero = (u32)s_len + 1;
    VERIFY(sizeof(str_value) + s_len_with_zero < 4096);

    // setup find structure
    char header[sizeof(str_value)];
    str_value* sv = (str_value*)header;
    sv->dwReference = 0;
    sv->dwLength = s_len;
    sv->dwCRC = crc32(value, s_len);
    sv->next = NULL;

    // search
    str_container_impl::cdb::iterator I = impl->container.find(sv); // only integer compares :)
    if (I != impl->container.end())
    {
        // something found - verify, it is exactly our string
        str_container_impl::cdb::iterator save = I;
        for (; I != impl->container.end() && (*I)->dwCRC == sv->dwCRC; ++I)
        {
            str_value* V = (*I);
            if (V->dwLength != sv->dwLength)
                continue;
            if (0 != memcmp(V->value, value, s_len))
                continue;
            result = V; // found
            break;
        }
    }

    bool is_leaked_string = !xr_strcmp(value, "enter leaked string here");

    // it may be the case, string is not found or has "non-exact" match
    if (0 == result || is_leaked_string)
    {
        // Insert string

        result = (str_value*)xr_malloc(sizeof(str_value) + s_len_with_zero
#ifdef DEBUG_MEMORY_NAME
            ,
            "storage: sstring"
#endif // DEBUG_MEMORY_NAME
            );

        static int num11 = 0;

        if (is_leaked_string)
        {
            ++num11;
            Msg("leaked_string: %d 0x%08x", num11, result);
        }

        result->dwReference = 0;
        result->dwLength = sv->dwLength;
        result->dwCRC = sv->dwCRC;
        result->next = NULL;

        CopyMemory(result->value, value, s_len_with_zero);

        impl->container.insert(result);
    }

    impl->cs.Leave();

    return result;
}

void str_container::clean()
{
    impl->cs.Enter();
    str_container_impl::cdb::iterator it = impl->container.begin();
    str_container_impl::cdb::iterator end = impl->container.end();
    for (; it != end;)
    {
        str_value* sv = *it;
        if (0 == sv->dwReference)
        {
            str_container_impl::cdb::iterator i_current = it;
            str_container_impl::cdb::iterator i_next = ++it;
            xr_free(sv);
            impl->container.erase(i_current);
            it = i_next;
        }
        else
        {
            it++;
        }
    }
    if (impl->container.empty())
        impl->container.clear();
    impl->cs.Leave();
}

void str_container::verify()
{
    impl->cs.Enter();
    str_container_impl::cdb::iterator it = impl->container.begin();
    str_container_impl::cdb::iterator end = impl->container.end();
    for (; it != end; ++it)
    {
        str_value* sv = *it;
        u32 crc = crc32(sv->value, sv->dwLength);
        string32 crc_str;
        R_ASSERT3(crc == sv->dwCRC,
            "CorePanic: read-only memory corruption (shared_strings)", xr_itoa(sv->dwCRC, crc_str, 16));
        R_ASSERT3(sv->dwLength == xr_strlen(sv->value),
            "CorePanic: read-only memory corruption (shared_strings, internal structures)", sv->value);
    }
    impl->cs.Leave();
}

void str_container::dump()
{
    impl->cs.Enter();
    str_container_impl::cdb::iterator it = impl->container.begin();
    str_container_impl::cdb::iterator end = impl->container.end();
    FILE* F = fopen("d:\\$str_dump$.txt", "w");
    for (; it != end; it++)
        fprintf(
            F, "ref[%4d]-len[%3d]-crc[%8X] : %s\n", (*it)->dwReference, (*it)->dwLength, (*it)->dwCRC, (*it)->value);
    fclose(F);
    impl->cs.Leave();
}

u32 str_container::stat_economy()
{
    impl->cs.Enter();
    str_container_impl::cdb::iterator it = impl->container.begin();
    str_container_impl::cdb::iterator end = impl->container.end();
    int counter = 0;
    counter -= sizeof(*this);
    counter -= sizeof(str_container_impl::cdb::allocator_type);
    const int node_size = 20;
    for (; it != end; it++)
    {
        counter -= sizeof(str_value);
        counter -= node_size;
        counter += int((int((*it)->dwReference) - 1) * int((*it)->dwLength + 1));
    }
    impl->cs.Leave();

    return u32(counter);
}

str_container::~str_container()
{
    clean();
    // dump ();
    xr_delete(impl);
    // R_ASSERT(impl->container.empty());
}

#endif // 0/1
