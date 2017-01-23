
template <class T>
IC void xr_delete(T*& ptr)
{
    if (ptr)
    {
        delete ptr;
        const_cast<T*&>(ptr) = NULL;
    }
}
