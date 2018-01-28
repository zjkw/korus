#include "korus/src//util/net_serialize.h"
#include "korus/src/exts/domain/tcp_client_channel_domain.h"
#include "socks5_server_channel.h"

socks5_server_channel::socks5_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_auth> auth)
: _tunnel_channel_type(TCT_NONE), _shakehand_state(SSS_NONE), _auth(auth), _bindcmd_server(nullptr), 
tcp_server_handler_terminal(reactor)
{
}

socks5_server_channel::~socks5_server_channel()
{
	delete _bindcmd_server;
	_bindcmd_server = nullptr;
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
		return size;
	case TCT_BIND:
		return size;
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
					uint8_t	cmd = 0;
					uint8_t	rsv = 0;
					uint8_t	atyp = 0;
					decodec >> ver >> cmd >> rsv >> atyp;
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

							switch (cmd)
							{
							case 0x01:
								if (!build_connectcmd_tunnel(ip, port))
								{
									_shakehand_state = SSS_NONE;

									close();
									return;
								}

								break;
							case 0x02:
								if (!build_bindcmd_tunnel(ip, port))
								{
									_shakehand_state = SSS_NONE;

									close();
									return;
								}
								break;
							case 0x03:
								if (!build_associatecmd_tunnel(ip, port))
								{
									_shakehand_state = SSS_NONE;

									close();
									return;
								}
								break;
							default:
								_shakehand_state = SSS_NONE;
								close();
								break;
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

							switch (cmd)
							{
							case 0x01:
								if (!build_connectcmd_tunnel(domain, port))
								{
									_shakehand_state = SSS_NONE;

									close();
									return;
								}
								break;
							default:
								_shakehand_state = SSS_NONE;
								close();
								break;
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

	tcp_client_handler_origin*		origin_channel = new tcp_client_handler_origin(reactor(), addr);
	std::shared_ptr<socks5_server_channel> self = std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this());
	_connectcmd_tunnel_client_channel = std::make_shared<socks5_connectcmd_tunnel_client_channel>(reactor(), self);
	build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)_connectcmd_tunnel_client_channel.get());

	_connectcmd_tunnel_client_channel->connect();

	return true;
}

bool	socks5_server_channel::build_connectcmd_tunnel(const std::string& ip, uint16_t port)
{
	assert(!_connectcmd_tunnel_client_channel);
	char addr[256];
	snprintf(addr, sizeof(addr), "%s:%u", ip.c_str(), port);

	tcp_client_channel_domain*		origin_channel = new tcp_client_channel_domain(reactor(), addr);
	std::shared_ptr<socks5_server_channel> self = std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this());
	_connectcmd_tunnel_client_channel = std::make_shared<socks5_connectcmd_tunnel_client_channel>(reactor(), self);
	build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)_connectcmd_tunnel_client_channel.get());

	_connectcmd_tunnel_client_channel->connect();

	return true;
}

bool	socks5_server_channel::build_bindcmd_tunnel(uint32_t ip_digit, uint16_t port_digit)
{
	assert(!_bindcmd_server);
	_bindcmd_server = new tcp_server<reactor_loop>(reactor(), "0.0.0.0:0", std::bind(&socks5_server_channel::binccmd_channel_factory, this, std::placeholders::_1));
	_bindcmd_server->start();

	std::string addr;
	_bindcmd_server->listen_addr(addr);

	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		_bindcmd_server = nullptr;
		return false;
	}

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << static_cast<uint32_t>(strtoul(ip.c_str(), NULL, 10)) << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
	send(codec.data(), codec.wpos());

	return true;
}

bool	socks5_server_channel::build_associatecmd_tunnel(uint32_t ip_digit, uint16_t port_digit)
{
	assert(!_associatecmd_server_channel);

	udp_server_handler_origin*		origin_channel = new udp_server_handler_origin(reactor(), "0.0.0.0:0");
	std::shared_ptr<socks5_server_channel>	self = std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this());
	_associatecmd_server_channel = std::make_shared<socks5_associatecmd_tunnel_server_channel>(reactor(), self, ip_digit, port_digit);
	
	build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)_associatecmd_server_channel.get());

	_associatecmd_server_channel->start();

	return true;
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

void	socks5_server_channel::on_bindcmd_tunnel_accept(std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel)
{
	if (_bindcmd_tunnel_server_channel)
	{
		_bindcmd_tunnel_server_channel->close();
		return;
	}

	_bindcmd_tunnel_server_channel = channel;
	std::string addr;
	channel->peer_addr(addr);

	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		return;
	}

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x03) << static_cast<uint32_t>(strtoul(ip.c_str(), NULL, 10)) << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
	send(codec.data(), codec.wpos());
}

void	socks5_server_channel::on_bindcmd_tunnel_close()
{

}

void	socks5_server_channel::on_associatecmd_tunnel_ready(const std::string& addr)
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

void	socks5_server_channel::on_associatecmd_tunnel_close()
{

}

std::shared_ptr<tcp_server_handler_base> socks5_server_channel::binccmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
{	
	std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel = std::make_shared<socks5_bindcmd_tunnel_server_channel>(reactor, std::dynamic_pointer_cast<socks5_server_channel>(shared_from_this()));
	std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(channel);
	return cb;
}

std::shared_ptr<udp_client_handler_base> socks5_server_channel::associatecmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<udp_client_handler_base> handler = std::make_shared<udp_client_handler_base>(reactor);
	std::shared_ptr<udp_client_handler_base> cb = std::dynamic_pointer_cast<udp_client_handler_base>(handler);
	return cb;
}