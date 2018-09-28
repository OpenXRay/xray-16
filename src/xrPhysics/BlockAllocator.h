#pragma once

template <class T, u32 block_size>
class CBlockAllocator
{
    u32 block_count;
    u32 block_position;
    T* current_block;
    xr_vector<T*> blocks;

public:
    IC T* add()
    {
        if (block_position == block_size)
            next_block();
        ++block_position;
        return &current_block[block_position - 1];
    }
    IC void empty()
    {
        block_count = 0;
        if (blocks.size())
        {
            block_position = 0;
            current_block = blocks[0];
        }
        else
        {
            block_position = block_size;
        }
    }
    CBlockAllocator() { init(); }
    ~CBlockAllocator() { clear(); }
    IC void init()
    {
        block_position = block_size;
        block_count = 0;
        current_block = NULL;
    }
    IC void clear()
    {
        for (auto& it : blocks)
            xr_free(it);
        blocks.clear();
        init();
    }

private:
    /////////////////////////////////////////////////////////////////
    IC void add_block() { blocks.push_back(xr_alloc<T>(block_size)); };
    IC void next_block()
    {
        if (block_count == blocks.size())
            add_block();
        current_block = blocks[block_count];
        ++block_count;
        block_position = 0;
    }
    ////////////////////////////////////////////////////////////////
public:
    template <typename _Predicate>
    IC void for_each(const _Predicate& pred)
    {
        if (!current_block)
            return;
        auto i = blocks.begin();
        auto e = blocks.begin() + block_count;
        u32 j;
        for (; i != e; ++i)
        {
            for (j = 0; j < block_size; ++j)
                pred.operator()((*i) + j);
        }
        for (j = 0; j < block_position; ++j)
        {
            pred.operator()(current_block + j);
        }
        // for_each(blocks.begin(),block.end(),pred);
    }

private:
    IC T* pointer(u32 position) { return blocks[position / block_size] + position % block_size; }
    IC T& back() { return current_block[block_position - 1]; }
    IC T& back_pointer() { return current_block + block_position - 1; }
    IC T& operator[](u32 position) { return *pointer(position); }
    // What is xr_allocator_t? This two methods are unused
    //IC void construct(u32 position) { xr_allocator_t<T>().construct(pointer(position)); }
    //IC void construct_back() { xr_allocator_t<T>().construct(back_pointer()); }
};
