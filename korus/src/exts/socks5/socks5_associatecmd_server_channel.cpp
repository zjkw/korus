#include "korus/src//util/socket_ops.h"
#include "korus/src//util/net_serialize.h"
#include "korus/src/tcp/tcp_client_channel.h"
#include "socks5_proto.h"
#include "socks5_associatecmd_server_channel.h"

socks5_associatecmd_server_channel::socks5_associatecmd_server_channel(std::shared_ptr<reactor_loop> reactor)
: _tunnel_valid(false), tcp_server_handler_terminal(reactor)
{
}

socks5_associatecmd_server_channel::~socks5_associatecmd_server_channel()
{
}

//override------------------
void	socks5_associatecmd_server_channel::on_chain_init()
{
}

void	socks5_associatecmd_server_channel::on_chain_final()
{
}

void	socks5_associatecmd_server_channel::on_accept()	//连接已经建立
{
}

void	socks5_associatecmd_server_channel::on_closed()
{
	if (_associatecmd_server_channel)
	{
		_associatecmd_server_channel->close();
	}
}

void	socks5_associatecmd_server_channel::init(const std::string& addr)
{
	assert(!_associatecmd_server_channel);

	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		return;
	}

	udp_server_handler_origin*		origin_channel = new udp_server_handler_origin(reactor(), "0.0.0.0:0");
	std::shared_ptr<socks5_associatecmd_server_channel>	self = std::dynamic_pointer_cast<socks5_associatecmd_server_channel>(shared_from_this());
	_associatecmd_server_channel = std::make_shared<socks5_associatecmd_tunnel_server_channel>(reactor(), self, strtoul(ip.c_str(), NULL, 10), (uint16_t)strtoul(port.c_str(), NULL, 10));

	build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)_associatecmd_server_channel.get());

	_associatecmd_server_channel->start();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_associatecmd_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		return CMS_MANUAL_CONTROL;
	}

	return CMS_INNER_AUTO_CLOSE;	
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_associatecmd_server_channel::on_recv_split(const void* buf, const size_t size)
{
	return size;
}

//这是一个待处理的完整包
void	socks5_associatecmd_server_channel::on_recv_pkg(const void* buf, const size_t size)
{

}

void	socks5_associatecmd_server_channel::on_tunnel_ready(const std::string& addr)
{
	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		assert(false);
		return;
	}
	uint32_t ip_digit = strtoul(ip.c_str(), NULL, 10);
	uint16_t port_digit = (uint16_t)strtoul(port.c_str(), NULL, 10);

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << ip_digit << port_digit;
	send(codec.data(), codec.wpos());
}
