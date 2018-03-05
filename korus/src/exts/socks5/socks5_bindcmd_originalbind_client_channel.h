#pragma once

#include "socks5_client_channel_base.h"

class socks5_bindcmd_client_handler_origin;

class socks5_bindcmd_originalbind_client_channel : public socks5_client_channel_base
{
public:
	socks5_bindcmd_originalbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_bindcmd_originalbind_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
		
	void set_integration(socks5_bindcmd_client_handler_origin* integration) { _integration = integration; }

private:
	std::string _server_addr;
	
	virtual bool	make_tunnel_pkg(net_serialize&	codec);
	virtual void	on_tunnel_pkg(net_serialize&	decodec);

	enum TunnerResState
	{
		TRS_NONE = 0,
		TRS_PROXYSVR_LISTEN_ADDRESS = 1,
		TRS_TARGETSVR_CONNECT_ADDRESS = 2
	};
	TunnerResState	_tunnelResState;

	socks5_bindcmd_client_handler_origin* _integration;
};
