#ifndef DEMO_MESSAGE_FILER
#define DEMO_MESSAGE_FILER

#include "xrCore/fastdelegate.h"
#include "xrCore/net_utils.h"
#include "xrCore/Containers/AssociativeVector.hpp"

#define FILTERS_COUNT 32

class message_filter
{
public:
    typedef fastdelegate::FastDelegate3<u32, u32, NET_Packet&> msg_type_subtype_func_t;
    message_filter();
    ~message_filter();
    void filter(u16 const& msg_type, u32 const& msg_subtype, msg_type_subtype_func_t const& found_func);
    void remove_filter(u16 const& msg_type, u32 const& msg_subtype);

    void check_new_data(NET_Packet& packet);

    void dbg_set_message_log_file(string_path const& message_log_file);

private:
    struct msg_type_subtype_t
    {
        msg_type_subtype_t(u16 const type = 0, u32 const subtype = 0, u32 const recv_time = 0)
            : msg_type(type), dest_obj_id(0), msg_subtype(subtype), msg_receive_time(recv_time) {}

        u16 msg_type;
        u16 dest_obj_id;
        u32 msg_subtype;
        u32 msg_receive_time;
        inline bool operator<(msg_type_subtype_t const& right) const
        {
            if (msg_type < right.msg_type)
                return true;
            else if (msg_type > right.msg_type)
                return false;
            return msg_subtype < right.msg_subtype;
        }
        void import(NET_Packet& packet);
    };
    void dbg_print_msg(NET_Packet& packet, msg_type_subtype_t const& msg_type);
    typedef AssociativeVector<msg_type_subtype_t, msg_type_subtype_func_t> filters_map_t;

    filters_map_t m_filters;
    IWriter* m_msg_log_file;
    string256 m_last_string;
    u32 m_strrepeat_count;
}; // class message_filter

#endif //#ifndef DEMO_MESSAGE_FILER
