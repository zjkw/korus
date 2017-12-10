#include "korus/src//util/net_serialize.h"
#include "korus/src/exts/domain/tcp_client_channel_domain.h"
#include "socks5_server_channel.h"

socks5_server_channel::socks5_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_auth> auth)
	: _tunnel_channel_type(TCT_NONE), _shakehand_state(SSS_NONE), _auth(auth),
	tcp_server_handler_base(reactor)
{
}

socks5_server_channel::~socks5_server_channel()
{
}

//override------------------
void	socks5_server_channel::on_chain_init()
{
}

void	socks5_server_channel::on_chain_final()
{
	_connectcmd_tunnel_client_channel = nullptr;
	_bindcmd_tunnel_server_channel = nullptr;
	_associatecmd_server_channel = nullptr;
}

void	socks5_server_channel::on_chain_zomby()
{
	// 因为没有被其他对象引用，本对象可在框架要求下退出，可以主动与消去外界引用
}

long	socks5_server_channel::chain_refcount()
{
	return tcp_server_handler_base::chain_refcount();
}

void	socks5_server_channel::on_accept()	//连接已经建立
{
	_shakehand_state = SSS_METHOD;
}

void	socks5_server_channel::on_closed()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		switch (_tunnel_channel_type)
		{
		case socks5_server_channel::TCT_CONNECT:
			_connectcmd_tunnel_client_channel->shutdown(SHUT_WR);
			return CMS_MANUAL_CONTROL;
		case socks5_server_channel::TCT_BIND:
			_bindcmd_tunnel_server_channel->shutdown(SHUT_WR);
			return CMS_MANUAL_CONTROL;
		case socks5_server_channel::TCT_ASSOCIATE:
		//	_associatecmd_server_channel->close();
			break;
		case socks5_server_channel::TCT_NONE:
		default:
			break;
		}
	}

	return CMS_INNER_AUTO_CLOSE;	
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_server_channel::on_recv_split(const void* buf, const size_t size)
{
	switch (_tunnel_channel_type)
	{
	case TCT_NONE:
		{
			net_serialize	decodec(buf, size);
			uint8_t ver = 0;
			decodec >> ver;

			switch (_shakehand_state)
			{
			case SSS_METHOD:
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
				break;
			case SSS_AUTH:			//auth,tunnel
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
				break;
			case SSS_TUNNEL:
				if (ver == SOCKS5_V)
				{
					decodec.read_skip(3);
					uint8_t	u8atyp = 0;
					decodec >> u8atyp;
					switch (u8atyp)
					{
					case 0x01:
						if (size >= 10)
						{
							return 10;
						}
						break;
					case 0x03:
						if (size >= 7)
						{
							uint8_t	u8domainlen = 0;
							decodec >> u8domainlen;
							if (size > u8domainlen + 7)
							{
								return u8domainlen + 7;
							}
						}
						break;
					case 0x04:
						if (size >= 22)
						{
							return 22;
						}
						break;
					default:
						break;
					}
					if (decodec)
					{
						return decodec.rpos();
					}
				}
				break;
			default:
				assert(false);
				break;
			}
		}
		break;
	case TCT_CONNECT:
		assert(_connectcmd_tunnel_client_channel);
		return _connectcmd_tunnel_client_channel->on_recv_split(buf, size);
	case TCT_BIND:
		assert(_bindcmd_tunnel_server_channel);
		return _bindcmd_tunnel_server_channel->on_recv_split(buf, size);
	case TCT_ASSOCIATE:	//忽略
		break;
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
		case 0x05:
			method_list.push_back(SMT_NOAUTH);
			break;
		case 0x01:
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
		*methods = 0x05;
		return true;
	case SMT_USERPSW:
		*methods = 0x01;
		return true;
	default:
		return false;
	}
}

