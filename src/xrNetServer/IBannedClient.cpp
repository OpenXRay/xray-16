#include "stdafx.h"
#include "IBannedClient.h"

void IBannedClient::Load(CInifile& ini, const shared_str& sect)
{
	HAddr.set(sect.c_str());

	tm							_tm_banned;
	const shared_str& time_to = ini.r_string(sect, "time_to");
	int res_t = sscanf(time_to.c_str(),
		"%02d.%02d.%d_%02d:%02d:%02d",
		&_tm_banned.tm_mday,
		&_tm_banned.tm_mon,
		&_tm_banned.tm_year,
		&_tm_banned.tm_hour,
		&_tm_banned.tm_min,
		&_tm_banned.tm_sec);
	VERIFY(res_t == 6);

	_tm_banned.tm_mon -= 1;
	_tm_banned.tm_year -= 1900;

	BanTime = mktime(&_tm_banned);

	Msg("- loaded banned client %s to %s", HAddr.to_string().c_str(), BannedTimeTo().c_str());
}

void IBannedClient::Save(CInifile& ini)
{
	ini.w_string(HAddr.to_string().c_str(), "time_to", BannedTimeTo().c_str());
}

xr_string IBannedClient::BannedTimeTo() const
{
	string256			res;
	tm*					_tm_banned;
	_tm_banned = _localtime64(&BanTime);
	xr_sprintf(res, sizeof(res),
		"%02d.%02d.%d_%02d:%02d:%02d",
		_tm_banned->tm_mday,
		_tm_banned->tm_mon + 1,
		_tm_banned->tm_year + 1900,
		_tm_banned->tm_hour,
		_tm_banned->tm_min,
		_tm_banned->tm_sec);

	return res;
}
