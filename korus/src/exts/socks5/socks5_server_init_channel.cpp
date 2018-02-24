#include "korus/src//util/socket_ops.h"
#include "korus/src//util/net_serialize.h"
#include "socks5_connectcmd_server_channel.h"
#include "socks5_bindcmd_server_channel.h"
#include "socks5_associatecmd_server_channel.h"
#include "socks5_server_init_channel.h"

socks5_server_init_channel::socks5_server_init_channel(std::shared_ptr<reactor_loop> reactor, const std::string& udp_listen_ip, std::shared_ptr<socks5_server_auth> auth)
: _shakehand_state(SSS_NONE), _udp_listen_ip(udp_listen_ip), _auth(auth),
tcp_server_handler_base(reactor)
{
}

socks5_server_init_channel::~socks5_server_init_channel()
{
}

//override------------------
void	socks5_server_init_channel::on_chain_init()
{
}

void	socks5_server_init_channel::on_chain_final()
{
}

void	socks5_server_init_channel::on_accept()	//连接已经建立
{
	_shakehand_state = SSS_METHOD;
}

void	socks5_server_init_channel::on_closed()
{
	if (SSS_NORMAL == _shakehand_state)
	{
		tcp_server_handler_base::on_closed();
	}
}

void	socks5_server_init_channel::chain_inref()
{
	if (SSS_NORMAL == _shakehand_state)
	{
		tcp_server_handler_base::chain_inref();
	}
	else
	{
		incr_ref();
	}
}