//这是一个待处理的完整包
void	socks5_server_channel::on_recv_pkg(const void* buf, const size_t size)
{
	switch (_tunnel_channel_type)
	{
	case TCT_NONE:
		{
			net_serialize	decodec(buf, size);
			uint8_t ver = 0;
			decodec >> ver;

			switch (_shakehand_state)
			{
			case SSS_METHOD:
				{
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
					decodec.read((char*)psw.data(), ulen);
					decodec >> plen;
					user.resize(ulen);
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
					if (_auth->is_valid(user, psw))
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
					if (ver != SOCKS5_V)
					{
						_shakehand_state = SSS_NONE;

						close();
						return;
					}

					uint8_t	ver = 0;
					uint8_t	rep = 0;
					uint8_t	rsv = 0;
					uint8_t	atyp = 0;
					decodec >> ver >> rep >> rsv >> atyp;
					if (!decodec)
					{
						_shakehand_state = SSS_NONE;

						close();
						return;
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
								_shakehand_state = SSS_NONE;

								close();
								return;
							}

							if (!build_connectcmd_tunnel(ip, port))
							{
								_shakehand_state = SSS_NONE;

								close();
								return;
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
								_shakehand_state = SSS_NONE;

								close();
								return;
							}

							if (!build_connectcmd_tunnel(domain, port))
							{
								_shakehand_state = SSS_NONE;

								close();
								return;
							}
						}
						break;
					case 0x04:
					default:
						_shakehand_state = SSS_NONE;
						close();
						return;
					}
				}
				break;
			default:
				assert(false);
				break;
			}
		}
		break;
	case TCT_CONNECT:
		assert(_connectcmd_tunnel_client_channel);
		_connectcmd_tunnel_client_channel->send(buf, size);
		break;
	case TCT_BIND:
		assert(_bindcmd_tunnel_server_channel);
		_bindcmd_tunnel_server_channel->send(buf, size);
		break;
	case TCT_ASSOCIATE:	//忽略
		break;
	default:
		assert(false);
		break;
	}
}

bool	socks5_server_channel::build_connectcmd_tunnel(uint32_t ip, uint16_t port)
{
	assert(!_connectcmd_tunnel_client_channel);
	char addr[256];
	snprintf(addr, sizeof(addr), "%u:%u", ip, port);

	std::shared_ptr<socks5_server_channel> self = std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this());
	_connectcmd_tunnel_client_channel = std::make_shared<socks5_connectcmd_tunnel_client_channel>(reactor(), self);
	std::shared_ptr<tcp_client_channel>		origin_channel = std::make_shared<tcp_client_channel>(reactor(), addr);
	build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(_connectcmd_tunnel_client_channel));

	_connectcmd_tunnel_client_channel->connect();

	return true;
}

bool	socks5_server_channel::build_connectcmd_tunnel(const std::string& ip, uint16_t port)
{
	assert(!_connectcmd_tunnel_client_channel);
	char addr[256];
	snprintf(addr, sizeof(addr), "%s:%u", ip.c_str(), port);

	std::shared_ptr<socks5_server_channel> self = std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this());
	_connectcmd_tunnel_client_channel = std::make_shared<socks5_connectcmd_tunnel_client_channel>(reactor(), self);
	std::shared_ptr<tcp_client_channel_domain>		origin_channel = std::make_shared<tcp_client_channel_domain>(reactor(), addr);
	build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(_connectcmd_tunnel_client_channel));

	_connectcmd_tunnel_client_channel->connect();

	return true;
}

bool	socks5_server_channel::build_bindcmd_tunnel()
{

}

bool	socks5_server_channel::build_associatecmd_tunnel()
{

}

/////////////////////////////////////
void	socks5_server_channel::on_connectcmd_tunnel_connect(const std::string& addr)
{
	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		return;
	}

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << static_cast<uint32_t>(strtoul(ip.c_str(), NULL, 10)) << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
	send(codec.data(), codec.wpos());
}

void	socks5_server_channel::on_connectcmd_tunnel_close()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_server_channel::on_connectcmd_tunnel_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		shutdown(SHUT_RD);
		return CMS_MANUAL_CONTROL;
	}
	else
	{
		return CMS_INNER_AUTO_CLOSE;
	}
}