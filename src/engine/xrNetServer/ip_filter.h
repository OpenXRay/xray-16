#ifndef NET_SERVER_IP_FILTER
#define NET_SERVER_IP_FILTER

struct subnet_item
{
	union
	{
		struct
		{
			u8	a1;
			u8	a2;
			u8	a3;
			u8	a4;
		};
		u32		data;
	} subnet_ip;			//IN NBO !!!
	u32 subnet_mask;
	subnet_item()
	{
		subnet_ip.data = 0;
		subnet_mask = 0;
	}
};


class ip_filter
{
	typedef xr_vector<subnet_item*> subnets_coll_t;
	subnets_coll_t m_all_subnets;
public:
	ip_filter		();
	~ip_filter		();

	u32		load			();
	void	unload			();
	bool	is_ip_present	(u32 ip_address);
};

#endif //#ifndef NET_SERVER_IP_FILTER