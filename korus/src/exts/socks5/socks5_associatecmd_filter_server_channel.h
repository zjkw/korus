#pragma once

#include "korus/src/udp/udp_server_channel.h"

//专门控制udp监听的同步on_ready事件
class socks5_associatecmd_filter_server_channel : public udp_server_handler_base
{
public:
	socks5_associatecmd_filter_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_associatecmd_filter_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();

	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	virtual int32_t	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	void			switch_normal(const struct sockaddr_in& si);

private:
	bool			_normal_state;

	struct sockaddr_in _si;

};