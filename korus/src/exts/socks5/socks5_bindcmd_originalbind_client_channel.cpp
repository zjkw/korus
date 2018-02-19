#include <assert.h>
#include "socks5_bindcmd_client_channel.h"
#include "korus/src/util/socket_ops.h"
#include "korus/src/util/net_serialize.h"
#include "socks5_bindcmd_originalbind_client_channel.h"

socks5_bindcmd_originalbind_client_channel::socks5_bindcmd_originalbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
: _server_addr(server_addr), _integration(nullptr), socks5_client_channel_base(reactor, socks_user, socks_psw)
{

}

socks5_bindcmd_originalbind_client_channel::~socks5_bindcmd_originalbind_client_channel()
{
}

//override------------------
void	socks5_bindcmd_originalbind_client_channel::on_chain_init()
{
}

void	socks5_bindcmd_originalbind_client_channel::on_chain_final()
{
}

void	socks5_bindcmd_originalbind_client_channel::chain_inref()
{
	_integration->chain_inref();
}

void	socks5_bindcmd_originalbind_client_channel::chain_deref()
{
	_integration->chain_deref();
}

void	socks5_bindcmd_originalbind_client_channel::on_connected()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_connected();
}

void	socks5_bindcmd_originalbind_client_channel::on_closed()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_originalbind_client_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (!_integration)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _integration->on_data_error(code);
}

int32_t socks5_bindcmd_originalbind_client_channel::on_recv_split(const void* buf, const size_t size)
{
	if (!_integration)
	{
		assert(false);
		return 0;
	}

	return _integration->on_data_recv_split(buf, size);
}

void	socks5_bindcmd_originalbind_client_channel::on_recv_pkg(const void* buf, const size_t size)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_recv_pkg(buf, size);
}

int32_t	socks5_bindcmd_originalbind_client_channel::make_tunnel_pkg(void* buf, const uint16_t size)
{
	net_serialize	codec(buf, size);

	codec << static_cast<uint8_t>(SOCKS5_V);
	codec << static_cast<uint8_t>(0x02);
	codec << static_cast<uint8_t>(0x00);

	std::string host, port;
	if (!sockaddr_from_string(_server_addr, host, port))
	{
		assert(false);
		return -1;
	}
	SOCK_ADDR_TYPE	sat = addrtype_from_string(_server_addr);
	if (sat == SAT_DOMAIN)
	{
		codec << static_cast<uint8_t>(0x03);
		codec << static_cast<uint8_t>(host.size());
		codec.write(host.data(), host.size());
	}
	else if (sat == SAT_IPV4)
	{
		struct in_addr ia;
		inet_aton(host.c_str(), &ia);

		codec << static_cast<uint8_t>(0x01);
		codec << static_cast<uint32_t>(ntohl(ia.s_addr));
		codec << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
	}
	else
	{
		assert(false);
		return -1;
	}

	return (int32_t)codec.wpos();
}

void	socks5_bindcmd_originalbind_client_channel::on_tunnel_pkg(const void* buf, const uint16_t size)
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
		decodec >> u32ip;
		if (!decodec)
		{
			close();
			return;
		}

		struct in_addr si;
		si.s_addr = u32ip;
		char str[16];
		if (!inet_ntop(AF_INET, &si, str, sizeof(str)))
		{
			close();
			return;
		}

		printf("socks5 ipv4: %s\n", str);
	}
	break;
	case 0x03:
	{
		uint8_t	u8domainlen = 0;
		decodec >> u8domainlen;
		char    szdomain[257];
		decodec.read(szdomain, u8domainlen);
		if (!decodec)
		{
			close();
			return;
		}
		szdomain[u8domainlen] = 0;

		printf("socks5 domain: %s\n", szdomain);
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

	//Í¨ÖªÍâ²ã
	tcp_client_handler_base::on_connected();
}