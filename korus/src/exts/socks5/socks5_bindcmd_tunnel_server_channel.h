#pragma once

#include "korus/src/tcp/tcp_server_channel.h"

class socks5_bindcmd_tunnel_server_channel : public tcp_server_handler_base, public multiform_state
{
public:
	socks5_bindcmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_tunnel_server_channel();
};