#include <assert.h>
#include "korus/src/util/net_serialize.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_associatecmd_client_channel.h"

socks5_associatecmd_client_channel::socks5_associatecmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_port, const std::string& socks_user, const std::string& socks_psw, const udp_client_channel_factory_t& udp_factory)
: _local_port(local_port), _udp_factory(udp_factory), _port(0), socks5_client_channel_base(reactor, socks_user, socks_psw)
{
	_resolve.reactor(reactor.get());
	_resolve.bind(std::bind(&socks5_associatecmd_client_channel::on_resolve_result, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

socks5_associatecmd_client_channel::~socks5_associatecmd_client_channel()
{
}

//override------------------
void	socks5_associatecmd_client_channel::on_chain_init()
{

}

void	socks5_associatecmd_client_channel::on_chain_final()
{
}

void	socks5_associatecmd_client_channel::on_chain_zomby()
{
	if (_channel)
	{
		try_chain_final(_channel, 1);
		_channel = nullptr;
	}
}

int32_t	socks5_associatecmd_client_channel::make_tunnel_pkg(void* buf, const uint16_t size)
{
	net_serialize	codec(buf, size);

	codec << static_cast<uint8_t>(SOCKS5_V);
	codec << static_cast<uint8_t>(0x01);
	codec << static_cast<uint8_t>(0x00);

	codec << static_cast<uint8_t>(0x01);
	codec << static_cast<uint32_t>(0x00);
	uint16_t port = strtoul(_local_port.c_str(), NULL, 10);
	codec << static_cast<uint32_t>(port);

	return (int32_t)codec.wpos();
}

void	socks5_associatecmd_client_channel::on_tunnel_pkg(const void* buf, const uint16_t size)
{
	net_serialize	decodec(buf, size);

	uint8_t	u8ver = 0;
	uint8_t	u8rep = 0;
	uint8_t	u8rsv = 0;
	uint8_t	u8atyp = 0;
	decodec >> u8ver >> u8rep >> u8rsv >> u8atyp;

	if (!decodec)
	{
		close();
		return;
	}

	if (SOCKS5_V != u8ver)
	{
		close();
		return;
	}

	if (u8rep)
	{
		CHANNEL_ERROR_CODE code = convert_error_code(u8rep);
		if (CMS_INNER_AUTO_CLOSE == on_error(code))
		{
			close();
		}
		return;
	}

	// strict check ?
	switch (u8atyp)
	{
	case 0x01:
	{
		uint32_t	u32ip = 0;
		uint16_t	u16port = 0;
		decodec >> u32ip >> u16port;
		if (!decodec)
		{
			close();
			return;
		}

		struct sockaddr_in si;
		si.sin_family = AF_INET;
		si.sin_addr.s_addr = u32ip;
		si.sin_port = u16port;
		char ip[16];
		if (!inet_ntop(si.sin_family, &si.sin_addr, ip, sizeof(ip)))
		{
			close();
			return;
		}

		printf("socks5 ipv4: %s:%d\n", ip, ntohs(u16port));
		if (!create_udp_client_channel(si))
		{
			close();
			return;
		}
	}
	break;
	case 0x03:
	{
		uint8_t	u8domainlen = 0;
		decodec >> u8domainlen;
		char    szdomain[257];
		decodec.read(szdomain, u8domainlen);
		decodec >> _port;
		if (!decodec)
		{
			close();
			return;
		}
		szdomain[u8domainlen] = 0;

		printf("socks5 domain: %s:%d\n", szdomain, ntohs(_port));

		// tbd
		std::string ip;
		DOMAIN_RESOLVE_STATE state = _resolve.start(szdomain, ip);	//无视成功，强制更新
		if (DRS_SUCCESS == state)
		{
			sockaddr_in si;
			if (!sockaddr_from_string(ip, ntohs(_port), si))
			{
				close();
				return;
			}
			create_udp_client_channel(si);
		}
		else if (DRS_PENDING != state)
		{
			close();
			return;
		}
	}
	break;
	case 0x04:
	{
		close();
		return;
	}
	break;
	default:
		break;
	}
	//

	_shakehand_state = SCS_NORMAL;

	//通知外层
	tcp_client_handler_base::on_connected();
}

bool socks5_associatecmd_client_channel::create_udp_client_channel(const sockaddr_in& si)
{
	if (_channel)
	{
		return true;
	}
	std::string bind_addr = std::string("0.0.0.0") + ":" + _local_port;
	_channel = std::make_shared<udp_client_channel>(reactor(), bind_addr);
	_channel->connect(si);
	std::shared_ptr<udp_client_handler_base>	terminal_channel = _udp_factory(reactor());
	build_channel_chain_helper(std::dynamic_pointer_cast<udp_client_handler_base>(_channel), terminal_channel);
	_channel->start();

	return true;
}

void	socks5_associatecmd_client_channel::on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
{
	if (result == DRS_SUCCESS)
	{
		struct sockaddr_in si;
		if (!sockaddr_from_string(ip, _port, si))
		{
			close();
			return;
		}
		create_udp_client_channel(si);
	}
	else
	{
		close();
		return;
	}
}