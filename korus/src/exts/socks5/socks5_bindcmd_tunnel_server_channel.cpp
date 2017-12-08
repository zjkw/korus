#include "socks5_bindcmd_tunnel_server_channel.h"

socks5_bindcmd_tunnel_server_channel::socks5_bindcmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor)
: tcp_server_handler_base(reactor)
{
}


socks5_bindcmd_tunnel_server_channel::~socks5_bindcmd_tunnel_server_channel()
{
}
