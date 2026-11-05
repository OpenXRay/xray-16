#pragma once

struct XRNETSERVER_API ip_address
{
	union {
		struct {
			u8	a1;
			u8	a2;
			u8	a3;
			u8	a4;
		};
		u32		data;
	}m_data;
	void		set(LPCSTR src_string);
	xr_string	to_string()	const;
  void	to_buf(char* buf, int count)	const;

	bool operator == (const ip_address& other) const
	{
		return (m_data.data == other.m_data.data) ||
			((m_data.a1 == other.m_data.a1) &&
			(m_data.a2 == other.m_data.a2) &&
				(m_data.a3 == other.m_data.a3) &&
				(m_data.a4 == 0));
	}
};

