#include <assert.h>
#include "korus/src/util/net_serialize.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_associatecmd_client_channel.h"

socks5_associatecmd_client_channel::socks5_associatecmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& udp_listen_ip, const std::string& socks_user, const std::string& socks_psw, const udp_server_channel_factory_t& udp_factory)
: _udp_listen_ip(udp_listen_ip), _proxy_udp_port(0), _filter_channel(nullptr), _udp_factory(udp_factory), _terminal_channel(nullptr), socks5_client_channel_base(reactor, socks_user, socks_psw)
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

int32_t	socks5_associatecmd_client_channel::make_tunnel_pkg(void* buf, const uint16_t size)
{
	//创建udp
	if (!create_udp_server_channel())
	{
		return -1;
	}
	std::string local_addr;
	_terminal_channel->local_addr(local_addr);
	struct sockaddr_in si;
	if (!sockaddr_from_string(local_addr, si))
	{
		return -1;
	}
	uint32_t ip_digit = ntohl(si.sin_addr.s_addr);
	uint16_t port_digit = ntohs(si.sin_port);
		
	net_serialize	codec(buf, size);

	codec << static_cast<uint8_t>(SOCKS5_V);
	codec << static_cast<uint8_t>(0x03);
	codec << static_cast<uint8_t>(0x00);

	codec << static_cast<uint8_t>(0x01);
	codec << ip_digit;//严格说，需要与udp具体监听地址一样，但我们监听是先从"0.0.0.0:0"这里开启
	codec << port_digit;

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

	struct sockaddr_in si;

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
			std::string addr;
			if (!string_from_ipport(addr, u32ip, u16port))
			{
				close();
				return;
			}
			if (!sockaddr_from_string(addr, si))
			{
				close();
				return;
			}

			printf("socks5 associate cmd ipv4: %s\n", addr.c_str());
		}
		break;
	case 0x03:
		{
			uint8_t	u8domainlen = 0;
			decodec >> u8domainlen;
			char    szdomain[257];
			decodec.read(szdomain, u8domainlen);
			decodec >> _proxy_udp_port;
			if (!decodec)
			{
				close();
				return;
			}
			szdomain[u8domainlen] = 0;

			printf("socks5 associate cmd domain: %s:%d\n", szdomain, _proxy_udp_port);

			// tbd
			std::string ip;
			DOMAIN_RESOLVE_STATE state = _resolve.start(szdomain, ip);	//无视成功，强制更新
			if (DRS_SUCCESS == state)
			{
				std::string addr;
				if (!string_from_ipport(addr, ip, _proxy_udp_port))
				{
					close();
					return;
				}
				if (!sockaddr_from_string(addr, si))
				{
					close();
					return;
				}
			}
			else if (DRS_PENDING != state)
			{
				close();
				return;
			}
			else
			{
				return;//waiting
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
	
	_filter_channel->switch_normal(si);
	_filter_channel->on_ready();
}

bool socks5_associatecmd_client_channel::create_udp_server_channel()
{
	if (_terminal_channel)
	{
		return true;
	}
	std::string bind_addr = _udp_listen_ip + ":0";
	udp_server_handler_origin* origin_channel = new udp_server_handler_origin(reactor(), bind_addr);
	_filter_channel = new socks5_associatecmd_filter_server_channel(reactor());
	_terminal_channel = _udp_factory(reactor());
	build_channel_chain_helper((udp_server_handler_base*)origin_channel, (udp_server_handler_base*)_filter_channel, (udp_server_handler_base*)_terminal_channel.get());

	_terminal_channel->start();

	return true;
}

void	socks5_associatecmd_client_channel::on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
{
	if (result == DRS_SUCCESS)
	{
		std::string addr;
		if (!string_from_ipport(addr, ip, _proxy_udp_port))
		{
			close();
			return;
		}
		struct sockaddr_in si;
		if (!sockaddr_from_string(addr, si))
		{
			close();
			return;
		}
		_shakehand_state = SCS_NORMAL;

		//通知外层
		tcp_client_handler_base::on_connected();

		_filter_channel->switch_normal(si);
		_filter_channel->on_ready();		
	}
	else
	{
		close();
		return;
	}
}