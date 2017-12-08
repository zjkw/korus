#include "socks5_connectcmd_tunnel_client_channel.h"

socks5_connectcmd_tunnel_client_channel::socks5_connectcmd_tunnel_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& addr)
: tcp_client_handler_base(reactor)
{
}


socks5_connectcmd_tunnel_client_channel::~socks5_connectcmd_tunnel_client_channel()
{
}
