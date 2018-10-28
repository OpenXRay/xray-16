#ifndef __XR_OBJECT_LIST_H__
#define __XR_OBJECT_LIST_H__

#ifdef DEBUG
extern ENGINE_API BOOL debug_destroy;
#endif

class IGameObject;
class NET_Packet;

class ENGINE_API CObjectList
{
public:
    struct ObjectUpdateStatistics
    {
    public:
        CStatTimer Update;
        u32 Updated;
        u32 Crows;
        u32 Active;
        u32 Total;

        IC void FrameStart()
        {
            Update.FrameStart();
            Updated = 0;
            Crows = 0;
            Active = 0;
            Total = 0;
        }

        IC void FrameEnd() { Update.FrameEnd(); }
    };

private:
    IGameObject* map_NETID[0xffff];
    typedef xr_vector<IGameObject*> Objects;
    Objects destroy_queue;
    Objects objects_active;
    Objects objects_sleeping;
    /**
     * @brief m_primary_crows   - list of items of the primary thread
     * @brief m_secondary_crows - list of items of the secondary thread
     */
    Objects m_primary_crows, m_secondary_crows;
    tid_t m_owner_thread_id;
    ObjectUpdateStatistics stats;
    u32 statsFrame;

public:
    typedef fastdelegate::FastDelegate1<IGameObject*> RELCASE_CALLBACK;
    struct SRelcasePair
    {
        int* m_ID;
        RELCASE_CALLBACK m_Callback;
        SRelcasePair(int* id, RELCASE_CALLBACK cb) : m_ID(id), m_Callback(cb) {}
        bool operator==(RELCASE_CALLBACK cb) { return m_Callback == cb; }
    };
    typedef xr_vector<SRelcasePair> RELCASE_CALLBACK_VEC;
    RELCASE_CALLBACK_VEC m_relcase_callbacks;

    void relcase_register(RELCASE_CALLBACK, int*);
    void relcase_unregister(int*);

public:
    const ObjectUpdateStatistics& GetStats()
    {
        stats.FrameEnd();
        return stats;
    }
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
    // methods
    CObjectList();
    ~CObjectList();

    IGameObject* FindObjectByName(shared_str name);
    IGameObject* FindObjectByName(LPCSTR name);
    IGameObject* FindObjectByCLS_ID(CLASS_ID cls);

    void Load();
    void Unload();

    IGameObject* Create(LPCSTR name);
    void Destroy(IGameObject* O);

private:
    void SingleUpdate(IGameObject* O);

public:
    void Update(bool bForce);

    void net_Register(IGameObject* O);
    void net_Unregister(IGameObject* O);

    u32 net_Export(NET_Packet* P, u32 _start, u32 _count); // return next start
    void net_Import(NET_Packet* P);

    ICF IGameObject* net_Find(u16 ID) const
    {
        if (ID == u16(-1))
            return (0);

        return (map_NETID[ID]);
    }

    void o_crow(IGameObject* O);
    void o_remove(Objects& v, IGameObject* O);
    void o_activate(IGameObject* O);
    void o_sleep(IGameObject* O);
    IC u32 o_count() { return objects_active.size() + objects_sleeping.size(); };
    IC IGameObject* o_get_by_iterator(u32 _it)
    {
        if (_it < objects_active.size())
            return objects_active[_it];
        else
            return objects_sleeping[_it - objects_active.size()];
    }
    bool dump_all_objects();

public:
    void register_object_to_destroy(IGameObject* object_to_destroy);
#ifdef DEBUG
    bool registered_object_to_destroy(const IGameObject* object_to_destroy) const;
#endif // #ifdef DEBUG

private:
    IC Objects& get_crows()
    {
        if (GetCurrentThreadId() == m_owner_thread_id)
            return (m_primary_crows);

        return (m_secondary_crows);
    }

    static void clear_crow_vec(Objects& o);
    static void dump_list(Objects& v, LPCSTR reason);
};

#endif //__XR_OBJECT_LIST_H__
