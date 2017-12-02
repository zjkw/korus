#pragma once

#include "socks5_client_channel_base.h"

// 逻辑时序
// 1，socks5_associatecmd_client_channel 向代理服务器执行associate_cmd操作，tcp连接目标服务器

class socks5_connectcmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_connectcmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_connectcmd_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

private:
	std::string _server_addr;

	virtual int32_t	make_tunnel_pkg(void* buf, const uint16_t size);
	virtual void	on_tunnel_pkg(const void* buf, const uint16_t size);

	virtual void	on_shakehandler_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);
};
