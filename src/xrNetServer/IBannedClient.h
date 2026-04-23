#pragma once
#include "ip_address.h"

class XRNETSERVER_API IBannedClient
{
public:
	ip_address			HAddr;
	time_t				BanTime;

	IBannedClient()
	{
		HAddr.m_data.data = 0;
		BanTime = 0;
	};
	void				Load(CInifile& ini, const shared_str& sect);
	void				Save(CInifile& ini);

	xr_string			BannedTimeTo() const;
};
