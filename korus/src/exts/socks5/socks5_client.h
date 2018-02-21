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
		_server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{
	}
	virtual ~socks5_connectcmd_client()
	{
	}

private:
	virtual complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*					origin_channel = create_origin_channel(reactor);
		socks5_connectcmd_client_channel*	sock5_channel = new socks5_connectcmd_client_channel(reactor, _server_addr, _socks_user, _socks_psw);
		complex_ptr<tcp_client_handler_base>			terminal_channel = create_terminal_channel(reactor);
		
		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)sock5_channel, (tcp_client_handler_base*)terminal_channel.get());
		origin_channel->connect();

		return terminal_channel;
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
		_server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{

	}

	virtual ~socks5_connectcmd_client()
	{
	}
private:
	virtual complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*					origin_channel = create_origin_channel(reactor);
		socks5_connectcmd_client_channel*	sock5_channel = new socks5_connectcmd_client_channel(reactor, _server_addr, _socks_user, _socks_psw);
		complex_ptr<tcp_client_handler_base>			terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)sock5_channel, (tcp_client_handler_base*)terminal_channel.get());
		origin_channel->connect();

		return terminal_channel;
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
	virtual void thread_init(thread_object*	thread_obj)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		complex_ptr<socks5_bindcmd_client_handler_base>	terminal_channel = build_channel_chain2(reactor);

		thread_obj->add_exit_task(std::bind(&socks5_bindcmd_client::thread_exit2, this, thread_obj, reactor, terminal_channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit2(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, complex_ptr<socks5_bindcmd_client_handler_base>	terminal_channel)
	{
		reactor->invalid();
	}
	complex_ptr<socks5_bindcmd_client_handler_base> build_channel_chain2(std::shared_ptr<reactor_loop> reactor)
	{
		socks5_bindcmd_client_handler_origin*		origin_channel = new socks5_bindcmd_client_handler_origin(reactor, _proxy_addr, _server_addr, _socks_user, _socks_psw, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		complex_ptr<socks5_bindcmd_client_handler_base>	terminal_channel = _factory(reactor);
		build_channel_chain_helper((socks5_bindcmd_client_handler_base*)origin_channel, (socks5_bindcmd_client_handler_base*)terminal_channel.get());
		origin_channel->ctrl_connect();

		return terminal_channel;
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
	virtual void init_object(std::shared_ptr<reactor_loop> reactor)
	{
		_socks5_channel = build_channel_chain2(_reactor);
	}
	complex_ptr<socks5_bindcmd_client_handler_base> build_channel_chain2(std::shared_ptr<reactor_loop> reactor)
	{
		socks5_bindcmd_client_handler_origin*		origin_channel = new socks5_bindcmd_client_handler_origin(reactor, _proxy_addr, _server_addr, _socks_user, _socks_psw, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		complex_ptr<socks5_bindcmd_client_handler_base>	terminal_channel = _factory(reactor);
		build_channel_chain_helper((socks5_bindcmd_client_handler_base*)origin_channel, (socks5_bindcmd_client_handler_base*)terminal_channel.get());
		origin_channel->ctrl_connect();

		return terminal_channel;
	}

	std::string _proxy_addr;
	std::string _socks_user;
	std::string _socks_psw;
	socks5_bindcmd_client_channel_factory_t _factory;

	complex_ptr<socks5_bindcmd_client_handler_base>	_socks5_channel;
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
	socks5_associatecmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const udp_server_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_server_channel_factory(factory),
		tcp_client(thread_num, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this, std::placeholders::_1), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_associatecmd_client()
	{
	}

private:
	complex_ptr<tcp_client_handler_base>	socks5_channel_factory(std::shared_ptr<reactor_loop> reactor)	//原生channel
	{
		//哨兵，严格说是可以不要的，socks5_associatecmd_client_channel改造成terminal，但由于改动太大，故此
		std::shared_ptr<tcp_client_handler_terminal> handler = std::make_shared<tcp_client_handler_terminal>(reactor);
		std::shared_ptr<tcp_client_handler_base> cb = std::dynamic_pointer_cast<tcp_client_handler_base>(handler);
		return cb;
	}
	virtual complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel = create_origin_channel(reactor);
		socks5_associatecmd_client_channel*	sock5_channel = new socks5_associatecmd_client_channel(reactor, _socks_user, _socks_psw, _udp_server_channel_factory);
		complex_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)sock5_channel, (tcp_client_handler_base*)terminal_channel.get());
		origin_channel->connect();

		return terminal_channel;
	}
	
	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_server_channel_factory_t _udp_server_channel_factory;
};

template <>
class socks5_associatecmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_associatecmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const udp_server_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_server_channel_factory(factory),
		tcp_client(reactor, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this, std::placeholders::_1), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

	virtual ~socks5_associatecmd_client()
	{
	}

private:
	complex_ptr<tcp_client_handler_base>	socks5_channel_factory(std::shared_ptr<reactor_loop> reactor)	//原生channel
	{
		//哨兵，严格说是可以不要的，socks5_associatecmd_client_channel改造成terminal，但由于改动太大，故此
		std::shared_ptr<tcp_client_handler_terminal> handler = std::make_shared<tcp_client_handler_terminal>(reactor);
		std::shared_ptr<tcp_client_handler_base> cb = std::dynamic_pointer_cast<tcp_client_handler_base>(handler);
		return cb;
	}
	virtual complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel = create_origin_channel(reactor);
		socks5_associatecmd_client_channel*	sock5_channel = new socks5_associatecmd_client_channel(reactor, _socks_user, _socks_psw, _udp_server_channel_factory);
		complex_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)sock5_channel, (tcp_client_handler_base*)terminal_channel.get());

		origin_channel->connect();

		return terminal_channel;
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_server_channel_factory_t _udp_server_channel_factory;
};