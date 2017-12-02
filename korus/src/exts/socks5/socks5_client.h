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
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(origin_channel);

		std::shared_ptr<socks5_connectcmd_client_channel>	sock5_channel = std::make_shared<socks5_connectcmd_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(sock5_channel), terminal_channel);

		origin_channel->connect();
		return true;
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
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(origin_channel);

		std::shared_ptr<socks5_connectcmd_client_channel>	sock5_channel = std::make_shared<socks5_connectcmd_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(sock5_channel), terminal_channel);

		origin_channel->connect();
		return true;
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
	// 设计考虑：其实外层也可以单独connect + bind进行组合操作，但是因为模板固话了多线程，所以一旦这么做则用户层需要将两个连接配对，这增加了管理和参数传递的麻烦，并且时序（先connect后bind）无法得到保证
	socks5_bindcmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const socks5_bindcmd_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _factory(factory), _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw),
		tcp_client(thread_num, proxy_addr, nullptr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_bindcmd_client()
	{
	}

private:
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		// ctrl channel
		std::shared_ptr<tcp_client_channel>			ctrl_origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(ctrl_origin_channel);
		std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	ctrl_socks_channel = std::make_shared<socks5_connectcmd_embedbind_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);
		
		// data channel
		std::shared_ptr<tcp_client_channel>			data_origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(data_origin_channel);
		std::shared_ptr<socks5_bindcmd_originalbind_client_channel>	data_socks_channel = std::make_shared<socks5_bindcmd_originalbind_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);;
		
		std::shared_ptr<socks5_bindcmd_client_channel>	terminal_channel = std::make_shared<socks5_bindcmd_client_channel>();

		build_relation(ctrl_socks_channel, data_socks_channel, terminal_channel);
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(ctrl_origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(ctrl_socks_channel));
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(data_origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(data_socks_channel));

		terminal_channel->on_chain_init();
		
		ctrl_origin_channel->connect();
		//data_origin_channel->connect();

		return true;
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	socks5_bindcmd_client_channel_factory_t _factory;
};

template <>
class socks5_bindcmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	socks5_bindcmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& ctrl_channel_factory, const tcp_client_channel_factory_t& data_channel_factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _ctrl_channel_factory(ctrl_channel_factory), _data_channel_factory(data_channel_factory), _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), 
		tcp_client(reactor, proxy_addr, nullptr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

	virtual ~socks5_bindcmd_client()
	{
	}

private:
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		// ctrl channel
		std::shared_ptr<tcp_client_channel>			ctrl_origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(ctrl_origin_channel);
		std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	ctrl_socks_channel = std::make_shared<socks5_connectcmd_embedbind_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);

		// data channel
		std::shared_ptr<tcp_client_channel>			data_origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(data_origin_channel);
		std::shared_ptr<socks5_bindcmd_originalbind_client_channel>	data_socks_channel = std::make_shared<socks5_bindcmd_originalbind_client_channel>(reactor, _server_addr, _socks_user, _socks_psw);;

		std::shared_ptr<socks5_bindcmd_client_channel>	terminal_channel = std::make_shared<socks5_bindcmd_client_channel>();

		build_relation(ctrl_socks_channel, data_socks_channel, terminal_channel);
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(ctrl_origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(ctrl_socks_channel));
		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(data_origin_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(data_socks_channel));

		terminal_channel->on_chain_init();

		ctrl_origin_channel->connect();
		//data_origin_channel->connect();

		return true;
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	tcp_client_channel_factory_t _ctrl_channel_factory;
	tcp_client_channel_factory_t _data_channel_factory;
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
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(origin_channel);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), terminal_channel);

		origin_channel->connect();
		return true;
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
	virtual bool build_channel_chain(std::shared_ptr<reactor_loop> reactor, std::list<std::shared_ptr<tcp_client_channel>>& origin_channel_list)	//存在一次性构建多个origin_channel
	{
		std::shared_ptr<tcp_client_channel>			origin_channel = create_origin_channel(reactor);
		origin_channel_list.push_back(origin_channel);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);

		build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(origin_channel), terminal_channel);

		origin_channel->connect();
		return true;
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_client_channel_factory_t _udp_client_channel_factory;
};