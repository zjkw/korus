#pragma once

#include "korus/src/udp/udp_server_channel.h"
#include "korus/src/exts/domain/domain_async_resolve_helper.h"
#include "socks5_client_channel_base.h"
#include "socks5_associatecmd_filter_server_channel.h"

// 逻辑时序
// 1，socks5_associatecmd_client_channel 向代理服务器执行associate_cmd操作，udp连接目标服务器

class socks5_associatecmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_associatecmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& udp_listen_ip, const std::string& socks_user, const std::string& socks_psw, const udp_server_channel_factory_t& udp_factory);
	virtual ~socks5_associatecmd_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

private:
	socks5_associatecmd_filter_server_channel*	_filter_channel;
	complex_ptr<udp_server_handler_base>		_terminal_channel;
	udp_server_channel_factory_t				_udp_factory;
	std::string									_udp_listen_ip;
	uint16_t									_proxy_udp_port;//用于解析域名时候缓存的端口

	domain_async_resolve_helper					_resolve;
	void	on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip);

	bool	create_udp_server_channel();

	virtual int32_t	make_tunnel_pkg(void* buf, const uint16_t size);
	virtual void	on_tunnel_pkg(const void* buf, const uint16_t size);
};
