#ifndef XRCLIENTS_POOL_H_INCLUDED
#define XRCLIENTS_POOL_H_INCLUDED

class xrClientData;
struct game_PlayerState;

class xrClientsPool
{
public:
			xrClientsPool				();
			~xrClientsPool				();

	void			Clear				();
	void			Add					(xrClientData* new_dclient);
	xrClientData*	Get					(xrClientData* new_client);
private:
	void	ClearExpiredClients			();

	struct dclient
	{
		xrClientData*	m_client;
		u32				m_dtime;
	};//struct dclient
	struct expired_client_deleter
	{
		//copy constructor is valid
		u32				m_expire_time;
		u32				m_current_time;
		bool const		operator()(dclient & right) const;
	};//struct expired_client_deleter
	struct pooled_client_finder
	{
		//copy constructor is valid
		xrClientData*		m_new_client;
		bool const		operator()(dclient const & right) const;
	};//struct pooled_client_finder

	typedef xr_vector<dclient>			dclients_t;
	dclients_t							m_dclients;
};//class xrClientsPool

#endif//#ifndef XRCLIENTS_POOL_H_INCLUDED