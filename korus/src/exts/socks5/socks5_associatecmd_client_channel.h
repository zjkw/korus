#pragma once

#include "korus/src/exts/domain/domain_async_resolve_helper.h"
#include "socks5_client_channel_base.h"

// 逻辑时序
// 1，socks5_associatecmd_client_channel 向代理服务器执行associate_cmd操作，udp连接目标服务器

class socks5_associatecmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_associatecmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_port, const std::string& socks_user, const std::string& socks_psw, const udp_client_channel_factory_t& udp_factory);
	virtual ~socks5_associatecmd_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

private:
	std::string  _local_port;
	std::shared_ptr<udp_client_handler_base>	_channel;
	udp_client_channel_factory_t _udp_factory;

	uint16_t	_port;
	domain_async_resolve_helper			_resolve;
	void	on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip);

	bool	create_udp_client_channel(const sockaddr_in& si);

	virtual int32_t	make_tunnel_pkg(void* buf, const uint16_t size);
	virtual void	on_tunnel_pkg(const void* buf, const uint16_t size);
};
