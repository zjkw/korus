#pragma once

#include <vector>
#include "korus/src//util/net_serialize.h"
#include "socks5_proto.h"
#include "korus/src/tcp/tcp_server.h"

class socks5_server_auth : public std::enable_shared_from_this<socks5_server_auth>
{
public:
	socks5_server_auth() {}
	~socks5_server_auth() {}
	virtual bool				check_userpsw(const std::string& user, const std::string& psw) = 0;
	virtual	SOCKS_METHOD_TYPE	select_method(const std::vector<SOCKS_METHOD_TYPE>&	method_list) = 0;
};

//中继角色，根据状态切换
class socks5_server_init_channel : public tcp_server_handler_base
{
public:
	socks5_server_init_channel(std::shared_ptr<reactor_loop> reactor, const std::string& bindcmd_tcp_listen_ip, const std::string& assocaitecmd_udp_listen_ip, std::shared_ptr<socks5_server_auth> auth);
	virtual ~socks5_server_init_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	virtual void	chain_inref();
	virtual void	chain_deref();

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const std::shared_ptr<buffer_thunk>& data);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data);
	
private:
	std::string							_bindcmd_tcp_listen_ip;	//bind cmd
	std::string							_assocaitecmd_udp_listen_ip;	//associate cmd for udp
	std::shared_ptr<socks5_server_auth>	_auth;

	bool send(const net_serialize&	codec);

	enum SOCKS_SERVER_STATE
	{
		SSS_NONE = 0,
		SSS_METHOD = 1,		
		SSS_AUTH = 2,
		SSS_TUNNEL = 3,	
		SSS_NORMAL = 4,
	};
	SOCKS_SERVER_STATE	_shakehand_state;
};