#pragma once

#include <typeindex>
#include <memory>

namespace xray::render::framegraph {

class Blackboard
{
    struct Entry
    {
        void* ptr = nullptr;
        void (*deleter)(void*) = nullptr;

        Entry() = default;

        template <typename T>
        explicit Entry(T* p)
            : ptr(p)
            , deleter([](void* v) { delete static_cast<T*>(v); })
        {}

        ~Entry() { if (deleter && ptr) deleter(ptr); }

        Entry(Entry&& o) noexcept : ptr(o.ptr), deleter(o.deleter) { o.ptr = nullptr; o.deleter = nullptr; }
        Entry& operator=(Entry&& o) noexcept
        {
            if (this != &o)
            {
                if (deleter && ptr) deleter(ptr);
                ptr = o.ptr; deleter = o.deleter;
                o.ptr = nullptr; o.deleter = nullptr;
            }
            return *this;
        }

        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
    };

    xr_map<std::type_index, Entry> m_storage;

public:
    template <typename T, typename... Args>
    T& add(Args&&... args)
    {
        auto key = std::type_index(typeid(T));
        VERIFY2(m_storage.find(key) == m_storage.end(), "Blackboard: type already registered");
        auto* p = new T(std::forward<Args>(args)...);
        m_storage.emplace(key, Entry(p));
        return *p;
    }

    template <typename T>
    [[nodiscard]] T& get()
    {
        auto it = m_storage.find(std::type_index(typeid(T)));
        VERIFY2(it != m_storage.end(), "Blackboard: type not found");
        return *static_cast<T*>(it->second.ptr);
    }

    template <typename T>
    [[nodiscard]] const T& get() const
    {
        auto it = m_storage.find(std::type_index(typeid(T)));
        VERIFY2(it != m_storage.end(), "Blackboard: type not found");
        return *static_cast<const T*>(it->second.ptr);
    }

    template <typename T>
    [[nodiscard]] T* try_get()
    {
        auto it = m_storage.find(std::type_index(typeid(T)));
        if (it == m_storage.end()) return nullptr;
        return static_cast<T*>(it->second.ptr);
    }

    template <typename T>
    [[nodiscard]] const T* try_get() const
    {
        auto it = m_storage.find(std::type_index(typeid(T)));
        if (it == m_storage.end()) return nullptr;
        return static_cast<const T*>(it->second.ptr);
    }

    template <typename T>
    [[nodiscard]] bool has() const
    {
        return m_storage.find(std::type_index(typeid(T))) != m_storage.end();
    }

    template <typename T, typename... Args>
    T& get_or_add(Args&&... args)
    {
        auto key = std::type_index(typeid(T));
        auto it = m_storage.find(key);
        if (it != m_storage.end())
            return *static_cast<T*>(it->second.ptr);
        auto* p = new T(std::forward<Args>(args)...);
        m_storage.emplace(key, Entry(p));
        return *p;
    }

    void clear() { m_storage.clear(); }
};

}
