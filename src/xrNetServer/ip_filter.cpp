#include "stdafx.h"
#include "ip_filter.h"
#include "xrCore/xr_ini.h"

ip_filter::ip_filter() {}

ip_filter::~ip_filter()
{
    for (subnets_coll_t::iterator i = m_all_subnets.begin(), ie = m_all_subnets.end(); i != ie; ++i)
    {
        xr_delete(*i);
    }
}

struct subnet_comparator
{
    bool operator()(subnet_item const* left, subnet_item const* right) const
    {
        return ((left->subnet_ip.data & left->subnet_mask) < (right->subnet_ip.data & right->subnet_mask));
    }
};

struct ip_searcher
{
    bool operator()(subnet_item const* left, subnet_item const* right) const
    {
        if (left->subnet_mask)
        {
            return ((left->subnet_ip.data & left->subnet_mask) < (right->subnet_ip.data & left->subnet_mask));
        }
        return ((left->subnet_ip.data & right->subnet_mask) < (right->subnet_ip.data & right->subnet_mask));
    }
};

static void hton_bo(u32 const source_ip, subnet_item& dest)
{
    subnet_item source;
    source.subnet_ip.data = source_ip;
    dest.subnet_ip.a1 = source.subnet_ip.a4;
    dest.subnet_ip.a2 = source.subnet_ip.a3;
    dest.subnet_ip.a3 = source.subnet_ip.a2;
    dest.subnet_ip.a4 = source.subnet_ip.a1;
}

#define SUBNET_LIST_SECT_NAME "subnet_list"

u32 ip_filter::load()
{
    string_path temp;
    FS.update_path(temp, "$app_data_root$", "ip_filter.ltx");
    CInifile ini(temp);

    if (!ini.section_exist(SUBNET_LIST_SECT_NAME))
        return 0;

    for (u32 i = 0, line_count = ini.line_count(SUBNET_LIST_SECT_NAME); i != line_count; ++i)
    {
        pcstr address;
        pcstr line;
        ini.r_line(SUBNET_LIST_SECT_NAME, i, &address, &line);
        if (!xr_strlen(address))
            continue;

        subnet_item* tmp_item = new subnet_item();
        unsigned int parse_data[5];
        unsigned int parsed_params = sscanf_s(
            address, "%u.%u.%u.%u/%u", &parse_data[0], &parse_data[1], &parse_data[2], &parse_data[3], &parse_data[4]);
        if ((parsed_params != 5) || (parse_data[0] > 255) || (parse_data[1] > 255) || (parse_data[2] > 255) ||
            (parse_data[3] > 255) || (parse_data[4] == 0))
        {
            VERIFY2(0, make_string("! ERROR: bad subnet: %s", address).c_str());
            xr_delete(tmp_item);
            continue;
        }

        tmp_item->subnet_ip.data = parse_data[3] | (parse_data[2] << 8) | (parse_data[1] << 16) | (parse_data[0] << 24);
        unsigned int zero_count = 32 - parse_data[4];
        tmp_item->subnet_mask = (u32(-1) >> zero_count) << zero_count;
        m_all_subnets.push_back(tmp_item);
    }
    sort(m_all_subnets.begin(), m_all_subnets.end(), subnet_comparator());
    return m_all_subnets.size();
}

bool ip_filter::is_ip_present(u32 ip_address)
{
    if (m_all_subnets.empty())
        return true;
    subnet_item tmp_fake_item;
    hton_bo(ip_address, tmp_fake_item);
    tmp_fake_item.subnet_mask = 0;
    return binary_search(m_all_subnets.begin(), m_all_subnets.end(), &tmp_fake_item, ip_searcher());
}

void ip_filter::unload()
{
    for (subnets_coll_t::iterator i = m_all_subnets.begin(), // delete_data(m_all_subnets);
             ie = m_all_subnets.end();
         i != ie; ++i)
    {
        xr_delete(*i);
    }
    m_all_subnets.clear();
}
