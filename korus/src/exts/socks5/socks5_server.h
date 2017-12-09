#pragma once

#include "korus/src/tcp/tcp_server.h"
#include "socks5_server_channel.h"

// 占坑
template<typename T>
class socks5_server
{
public:
	socks5_server(){}
	virtual ~socks5_server(){}
};

// 一个tcp_server拥有多线程
template <>
class socks5_server<uint16_t> : public tcp_server<uint16_t>
{
public:
	socks5_server(uint16_t thread_num, const std::string& listen_addr, const std::shared_ptr<socks5_server_auth> auth, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _auth(auth),
		tcp_server(thread_num, listen_addr, std::bind(&socks5_server::channel_factory, this, std::placeholders::_1), backlog, defer_accept,
		self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

private:
	std::shared_ptr<socks5_server_auth> _auth;

	std::shared_ptr<tcp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
	{
		std::shared_ptr<socks5_server_channel> channel = std::make_shared<socks5_server_channel>(reactor, _auth);
		std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(channel);
		return cb;
	}
};

template <>
class socks5_server<reactor_loop> : public tcp_server<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_server(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, const std::shared_ptr<socks5_server_auth> auth, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _auth(auth), 
		tcp_server(reactor, listen_addr, std::bind(&socks5_server::channel_factory, this, std::placeholders::_1), backlog, defer_accept,
		self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

private:
	std::shared_ptr<socks5_server_auth> _auth;

	std::shared_ptr<tcp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
	{
		std::shared_ptr<socks5_server_channel> channel = std::make_shared<socks5_server_channel>(reactor, _auth);
		std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(channel);
		return cb;
	}
};