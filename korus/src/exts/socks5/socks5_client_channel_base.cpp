#include <assert.h>
#include <string.h>
#include "socks5_proto.h"
#include "socks5_client_channel_base.h"

socks5_client_channel_base::socks5_client_channel_base(std::shared_ptr<reactor_loop> reactor, const std::string& socks_user, const std::string& socks_psw)
	: _socks_user(socks_user), _socks_psw(socks_psw), _shakehand_state(SCS_NONE), tcp_client_handler_base(reactor)
{

}

socks5_client_channel_base::~socks5_client_channel_base()
{
}

//override------------------
void	socks5_client_channel_base::on_chain_init()
{
}

void	socks5_client_channel_base::on_chain_final()
{
}

void	socks5_client_channel_base::on_connected()
{
	uint8_t	buf[2048];
	net_serialize	codec(buf, sizeof(buf));

	bool	suc = make_method_pkg(codec);
	if (!suc)
	{
		close();
		return;
	}
	send(codec);

	_shakehand_state = SCS_METHOD;
}

void	socks5_client_channel_base::on_closed()
{
	_shakehand_state = SCS_NONE;

	tcp_client_handler_base::on_closed();
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_client_channel_base::on_recv_split(const std::shared_ptr<buffer_thunk>& data)
{
	switch (_shakehand_state)
	{
	case SCS_METHOD:
		if (data->used() >= 2)
		{
			return 2;
		}
		break;
	case SCS_AUTH:
		if (data->used() >= 2)
		{
			return 2;
		}
		break;
	case SCS_TUNNEL:
		if (data->used() > 4)
		{
			net_serialize	decodec(data->ptr(), data->used());

			decodec.read_skip(3);
			uint8_t	u8atyp = 0;
			decodec >> u8atyp;
			switch (u8atyp)
			{
			case 0x01:
				if (data->used() >= 10)
				{
					return 10;
				}
				break;
			case 0x03:
				if (data->used() >= 7)
				{
					uint8_t	u8domainlen = 0;
					decodec >> u8domainlen;
					if (data->used() > u8domainlen + 7)
					{
						return u8domainlen + 7;
					}
				}
				break;
			case 0x04:
				if (data->used() >= 22)
				{
					return 22;
				}
				break;
			default:
				break;
			}
		}
		break;
	case SCS_NORMAL:
		return tcp_client_handler_base::on_recv_split(data);
	case SCS_NONE:
	default:
		assert(false);
		break;
	}

	return 0;
}

//这是一个待处理的完整包
void	socks5_client_channel_base::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data)
{
	net_serialize	decodec(data->ptr(), data->used());
	switch (_shakehand_state)
	{
	case SCS_METHOD:
		on_method_pkg(decodec);
		break;
	case SCS_AUTH:
		on_auth_pkg(decodec);
		break;
	case SCS_TUNNEL:
		on_tunnel_pkg(decodec);
		break;
	case SCS_NORMAL:
		tcp_client_handler_base::on_recv_pkg(data);
		break;
	case SCS_NONE:
	default:
		assert(false);
		break;
	}
}

//////////////////////////
bool	socks5_client_channel_base::make_method_pkg(net_serialize&	codec)
{
	codec << static_cast<uint8_t>(SOCKS5_V);
	if (_socks_user.empty())
	{
		codec << static_cast<uint8_t>(1);
		codec << static_cast<uint8_t>(0x00);
	}
	else
	{
		codec << static_cast<uint8_t>(2);
		codec << static_cast<uint8_t>(0x00);
		codec << static_cast<uint8_t>(0x02);
	}
	return codec;
}

void	socks5_client_channel_base::on_method_pkg(net_serialize&	decodec)
{
	uint8_t	u8ver = 0;
	uint8_t	u8method = 0;
	decodec >> u8ver >> u8method;

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

	switch (u8method)
	{
	case 0x00:   //auth: none
		{
			// send
			uint8_t	send_buf[2048];
			net_serialize	codec(send_buf, sizeof(send_buf));
			bool	suc = make_tunnel_pkg(codec);
			if (!suc)
			{
				close();
				return;
			}

			send(codec);

			_shakehand_state = SCS_TUNNEL;
		}
		break;
	case 0x02:	//auth: user+psw
		{
			// send
			uint8_t	send_buf[2048];
			net_serialize	codec(send_buf, sizeof(send_buf));
			bool	suc = make_auth_pkg(codec);
			if (!suc)
			{
				close();
				return;
			}

			send(codec);

			_shakehand_state = SCS_AUTH;
		}
		break;
	case 0xff:  //fall through
	default:
		if (CMS_INNER_AUTO_CLOSE == on_error(CEC_SOCKS5_METHOD_NOSUPPORTED))
		{
			close();
		}
		break;
	}
}

bool	socks5_client_channel_base::make_auth_pkg(net_serialize&	codec)
{
	codec << static_cast<uint8_t>(0x01);

	codec << static_cast<uint8_t>(_socks_user.size());
	codec.write(_socks_user.data(), _socks_user.size());
	codec << static_cast<uint8_t>(_socks_psw.size());
	codec.write(_socks_psw.data(), _socks_psw.size());

	return codec;
}

void	socks5_client_channel_base::on_auth_pkg(net_serialize&	decodec)
{
	uint8_t	u8ver = 0;
	uint8_t	u8status = 0;
	decodec >> u8ver >> u8status;

	if (!decodec)
	{
		close();
		return;
	}

	if (0x01 != u8ver)
	{
		close();
		return;
	}

	if (0x00 != u8status)
	{
		if (CMS_INNER_AUTO_CLOSE == on_error(CEC_SOCKS5_AUTH_FAILED))
		{
			close();
		}
		return;
	}

	// send
	uint8_t	send_buf[2048];
	net_serialize	codec(send_buf, sizeof(send_buf));
	bool suc = make_tunnel_pkg(codec);
	if (!suc)
	{
		close();
		return;
	}

	send(codec);

	_shakehand_state = SCS_TUNNEL;
}

CHANNEL_ERROR_CODE socks5_client_channel_base::convert_error_code(uint8_t u8rep)
{
	switch (u8rep)
	{
	case 0x00:
		return CEC_NONE;
	case 0x01:
		return CEC_SOCKS5_REQUEST_FAILED;
	case 0x02:
		return CEC_SOCKS5_REGUAL_DISALLOW;
	case 0x03:
		return CEC_SOCKS5_NET_UNREACHABLE;
	case 0x04:
		return CEC_SOCKS5_HOST_UNREACHABLE;
	case 0x05:
		return CEC_SOCKS5_CONN_REFUSE;
	case 0x06:
		return CEC_SOCKS5_TTL_TIMEOUT;
	case 0x08:
		return CEC_SOCKS5_CMD_NOSUPPORTED;
	case 0x09:
		return CEC_SOCKS5_ADDR_NOSUPPORTED;
	default:
		return CEC_SOCKS5_UNKNOWN;
	}
}

bool	socks5_client_channel_base::send(const net_serialize&	codec)
{
	if (!codec)
	{
		return false;
	}
	std::shared_ptr<buffer_thunk>	thunk = std::make_shared<buffer_thunk>(codec.data(), codec.wpos());
	tcp_client_handler_base::send(thunk);
	return true;
}