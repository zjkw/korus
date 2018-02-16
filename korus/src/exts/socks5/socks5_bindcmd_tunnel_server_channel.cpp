#include "socks5_bindcmd_server_channel.h"
#include "socks5_bindcmd_tunnel_server_channel.h"

socks5_bindcmd_tunnel_server_channel::socks5_bindcmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_bindcmd_server_channel> channel)
: _origin_channel(channel), tcp_server_handler_terminal(reactor)
{
}

socks5_bindcmd_tunnel_server_channel::~socks5_bindcmd_tunnel_server_channel()
{
}


//override------------------
void	socks5_bindcmd_tunnel_server_channel::on_chain_init()
{
}

void	socks5_bindcmd_tunnel_server_channel::on_chain_final()
{
}

void	socks5_bindcmd_tunnel_server_channel::on_accept()	//连接已经建立
{
	std::shared_ptr<socks5_bindcmd_server_channel>	channel = _origin_channel.lock();
	if (!channel)
	{
		close();
		return;
	}
	channel->on_tunnel_accept(std::dynamic_pointer_cast<socks5_bindcmd_tunnel_server_channel>(this->shared_from_this()));
}

void	socks5_bindcmd_tunnel_server_channel::on_closed()
{
	std::shared_ptr<socks5_bindcmd_server_channel>	channel = _origin_channel.lock();
	if (!channel)
	{
		close();
		return;
	}
	channel->close();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_bindcmd_tunnel_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		std::shared_ptr<socks5_bindcmd_server_channel>	channel = _origin_channel.lock();
		if (channel)
		{
			channel->shutdown(SHUT_RD);
			return CMS_MANUAL_CONTROL;
		}
	}

	return CMS_INNER_AUTO_CLOSE;
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_bindcmd_tunnel_server_channel::on_recv_split(const void* buf, const size_t size)
{
	return size;
}

//这是一个待处理的完整包
void	socks5_bindcmd_tunnel_server_channel::on_recv_pkg(const void* buf, const size_t size)
{
	std::shared_ptr<socks5_bindcmd_server_channel>	channel = _origin_channel.lock();
	if (!channel)
	{
		close();
		return;
	}
	channel->send(buf, size);
}