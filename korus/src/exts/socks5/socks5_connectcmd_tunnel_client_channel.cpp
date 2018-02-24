#include <assert.h>
#include "socks5_connectcmd_server_channel.h"
#include "socks5_connectcmd_tunnel_client_channel.h"

socks5_connectcmd_tunnel_client_channel::socks5_connectcmd_tunnel_client_channel(std::shared_ptr<reactor_loop> reactor, std::weak_ptr<socks5_connectcmd_server_channel> server_channel)
: _server_channel(server_channel), tcp_client_handler_terminal(reactor)
{
}

socks5_connectcmd_tunnel_client_channel::~socks5_connectcmd_tunnel_client_channel()
{
}

//override------------------
void	socks5_connectcmd_tunnel_client_channel::on_chain_init()
{
}

void	socks5_connectcmd_tunnel_client_channel::on_chain_final()
{
}

void	socks5_connectcmd_tunnel_client_channel::on_connected()
{
	std::shared_ptr<socks5_connectcmd_server_channel> channel = _server_channel.lock();
	if (!channel)
	{
		close();
		return;
	}

	std::string addr;
	if (!peer_addr(addr))
	{
		close();
		return;
	}

	channel->on_tunnel_connect(addr);
}

void	socks5_connectcmd_tunnel_client_channel::on_closed()
{
	std::shared_ptr<socks5_connectcmd_server_channel> channel = _server_channel.lock();
	if (!channel)
	{
		return;
	}

	channel->close();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_connectcmd_tunnel_client_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		std::shared_ptr<socks5_connectcmd_server_channel> channel = _server_channel.lock();
		if (channel)
		{
			channel->shutdown(SHUT_WR);
		}
		return CMS_MANUAL_CONTROL;
	}

	return CMS_INNER_AUTO_CLOSE;
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_connectcmd_tunnel_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return len;
}

//这是一个待处理的完整包
void	socks5_connectcmd_tunnel_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	std::shared_ptr<socks5_connectcmd_server_channel> channel = _server_channel.lock();
	if (!channel)
	{
		close();
		return;
	}

	channel->send(buf, len);
}

