#include "korus/src//util/socket_ops.h"
#include "korus/src//util/net_serialize.h"
#include "socks5_proto.h"
#include "korus/src/exts/domain/tcp_client_channel_domain.h"
#include "socks5_bindcmd_server_channel.h"

socks5_bindcmd_server_channel::socks5_bindcmd_server_channel(std::shared_ptr<reactor_loop> reactor)
: _bindcmd_server(nullptr), _is_valid(false), tcp_server_handler_terminal(reactor)
{
}

socks5_bindcmd_server_channel::~socks5_bindcmd_server_channel()
{
	delete _bindcmd_server;
	_bindcmd_server = nullptr;
}

//override------------------
void	socks5_bindcmd_server_channel::on_chain_init()
{
}

void	socks5_bindcmd_server_channel::on_chain_final()
{
}

void	socks5_bindcmd_server_channel::on_accept()	//连接已经建立
{
}

void	socks5_bindcmd_server_channel::on_closed()
{

}

//tbd BIND请求包中仍然指明FTPSERVER.ADDR / FTPSERVER.PORT。SOCKS Server应该据此进行评估。
void	socks5_bindcmd_server_channel::init(const std::string& addr)
{
	if(_bindcmd_server)
	{
		assert(false);
		return;
	}

	_bindcmd_server = new tcp_server<reactor_loop>(reactor(), "0.0.0.0:0", std::bind(&socks5_bindcmd_server_channel::binccmd_channel_factory, this, std::placeholders::_1));
	_bindcmd_server->start();

	std::string listen_addr;
	_bindcmd_server->listen_addr(listen_addr);

	std::string ip;
	std::string port;
	if (!sockaddr_from_string(addr, ip, port))
	{
		delete _bindcmd_server;
		_bindcmd_server = nullptr;
		return;
	}

	uint8_t buf[32];
	net_serialize	codec(buf, sizeof(buf));
	codec << static_cast<uint8_t>(SOCKS5_V) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x00) << static_cast<uint8_t>(0x01) << static_cast<uint32_t>(strtoul(ip.c_str(), NULL, 10)) << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
	send(codec.data(), codec.wpos());
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_bindcmd_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		std::shared_ptr<socks5_bindcmd_tunnel_server_channel>		channel =	_bindcmd_tunnel_server_channel.lock();
		if (channel)
		{
			channel->shutdown(SHUT_WR);
		}
		return CMS_MANUAL_CONTROL;
	}

	return CMS_INNER_AUTO_CLOSE;	
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_bindcmd_server_channel::on_recv_split(const void* buf, const size_t size)
{
	return size;
}


//这是一个待处理的完整包
void	socks5_bindcmd_server_channel::on_recv_pkg(const void* buf, const size_t size)
{
	if (_is_valid)
	{
		std::shared_ptr<socks5_bindcmd_tunnel_server_channel>		channel = _bindcmd_tunnel_server_channel.lock();
		if (!channel)
		{
			close();
			return;
		}

		channel->send(buf, size);
	}
}

void	socks5_bindcmd_server_channel::on_tunnel_accept(std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel)
{
	if (!_bindcmd_tunnel_server_channel.expired())
	{
		assert(false);
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

	_is_valid = true;
}


std::shared_ptr<tcp_server_handler_base> socks5_bindcmd_server_channel::binccmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
{	
	std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel = std::make_shared<socks5_bindcmd_tunnel_server_channel>(reactor, std::dynamic_pointer_cast<socks5_bindcmd_server_channel>(shared_from_this()));
	std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(channel);
	return cb;
}
