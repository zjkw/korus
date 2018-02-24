#include <assert.h>
#include "korus/src/util/net_serialize.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_associatecmd_server_channel.h"
#include "socks5_associatecmd_tunnel_server_channel.h"


socks5_associatecmd_tunnel_server_channel::socks5_associatecmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor, std::weak_ptr<socks5_associatecmd_server_channel> channel, uint32_t ip_digit, uint16_t digit_port)
: _origin_channel(channel), _client_ip(ip_digit), _client_port(digit_port), udp_server_handler_terminal(reactor)
{

}

socks5_associatecmd_tunnel_server_channel::~socks5_associatecmd_tunnel_server_channel()
{
}

//override------------------
void	socks5_associatecmd_tunnel_server_channel::on_chain_init()
{

}

void	socks5_associatecmd_tunnel_server_channel::on_chain_final()
{
}

void	socks5_associatecmd_tunnel_server_channel::on_ready()
{
	std::shared_ptr<socks5_associatecmd_server_channel>	channel = _origin_channel.lock();
	if (channel)
	{
		std::string addr;
		local_addr(addr);
		channel->on_tunnel_ready(addr);
	}
}

void	socks5_associatecmd_tunnel_server_channel::on_closed()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_associatecmd_tunnel_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	return CMS_INNER_AUTO_CLOSE;
}

//这是一个待处理的完整包
void	socks5_associatecmd_tunnel_server_channel::on_recv_pkg(const void* buf, const size_t size, const sockaddr_in& peer_addr)
{
	//根据协议，_client_ip可为空，所以不能作为判断依据
	if (_client_ip == ntohl(peer_addr.sin_addr.s_addr) && _client_port == ntohs(peer_addr.sin_port))
	{
		net_serialize	decodec(buf, size);
		uint16_t	rsv	=	0;
		uint8_t		flag =	0;
		uint8_t		atyp =	0;
		decodec >> rsv >> flag >> atyp;
		if (!decodec)
		{
			return;
		}
		if (0x01 != atyp)
		{
			return;
		}

		uint32_t	ip = 0;
		uint16_t	port = 0;
		decodec >> ip >> port;
		if (!decodec)
		{
			return;
		}

		sockaddr_in target_addr;
		sockaddr_from_string(ip, port, target_addr);

		uint8_t* rpos = (uint8_t*)decodec.data();
		send((void*)(rpos + decodec.rpos()), decodec.size() - decodec.rpos(), target_addr);
	}
	else
	{
		std::string temp_buf;
		temp_buf.resize(size + 10);
		net_serialize	codec(temp_buf.data(), temp_buf.size());

		codec << static_cast<uint16_t>(0x0000) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << static_cast<uint32_t>(ntohl(peer_addr.sin_addr.s_addr)) << static_cast<uint16_t>(htons(peer_addr.sin_port));
		codec.write(buf, size);
		if (!codec)
		{
			return;
		}

		sockaddr_in target_addr;
		sockaddr_from_string(_client_ip, _client_port, target_addr);

		send(codec.data(), codec.size(), target_addr);
	}
}
