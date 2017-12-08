#pragma once

#include "korus/src/tcp/tcp_client_channel.h"

class socks5_connectcmd_tunnel_client_channel : public tcp_client_handler_base, public multiform_state
{
public:
	socks5_connectcmd_tunnel_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& addr);
	virtual ~socks5_connectcmd_tunnel_client_channel();
};

