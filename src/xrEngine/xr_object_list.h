#ifndef __XR_OBJECT_LIST_H__
#define __XR_OBJECT_LIST_H__

// refs
class ENGINE_API CObject;
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

        IC void FrameEnd()
        { Update.FrameEnd(); }
    };
private:
    CObject* map_NETID[0xffff];
    typedef xr_vector<CObject*> Objects;
    Objects destroy_queue;
    Objects objects_active;
    Objects objects_sleeping;
    Objects m_crows[2];
    u32 m_owner_thread_id;
    ObjectUpdateStatistics stats;
    u32 statsFrame;

public:
    typedef fastdelegate::FastDelegate1<CObject*> RELCASE_CALLBACK;
    struct SRelcasePair
    {
        int* m_ID;
        RELCASE_CALLBACK m_Callback;
        SRelcasePair(int* id, RELCASE_CALLBACK cb) :m_ID(id), m_Callback(cb) {}
        bool operator== (RELCASE_CALLBACK cb) { return m_Callback == cb; }
    };
    typedef xr_vector<SRelcasePair> RELCASE_CALLBACK_VEC;
    RELCASE_CALLBACK_VEC m_relcase_callbacks;

    void relcase_register(RELCASE_CALLBACK, int*);
    void relcase_unregister(int*);

public:
    const ObjectUpdateStatistics &GetStats()
    {
        stats.FrameEnd();
        return stats;
    }
    void DumpStatistics(class CGameFont &font, class PerformanceAlert *alert);
    // methods
    CObjectList();
    ~CObjectList();

    CObject* FindObjectByName(shared_str name);
    CObject* FindObjectByName(LPCSTR name);
    CObject* FindObjectByCLS_ID(CLASS_ID cls);

    void Load();
    void Unload();

    CObject* Create(LPCSTR name);
    void Destroy(CObject* O);
private:
    void SingleUpdate(CObject* O);
public:
    void Update(bool bForce);

    void net_Register(CObject* O);
    void net_Unregister(CObject* O);

    u32 net_Export(NET_Packet* P, u32 _start, u32 _count); // return next start
    void net_Import(NET_Packet* P);

    ICF CObject* net_Find(u16 ID) const
    {
        if (ID == u16(-1))
            return (0);

        return (map_NETID[ID]);
    }

    void o_crow(CObject* O);
    void o_remove(Objects& v, CObject* O);
    void o_activate(CObject* O);
    void o_sleep(CObject* O);
    IC u32 o_count() { return objects_active.size() + objects_sleeping.size(); };
    IC CObject* o_get_by_iterator(u32 _it)
    {
        if (_it < objects_active.size()) return objects_active[_it];
        else return objects_sleeping[_it - objects_active.size()];
    }
    bool dump_all_objects();

public:
    void register_object_to_destroy(CObject* object_to_destroy);
#ifdef DEBUG
    bool registered_object_to_destroy(const CObject* object_to_destroy) const;
#endif // #ifdef DEBUG

private:
    IC Objects& get_crows()
    {
        if (GetCurrentThreadId() == m_owner_thread_id)
            return (m_crows[0]);

        return (m_crows[1]);
    }

    static void clear_crow_vec(Objects& o);
    static void dump_list(Objects& v, LPCSTR reason);
};

#endif //__XR_OBJECT_LIST_H__
