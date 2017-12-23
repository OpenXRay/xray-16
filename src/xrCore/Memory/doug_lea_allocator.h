////////////////////////////////////////////////////////////////////////////
// Created : 14.08.2009
// Author : Armen Abroyan
// Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////
#pragma once

class XRCORE_API doug_lea_allocator
{
public:
    doug_lea_allocator(void* arena, u32 arena_size, pcstr arena_id);
    ~doug_lea_allocator();
    void* malloc_impl(u32 size);
    void* realloc_impl(void* pointer, u32 new_size);
    void free_impl(void*& pointer);
    u32 get_allocated_size() const;
    pcstr get_arena_id() const { return m_arena_id; }
    template <typename T>
    void free_impl(T*& pointer)
    {
        free_impl(reinterpret_cast<void*&>(pointer));
    }

    template <typename T>
    T* alloc_impl(u32 const count)
    {
        return (T*)malloc_impl(count * sizeof(T));
    }

private:
    pcstr m_arena_id;
    void* m_dl_arena;
};
