#include "korus/src//util/socket_ops.h"
#include "korus/src//util/net_serialize.h"
#include "korus/src/exts/domain/tcp_client_channel_domain.h"
#include "socks5_proto.h"
#include "socks5_connectcmd_server_channel.h"

socks5_connectcmd_server_channel::socks5_connectcmd_server_channel(std::shared_ptr<reactor_loop> reactor)
	: _tunnel_valid(false), tcp_server_handler_terminal(reactor)
{
}

socks5_connectcmd_server_channel::~socks5_connectcmd_server_channel()
{
}

//tbd client-term需要reactor保持引用，目前这块是缺失的
void	socks5_connectcmd_server_channel::init(const std::string& addr)
{
	tcp_client_channel_domain*		origin_channel = new tcp_client_channel_domain(reactor(), addr);

	std::shared_ptr<socks5_connectcmd_server_channel> self = std::dynamic_pointer_cast<socks5_connectcmd_server_channel>(shared_from_this());
	_connectcmd_tunnel_client_channel = std::make_shared<socks5_connectcmd_tunnel_client_channel>(reactor(), self);
	
	build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)_connectcmd_tunnel_client_channel.get());

	_connectcmd_tunnel_client_channel->connect();
}

//override------------------
void	socks5_connectcmd_server_channel::on_chain_init()
{
}

void	socks5_connectcmd_server_channel::on_chain_final()
{
}

void	socks5_connectcmd_server_channel::on_accept()	//连接已经建立
{
}

void	socks5_connectcmd_server_channel::on_closed()
{
	_connectcmd_tunnel_client_channel->close();//不会形成相互的死锁，因为close本身是有有效性判断的
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_connectcmd_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		_connectcmd_tunnel_client_channel->shutdown(SHUT_WR);

		return CMS_MANUAL_CONTROL;
	}

	return CMS_INNER_AUTO_CLOSE;	
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_connectcmd_server_channel::on_recv_split(const void* buf, const size_t size)
{
	return size;
}

//这是一个待处理的完整包
void	socks5_connectcmd_server_channel::on_recv_pkg(const void* buf, const size_t size)
{
	if (_tunnel_valid)
	{
		_connectcmd_tunnel_client_channel->send(buf, size);
	}
}

void	socks5_connectcmd_server_channel::on_tunnel_connect(const std::string& addr)
{
	struct sockaddr_in si;
	if (!sockaddr_from_string(addr, si))
	{
		return;
	}
	uint32_t ip_digit = ntohl(si.sin_addr.s_addr);
	uint16_t port_digit = ntohs(si.sin_port);

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << ip_digit << port_digit;
	send(codec.data(), codec.wpos());

	_tunnel_valid = true;
}