void	socks5_server_init_channel::chain_deref()
{
	if (SSS_NORMAL == _shakehand_state)
	{
		tcp_server_handler_base::chain_deref();
	}
	else
	{
		if (!decr_ref())
		{
			chain_final();
		}
	}
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_server_init_channel::on_error(CHANNEL_ERROR_CODE code)
{
	//通道阶段，交给上层处理
	if (SSS_NORMAL == _shakehand_state)
	{
		return _tunnel_next->on_error(code);
	}
	else
	{
		return CMS_INNER_AUTO_CLOSE;	
	}
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_server_init_channel::on_recv_split(const void* buf, const size_t size)
{
	switch (_shakehand_state)
	{
		case SSS_METHOD:
		{
			net_serialize	decodec(buf, size);
			uint8_t ver = 0;
			decodec >> ver;

			if (ver == SOCKS5_V)	//method
			{
				uint8_t nmethod = 0;
				decodec >> nmethod;
				decodec.read_skip(nmethod);
				if (decodec)
				{
					return decodec.rpos();
				}
			}
		}
		break;
	case SSS_AUTH:				//auth,tunnel
		{
			net_serialize	decodec(buf, size);
			uint8_t ver = 0;
			decodec >> ver;

			if (ver == 0x01)
			{
				uint8_t ulen = 0;
				uint8_t plen = 0;
				decodec >> ulen;
				decodec.read_skip(ulen);
				decodec >> plen;
				decodec.read_skip(plen);
				if (decodec)
				{
					return decodec.rpos();
				}
			}
		}
		break;
	case SSS_TUNNEL:
		{
			net_serialize	decodec(buf, size);
			uint8_t ver = 0;
			decodec >> ver;
			if (ver == SOCKS5_V)
			{
				decodec.read_skip(2);
				uint8_t	u8atyp = 0;
				decodec >> u8atyp;
				switch (u8atyp)
				{
				case 0x01:
					decodec.read_skip(6);
					break;
				case 0x03:
					{
						uint8_t	u8domainlen = 0;
						decodec >> u8domainlen;
						if (decodec)
						{
							decodec.read_skip(u8domainlen);
						}
					}
					break;
				case 0x04:
					decodec.read_skip(180);
					break;
				default:
					break;
				}
				if (decodec)
				{
					return decodec.rpos();
				}
			}
		}
		break;
	case SSS_NORMAL:
		return _tunnel_next->on_recv_split(buf, size);
	default:
		assert(false);
		break;
	}

	return 0;
}

static	bool	bin2method(std::vector<SOCKS_METHOD_TYPE>&	method_list, uint8_t* methods, uint8_t nmethod)
{
	method_list.clear();
	for (size_t i = 0; i < nmethod; i++)
	{
		switch (methods[i])
		{
		case 0x000:
			method_list.push_back(SMT_NOAUTH);
			break;
		case 0x02:
			method_list.push_back(SMT_USERPSW);
			break;
		}
	}
	return true;
}

static	bool	method2bin(uint8_t* methods, const SOCKS_METHOD_TYPE&	method_type)
{
	switch (method_type)
	{
	case SMT_NOAUTH:
		*methods = 0x00;
		return true;
	case SMT_USERPSW:
		*methods = 0x02;
		return true;
	default:
		return false;
	}
}

static bool    decode_addr(net_serialize& decodec, std::string& addr)
{
	uint8_t	atyp = 0;
	decodec >> atyp;
	if (!decodec)
	{
		return false;
	}

	switch (atyp)
	{
	case 0x01:
		{
			uint32_t	ip = 0;
			uint16_t	port = 0;
			decodec >> ip >> port;
			if (!decodec)
			{
				return false;
			}

			if (!string_from_ipport(addr, ip, port))
			{
				return false;
			}
		}
		break;
	case 0x03:
		{
			uint8_t	domainlen = 0;
			std::string domain;
			decodec >> domainlen;
			domain.resize(domainlen);
			decodec.read((char*)domain.data(), domainlen);
			uint16_t	port = 0;
			decodec >> port;
			if (!decodec)
			{
				return false;
			}

			if (!string_from_ipport(addr, domain, port))
			{
				return false;
			}
		}
		break;
	case 0x04:
	default:
		return false;
	}

	return true;
}

//这是一个待处理的完整包
void	socks5_server_init_channel::on_recv_pkg(const void* buf, const size_t size)
{
	switch (_shakehand_state)
	{
		case SSS_METHOD:
			{
				net_serialize	decodec(buf, size);
				uint8_t ver = 0;
				decodec >> ver;

				if (ver != SOCKS5_V)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}
				uint8_t nmethod = 0;
				decodec >> nmethod;
				if (nmethod > 5)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}
				uint8_t methods[256];
				decodec.read(methods, nmethod);
				if (!decodec)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}
				std::vector<SOCKS_METHOD_TYPE>	method_list;
				bin2method(method_list, methods, nmethod);
				SOCKS_METHOD_TYPE method_ret = _auth->select_method(method_list);
				uint8_t method_bin = 0;
				method2bin(&method_bin, method_ret);

				uint8_t buf_ret[2];
				net_serialize	codec(buf_ret, sizeof(buf_ret));
				codec << static_cast<uint8_t>(SOCKS5_V);

				if (SMT_NOAUTH == method_ret)
				{
					_shakehand_state = SSS_TUNNEL;

					codec << method_bin;
					send(buf_ret, codec.wpos());
				}
				else if (SMT_USERPSW == method_ret)
				{
					_shakehand_state = SSS_AUTH;

					codec << method_bin;
					send(buf_ret, codec.wpos());
				}
				else
				{
					_shakehand_state = SSS_NONE;

					// tbd 失败情况下，则加上服务器主动关闭？ delay_close或close加上参数？
					codec << static_cast<uint8_t>(0xff);
					send(buf_ret, codec.wpos());
				}
			}
			break;
		case SSS_AUTH:
			{
				net_serialize	decodec(buf, size);
				uint8_t ver = 0;
				decodec >> ver;

				if (ver != 0x01)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}

				uint8_t ulen = 0;
				std::string user;
				uint8_t plen = 0;
				std::string psw;

				decodec >> ulen;
				user.resize(ulen);
				decodec.read((char*)user.data(), ulen);
				decodec >> plen;
				psw.resize(plen);
				decodec.read((char*)psw.data(), plen);
				if (!decodec)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}

				uint8_t buf_ret[2];
				net_serialize	codec(buf_ret, sizeof(buf_ret));
				codec << static_cast<uint8_t>(0x01);
				if (_auth->check_userpsw(user, psw))
				{
					codec << static_cast<uint8_t>(0x00);
					send(buf_ret, codec.wpos());

					_shakehand_state = SSS_TUNNEL;
				}
				else
				{
					_shakehand_state = SSS_NONE;

					// tbd 失败情况下，则加上服务器主动关闭？ delay_close或close加上参数？
					codec << static_cast<uint8_t>(0x01);
					send(buf_ret, codec.wpos());
				}
			}
			break;
		case SSS_TUNNEL:
			{
				net_serialize	decodec(buf, size);
				uint8_t ver = 0;
				decodec >> ver;
				if (ver != SOCKS5_V)
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}

				uint8_t	cmd = 0;
				uint8_t	rsv = 0;
				decodec >> cmd >> rsv;
				std::string addr;
				if (!decodec || !decode_addr(decodec, addr))
				{
					_shakehand_state = SSS_NONE;

					close();
					return;
				}

				switch (cmd)
				{
				case 0x01:
					{
						std::shared_ptr<socks5_connectcmd_server_channel> term = std::make_shared<socks5_connectcmd_server_channel>(reactor());
						chain_insert(term.get());
						_shakehand_state = SSS_NORMAL;
						term->transfer_ref(get_ref());
						term->init(addr);
					}
					break;
				case 0x02:
					{
						std::shared_ptr<socks5_bindcmd_server_channel> term = std::make_shared<socks5_bindcmd_server_channel>(reactor());
						chain_insert(term.get());
						_shakehand_state = SSS_NORMAL;
						term->transfer_ref(get_ref());
						term->init(addr);
					}
					break;
				case 0x03:
					{
						std::shared_ptr<socks5_associatecmd_server_channel> term = std::make_shared<socks5_associatecmd_server_channel>(reactor());
						chain_insert(term.get());
						_shakehand_state = SSS_NORMAL;
						term->transfer_ref(get_ref());
						term->init(addr, _udp_listen_ip);
					}
					break;
				default:
					_shakehand_state = SSS_NONE;
					close();
					break;
				}
			}
			break;
		case SSS_NORMAL:
			{
				_tunnel_next->on_recv_pkg(buf, size);
			}
		default:
			assert(false);
			break;
	}
}
