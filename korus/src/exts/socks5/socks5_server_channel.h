#pragma once

#include <vector>
#include "korus/src/tcp/tcp_server_channel.h"
#include "socks5_connectcmd_tunnel_client_channel.h"
#include "socks5_bindcmd_tunnel_server_channel.h"
#include "socks5_associatecmd_server_channel.h"

//connectcmd:
//	source_client													socks5_proxy												target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|构造socks5_server_channel									|					
//		|																|<---															|					
//		|																|----构造socks5_connectcmd_tunnel_client_channel，并connect---->|					
//		|																|----															|					
//		|																|	|connect结果												|					
//		|																|<---															|					
//		|<----------------------tunnel_ack------------------------------|																|					
//		|																|																|		
//		|-----------------------data_trans----------------------------->|																|
//		|																|----															|					
//		|																|	|socks5_server_channel接收									|					
//		|																|<---															|		
//		|																|----socks5_connectcmd_tunnel_client_channel	data_trans----->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans-------------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_connectcmd_tunnel_client_channel接收				|					
//		|																|<---															|		
//		|<----------------------socks5_server_channel	data_trans------|																|

//connectcmd:(其中connectcmd如上，下面仅为实际数据通道的bindcmd)
//	source_client													socks5_proxy												target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|构造socks5_server_channel									|					
//		|																|<---															|					
//		|																|----															|					
//		|																|	|构造socks5_bindcmd_tunnel_listen，执行listen				|					
//		|																|<---															|	
//		|<----------------------tunnel_ack_1----------------------------|																|	
//		|																|<---主动connect	socks5_bindcmd_tunnel_listen----------------|	
//		|																|----															|					
//		|																|	|构造socks5_bindcmd_tunnel_server_channel					|					
//		|																|<---															|					
//		|<----------------------tunnel_ack_2----------------------------|																|					
//		|																|																|		
//		|-----------------------data_trans----------------------------->|																|
//		|																|----															|					
//		|																|	|socks5_server_channel接收									|					
//		|																|<---															|		
//		|																|----socks5_bindcmd_tunnel_server_channel	data_trans--------->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans-------------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_bindcmd_tunnel_server_channel接收					|					
//		|																|<---															|		
//		|<----------------------socks5_server_channel	data_trans------|																|

//associatecmd:
//	source_client													socks5_proxy												target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|构造socks5_server_channel									|					
//		|																|<---															|					
//		|																|----															|					
//		|																|	|构造socks5_associatecmd_server_channel，执行bind			|					
//		|																|<---															|	
//		|<----------------------tunnel_ack------------------------------|																|					
//		|																|																|					
//		|-----------------------data_trans(udp)------------------------>|																|
//		|																|----															|					
//		|																|	|socks5_associatecmd_server_channel接收						|					
//		|																|<---															|		
//		|																|----socks5_associatecmd_server_channel		data_trans(udp)---->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans(udp)--------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_associatecmd_server_channel接收						|					
//		|																|<---															|		
//		|<-------socks5_associatecmd_server_channel	data_trans(udp)-----|																|

class socks5_server_auth
{
public:
	socks5_server_auth() {}
	~socks5_server_auth() {}
	virtual bool				is_valid(const std::string& user, const std::string& psw) = 0;
	virtual	SOCKS_METHOD_TYPE	select_method(const std::vector<SOCKS_METHOD_TYPE>&	method_list) = 0;

};


class socks5_server_channel : public tcp_server_handler_base
{
public:
	socks5_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_auth> auth);
	virtual ~socks5_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();
	virtual void	on_accept();
	virtual void	on_closed();

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t size);

private:
	std::shared_ptr<socks5_server_auth>	_auth;

	enum SOCKS_SERVER_STATE
	{
		SSS_NONE = 0,
		SSS_METHOD = 1,		
		SSS_AUTH = 2,
		SSS_TUNNEL = 3,
	};
	SOCKS_SERVER_STATE	_shakehand_state;
	enum TUNNEL_CHANNEL_TYPE
	{
		TCT_NONE = 0,
		TCT_CONNECT = 1,
		TCT_BIND = 2,
		TCT_ASSOCIATE = 3
	};
	TUNNEL_CHANNEL_TYPE		_tunnel_channel_type;
	std::shared_ptr<socks5_connectcmd_tunnel_client_channel>	_connectcmd_tunnel_client_channel;	//仅当_tunnel_channel_type = TCT_CONNECT有效：控制生命期和数据转发
	std::shared_ptr<socks5_bindcmd_tunnel_server_channel>		_bindcmd_tunnel_server_channel;		//仅当_tunnel_channel_type = TCT_BIND有效：控制生命期和数据转发
	std::shared_ptr<socks5_associatecmd_server_channel>			_associatecmd_server_channel;		//仅当_tunnel_channel_type = TCT_ASSOCIATE有效：仅仅控制生命期
};