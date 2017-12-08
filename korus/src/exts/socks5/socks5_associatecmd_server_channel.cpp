#include <assert.h>
#include "korus/src/util/net_serialize.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_associatecmd_server_channel.h"

socks5_associatecmd_server_channel::socks5_associatecmd_server_channel(std::shared_ptr<reactor_loop> reactor)
: udp_server_handler_base(reactor)
{

}

socks5_associatecmd_server_channel::~socks5_associatecmd_server_channel()
{
}
