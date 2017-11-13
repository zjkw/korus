#pragma once

#include "socks5_connectcmd_client_channel.h"

class socks5_bindcmd_integration_handler_base;

class socks5_connectcmd_embedbind_client_channel : public socks5_connectcmd_client_channel
{
public:
	socks5_connectcmd_embedbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_connectcmd_embedbind_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_connected();
	virtual void	on_closed();
	CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);		//参考CHANNEL_ERROR_CODE定义	
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len);

	virtual long	chain_refcount();

	void set_integration(std::shared_ptr<socks5_bindcmd_integration_handler_base> integration) { _integration = integration; }

private:
	std::shared_ptr<socks5_bindcmd_integration_handler_base> _integration;

	virtual std::shared_ptr<chain_sharedobj_interface> chain_terminal();
};
