#include <assert.h>
#include "socks5_bindcmd_client_channel.h"
#include "korus/src/util/socket_ops.h"
#include "socks5_bindcmd_originalbind_client_channel.h"

socks5_bindcmd_originalbind_client_channel::socks5_bindcmd_originalbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
: _server_addr(server_addr), _tunnelResState(TRS_NONE), socks5_client_channel_base(reactor, socks_user, socks_psw)
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

bool	socks5_bindcmd_originalbind_client_channel::make_tunnel_pkg(net_serialize&	codec)
{
	codec << static_cast<uint8_t>(SOCKS5_V);
	codec << static_cast<uint8_t>(0x02);
	codec << static_cast<uint8_t>(0x00);

	std::string host, port;
	if (!sockaddr_from_string(_server_addr, host, port))
	{
		assert(false);
		return false;
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
		return false;
	}

	return codec;
}

void	socks5_bindcmd_originalbind_client_channel::on_tunnel_pkg(net_serialize&	decodec)
{
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

	std::string addr;

	// strict check ?
	switch (u8atyp)
	{
	case 0x01:
		{
			uint32_t	ip = 0;
			uint16_t	port = 0;
			decodec >> ip >> port;
			if (!decodec)
			{
				close();
				return;
			}
			if (!string_from_ipport(addr, ip, port))
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
			uint16_t port;
			decodec >> port;
			if (!decodec)
			{
				close();
				return;
			}
			szdomain[u8domainlen] = 0;

			if (!string_from_ipport(addr, szdomain, port))
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

	switch (_tunnelResState)
	{
	case TRS_NONE:
		_tunnelResState = TRS_PROXYSVR_LISTEN_ADDRESS;
		printf("socks5 bindcmd proxy_server_listen: %s\n", addr.c_str());
		_integration->on_data_prepare(addr);
		break;
	case TRS_PROXYSVR_LISTEN_ADDRESS:
		_tunnelResState = TRS_TARGETSVR_CONNECT_ADDRESS;
		//通知外层
		printf("socks5 bindcmd target_server local connect: %s\n", addr.c_str());
		_shakehand_state = SCS_NORMAL;
		tcp_client_handler_base::on_connected();//必须tcp_client_handler_base
		break;
	case TRS_TARGETSVR_CONNECT_ADDRESS:
	default:
		assert(false);
		break;
	}
}