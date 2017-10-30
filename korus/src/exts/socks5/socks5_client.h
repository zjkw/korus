#pragma once

#include "korus/src/tcp/tcp_client.h"
#include "socks5_client_channel.h"

/////////////////////////////////////// tcp_client
// Õ¼¿Ó
template<typename T>
class socks5_tcp_client
{
public:
	socks5_tcp_client(){}
	virtual ~socks5_tcp_client(){}
};

template <>
class socks5_tcp_client<uint16_t> : tcp_client<uint16_t>
{
public:
	socks5_tcp_client(uint16_t thread_num, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: tcp_client(thread_num, server_addr, factory,	connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
		_factory_chain.push_back(factory);
		_factory_chain.push_back(std::bind(&socks5_tcp_client::socks5_factory, this));
		inner_init();
	}
	virtual ~socks5_tcp_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_factory()
	{
//		std::shared_ptr<socks5_client_channel>	channel = std::make_shared<socks5_client_channel>();
		return nullptr;// std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}

};

///////////////////////////////////// udp_client