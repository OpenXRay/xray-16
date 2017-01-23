#pragma once
#include <assert.h>
#include <windows.h>


template <class _Type>
class nvVector
{
    _Type *m_data;

    size_t allocated_size;
    size_t current_size;

public:


    void tvfree(_Type * & ptr)
    {

        if (ptr)
        {

            delete [] ptr;
            ptr = 0;
        }
    }

    _Type * tvallocate(size_t elements)
    {

        return new _Type[elements];

   


    }
    nvVector & operator = ( const nvVector& v )
    {

        resize(v.size());

        for(size_t i = 0; i < v.size(); i++)
        {
            m_data[i] = v.m_data[i];
        }
        return *this; 
    }


    void FirstAllocation()
    {
        // start with 256 entries

        tvfree (m_data);

        allocated_size = 256;
        current_size = 0;
        m_data = tvallocate(allocated_size);
    }

    nvVector()
    { 
        m_data = 0;
        current_size = 0;
        allocated_size = 0;

    }


    void resize(size_t newSize)
    {

        if (newSize != allocated_size)
        {
            allocated_size = newSize;

            tvfree (m_data);

            m_data = tvallocate(allocated_size);
        }

        current_size = newSize;

    }
    void push_back(_Type item)
    {
        if (allocated_size == 0)
        {
            FirstAllocation();
        }
        else if (current_size >= allocated_size)
        {
            allocated_size = allocated_size * 2;

            _Type *temp = tvallocate(allocated_size);

            // copy old data to new area
            for(size_t i = 0; i< current_size; i++)
                temp[i] = m_data[i];

            tvfree (m_data);

            m_data = temp;
        }

        
        m_data[current_size] = item;
        current_size++;
        
        

    }

    // indexing

    _Type& operator [] ( size_t i) 
    {
#ifdef _DEBUG
        assert(i < current_size);
        assert(current_size <= allocated_size);
#endif
        return m_data[i]; 
    };  
    
    const _Type& operator[](size_t i) const 
    { 
#ifdef _DEBUG
        assert(i < current_size);
        assert(current_size <= allocated_size);
#endif
        return m_data[i];
    }

    void Release()
    {
        tvfree (m_data);

        current_size = 0;
        allocated_size = 0;

    }
    void clear()
    {
        Release();


    }

    size_t size() const
    {
        return current_size;
    }


    ~nvVector()
    {
        Release();
    }

};

