#pragma once

#include "socks5_connectcmd_client_channel.h"

class socks5_bindcmd_client_channel;

class socks5_connectcmd_embedbind_client_channel : public socks5_connectcmd_client_channel
{
public:
	socks5_connectcmd_embedbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_connectcmd_embedbind_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual long	chain_refcount();
	virtual std::shared_ptr<chain_sharedobj_interface> chain_terminal();
	void set_integration(std::shared_ptr<socks5_bindcmd_client_channel> integration) { _integration = integration; }

	virtual void	on_connected();
	virtual void	on_closed();
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);

	virtual int32_t on_recv_split(const void* buf, const size_t size);
	virtual void	on_recv_pkg(const void* buf, const size_t size);

private:
	std::shared_ptr<socks5_bindcmd_client_channel> _integration;
	
	virtual void	on_shakehandler_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);
};
