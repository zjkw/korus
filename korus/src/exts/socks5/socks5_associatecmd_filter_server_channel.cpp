#include <assert.h>
#include "korus/src/util/net_serialize.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_associatecmd_filter_server_channel.h"

socks5_associatecmd_filter_server_channel::socks5_associatecmd_filter_server_channel(std::shared_ptr<reactor_loop> reactor)
	: _normal_state(false), udp_server_handler_base(reactor)
{

}

socks5_associatecmd_filter_server_channel::~socks5_associatecmd_filter_server_channel()
{
}

//override------------------
void	socks5_associatecmd_filter_server_channel::on_chain_init()
{

}

void	socks5_associatecmd_filter_server_channel::on_chain_final()
{
}

void	socks5_associatecmd_filter_server_channel::on_ready()
{
	if (_normal_state)
	{
		udp_server_handler_base::on_ready();
	}
}

void	socks5_associatecmd_filter_server_channel::switch_normal(const struct sockaddr_in& si)
{
	_normal_state = true;
	_si = si;
	connect(si);
}

//这是一个待处理的完整包
void	socks5_associatecmd_filter_server_channel::on_recv_pkg(const void* buf, const size_t size, const sockaddr_in& peer_addr)
{
	if (!_normal_state)
	{
		assert(false);
		return;
	}

	net_serialize	decodec(buf, size);
	uint16_t	rsv = 0;
	uint8_t		flag = 0;
	uint8_t		atyp = 0;
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

	struct sockaddr_in target_addr;
	if (!sockaddr_from_string(ip, port, target_addr))
	{
		return;
	}

	uint8_t* rpos = (uint8_t*)decodec.data();
	udp_server_handler_base::on_recv_pkg((void*)(rpos + decodec.rpos()), decodec.size() - decodec.rpos(), target_addr);
}

int32_t	socks5_associatecmd_filter_server_channel::send(const void* buf, const size_t size, const sockaddr_in& peer_addr)
{
	if (!_normal_state)
	{
		assert(false);
		return - 1;
	}

	std::string local_ad;
	std::string proxy_ad;
	std::string peer_ad;
	local_addr(local_ad);
	string_from_sockaddr(proxy_ad, _si);
	string_from_sockaddr(peer_ad, peer_addr);
	printf("\nlocal: %s, proxy: %s, target: %s\n", local_ad.c_str(), proxy_ad.c_str(), peer_ad.c_str());
	
	std::string temp_buf;
	temp_buf.resize(size + 10);
	net_serialize	codec(temp_buf.data(), temp_buf.size());

	codec << static_cast<uint16_t>(0x0000) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << static_cast<uint32_t>(ntohl(peer_addr.sin_addr.s_addr)) << static_cast<uint16_t>(ntohs(peer_addr.sin_port));
	codec.write(buf, size);
	if (!codec)
	{
		assert(false);
		return -1;
	}

	//如果使用connnect了，则直接send，否则带上地址
	return udp_server_handler_base::send(codec.data(), codec.size()/*, _si*/);
}

int32_t	socks5_associatecmd_filter_server_channel::send(const void* buf, const size_t size)
{
	//无目标，不建议这么做
	assert(false);
	return -1;
}