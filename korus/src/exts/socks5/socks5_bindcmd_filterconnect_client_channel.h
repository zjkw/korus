#pragma once

#include "socks5_connectcmd_client_channel.h"

class socks5_bindcmd_client_handler_origin;

class socks5_bindcmd_filterconnect_client_channel : public tcp_client_handler_base
{
public:
	socks5_bindcmd_filterconnect_client_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_filterconnect_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	chain_inref();
	virtual void	chain_deref();
	void set_integration(socks5_bindcmd_client_handler_origin* integration) { _integration = integration; }

	virtual void	on_connected();
	virtual void	on_closed();
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);

	virtual int32_t on_recv_split(const void* buf, const size_t size);
	virtual void	on_recv_pkg(const void* buf, const size_t size);

private:
	socks5_bindcmd_client_handler_origin* _integration;
};
