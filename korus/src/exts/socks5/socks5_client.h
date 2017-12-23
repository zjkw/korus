#pragma once

#include "string.h"
#include "korus/src/tcp/tcp_client.h"
#include "korus/src/udp/udp_client.h"
#include "socks5_connectcmd_client_channel.h"
#include "socks5_bindcmd_client_channel.h"	//取代bindcmd作为入口
#include "socks5_associatecmd_client_channel.h"
/////////////////////////////////////// connect_cmd_mode

// 占坑
template<typename T>
class socks5_connectcmd_client
{
public:
	socks5_connectcmd_client(){}
	virtual ~socks5_connectcmd_client(){}
};

template <>
class socks5_connectcmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	socks5_connectcmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: tcp_client(thread_num, proxy_addr, factory, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size),
		_server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{
	}
	virtual ~socks5_connectcmd_client()
	{
	}

private:
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)	
	{
		std::shared_ptr<tcp_client_channel>					origin_channel = create_origin_channel(reactor);
		std::shared_ptr<socks5_connectcmd_client_channel>	sock5_channel = std::make_shared<socks5_connectcmd_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);
		std::shared_ptr<tcp_client_handler_base>			terminal_channel = create_terminal_channel(reactor);
		
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(sock5_channel), terminal_channel);
		origin_channel->connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
};

template <>
class socks5_connectcmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_connectcmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: tcp_client(reactor, proxy_addr, factory, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size),
		_server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{

	}

	virtual ~socks5_connectcmd_client()
	{
	}
private:
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)	
	{
		std::shared_ptr<tcp_client_channel>					origin_channel = create_origin_channel(reactor);
		std::shared_ptr<socks5_connectcmd_client_channel>	sock5_channel = std::make_shared<socks5_connectcmd_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);
		std::shared_ptr<tcp_client_handler_base>			terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(sock5_channel), terminal_channel);
		origin_channel->connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
};


///////////////////////////////////// bind_cmd_mode
// 占坑
template<typename T>
class socks5_bindcmd_client
{
public:
	socks5_bindcmd_client(){}
	virtual ~socks5_bindcmd_client(){}
};

template <>
class socks5_bindcmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	socks5_bindcmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const socks5_bindcmd_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _factory(factory), _proxy_addr(proxy_addr), _socks_user(socks_user), _socks_psw(socks_psw),
		tcp_client(thread_num, proxy_addr, nullptr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_bindcmd_client()
	{
	}

private:
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)	
	{
		std::shared_ptr<socks5_bindcmd_client_channel>		origin_channel = std::make_shared<socks5_bindcmd_client_channel>(reactor, _proxy_addr, _server_addr, _socks_user, _socks_psw, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		std::shared_ptr<socks5_bindcmd_client_handler_base>	terminal_channel = _factory(reactor);
		build_channel_chain_helper(std::dynamic_pointer_cast<socks5_bindcmd_client_handler_base>(origin_channel), terminal_channel);
		origin_channel->ctrl_connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}

	std::string _proxy_addr;
	std::string _socks_user;
	std::string _socks_psw;
	socks5_bindcmd_client_channel_factory_t _factory;
};

template <>
class socks5_bindcmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_bindcmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const socks5_bindcmd_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _factory(factory), _proxy_addr(proxy_addr), _socks_user(socks_user), _socks_psw(socks_psw),
		tcp_client(reactor, proxy_addr, nullptr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

	virtual ~socks5_bindcmd_client()
	{
	}

private:
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		std::shared_ptr<socks5_bindcmd_client_channel>		origin_channel = std::make_shared<socks5_bindcmd_client_channel>(reactor, _proxy_addr, _server_addr, _socks_user, _socks_psw, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		std::shared_ptr<socks5_bindcmd_client_handler_base>	terminal_channel = _factory(reactor);
		build_channel_chain_helper(std::dynamic_pointer_cast<socks5_bindcmd_client_handler_base>(origin_channel), terminal_channel);
		origin_channel->ctrl_connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}

	std::string _proxy_addr;
	std::string _socks_user;
	std::string _socks_psw;
	socks5_bindcmd_client_channel_factory_t _factory;
};

///////////////////////////////////// assocate_cmd_mode

template<typename T>
class socks5_associatecmd_client
{
public:
	socks5_associatecmd_client(){}
	virtual ~socks5_associatecmd_client(){}
};

template <>
class socks5_associatecmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	socks5_associatecmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const udp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_client_channel_factory(factory),
		tcp_client(thread_num, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_associatecmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()	//原生channel
	{
		std::shared_ptr<socks5_associatecmd_client_channel>	channel = std::make_shared<socks5_associatecmd_client_channel>(nullptr, _server_addr, _socks_user, _socks_psw, _udp_client_channel_factory);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)	
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), terminal_channel);
		origin_channel->connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}
	
	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_client_channel_factory_t _udp_client_channel_factory;
};

template <>
class socks5_associatecmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_associatecmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const udp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_client_channel_factory(factory),
		tcp_client(reactor, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

	virtual ~socks5_associatecmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()	//原生channel
	{
		std::shared_ptr<socks5_associatecmd_client_channel>	channel = std::make_shared<socks5_associatecmd_client_channel>(nullptr, _server_addr, _socks_user, _socks_psw, _udp_client_channel_factory);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}
	virtual std::shared_ptr<chain_sharedobj_interface> build_channel_chain(std::shared_ptr<reactor_loop> reactor)	
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), terminal_channel);
		origin_channel->connect();

		return std::dynamic_pointer_cast<chain_sharedobj_interface>(origin_channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_client_channel_factory_t _udp_client_channel_factory;
};