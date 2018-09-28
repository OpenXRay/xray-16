#pragma once

/*
#define DECLARE_PHLIST_ITEM(class_name) public:\
                                                class CPHListItem\
                                                {\
                                                    friend class CPHItemList<class_name>;\
                                                    friend class CPHItemList<class_name>::iterator;\
                                                    class_name* next;\
                                                    class_name** tome;\
                                                };\
                                                private:
*/
#define DECLARE_PHLIST_ITEM(class_name)             \
    friend class CPHItemList<class_name>;           \
    friend class CPHItemList<class_name>::iterator; \
    class_name* next;                               \
    class_name** tome;
#define DECLARE_PHSTACK_ITEM(class_name)   \
    DECLARE_PHLIST_ITEM(class_name)        \
    friend class CPHItemStack<class_name>; \
    u16 stack_pos;

//#define TPI(item) ((T::CPHListItem*)item)

template <class T>
class CPHItemList
{
    T* first_next;
    T** last_tome;

protected:
    u16 size;

public:
    class iterator
    {
        T* my_ptr;

    public:
        iterator() { my_ptr = 0; }
        iterator(T* i) { my_ptr = i; }
        iterator operator++() { return my_ptr = ((my_ptr)->next); }
        T* operator*() { return my_ptr; }
        bool operator!=(iterator right) { return my_ptr != right.my_ptr; }
    };
    CPHItemList() { empty(); }
    u16 count() { return size; }
    void push_back(T* item)
    {
        VERIFY2(size < 65535, "CPHItemList overflow");
        *(last_tome) = item;
        item->tome = last_tome;
        last_tome = &((item)->next);
        item->next = 0;
        size++;
    }
    void move_items(CPHItemList<T>& source_list)
    {
        if (!source_list.first_next)
            return;
        VERIFY2(size + source_list.size < 65535, "CPHItemList overflow");
        *(last_tome) = source_list.first_next;
        source_list.first_next->tome= last_tome;
        last_tome = source_list.last_tome;
        size += source_list.size;
        source_list.empty();
    }
    void erase(iterator i)
    {
        VERIFY2(size != 0, "CPHItemList underflow");
        T* item = *i;
        T* next = item->next;
        *(item->tome) = next;
        if (next)
            next->tome = item->tome;
        else
            last_tome = item->tome;
        size--;
    }
    void empty()
    {
        last_tome = &first_next;
        first_next = 0;
        size = 0;
    }
    iterator begin() { return iterator(first_next); }
    iterator end() { return iterator(0); }
};

template <class T>
class CPHItemStack : public CPHItemList<T>
{
public:
    void push_back(T* item)
    {
        item->stack_pos = this->size;
        CPHItemList<T>::push_back(item);
    }
};
#define DEFINE_PHITEM_LIST(T, N, I)\
    typedef CPHItemList<T> N;\
    typedef CPHItemList<T>::iterator I;
#define DEFINE_PHITEM_STACK(T, N, I)\
    typedef CPHItemStack<T> N;\
    typedef CPHItemStack<T>::iterator I;
