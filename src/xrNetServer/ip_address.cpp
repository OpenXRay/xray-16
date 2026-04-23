#include "stdafx.h"
#include "ip_address.h"

void ip_address::set(LPCSTR src_string)
{
	u32		buff[4];
	int cnt = sscanf(src_string, "%d.%d.%d.%d", &buff[0], &buff[1], &buff[2], &buff[3]);
	if (cnt == 4)
	{
		m_data.a1 = u8(buff[0] & 0xff);
		m_data.a2 = u8(buff[1] & 0xff);
		m_data.a3 = u8(buff[2] & 0xff);
		m_data.a4 = u8(buff[3] & 0xff);
	}
	else
	{
		// Msg("! Bad ipAddress format [%s]", src_string);
		m_data.data = 0;
	}
}

xr_string ip_address::to_string() const
{
	string128	res;
	xr_sprintf(res, sizeof(res), "%d.%d.%d.%d", m_data.a1, m_data.a2, m_data.a3, m_data.a4);
	return		res;
}

void ip_address::to_buf(char * buf, int count) const
{
  xr_sprintf(buf, count, "%d.%d.%d.%d", m_data.a1, m_data.a2, m_data.a3, m_data.a4);
}
