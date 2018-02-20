#pragma once

#include <functional>
#include "korus/src/util/chain_object.h"
#include "socks5_bindcmd_originalbind_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

// 逻辑时序
// 1，socks5_connectcmd_embedbind_client_channel 向代理服务器执行connect_cmd操作，tcp连接目标服务器
// 2，socks5_bindcmd_originalbind_client_channel 向代理服务器请求bind操作，返回代理监听（目标服务器的connect）的ip和端口 X
// 3，socks5_connectcmd_embedbind_client_channel 发送请求，要求目标服务器连接 X
// 4，代理服务器返回 目标服务器连接的ip和端口

class socks5_bindcmd_client_handler_base : public chain_object_linkbase<socks5_bindcmd_client_handler_base>, public obj_refbase<socks5_bindcmd_client_handler_base>
{
public:
	socks5_bindcmd_client_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_client_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

	//ctrl channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	ctrl_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	ctrl_close();
	virtual void	ctrl_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	ctrl_connect();
	virtual TCP_CLTCONN_STATE	ctrl_state();
	virtual void	on_ctrl_connected();
	virtual void	on_ctrl_closed();
	CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code);				//参考CHANNEL_ERROR_CODE定义	
	virtual int32_t on_ctrl_recv_split(const void* buf, const size_t size);
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t size);	//这是一个待处理的完整包

	//data channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	data_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	data_close();
	virtual void	data_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	data_connect();
	virtual TCP_CLTCONN_STATE	data_state();
	virtual void	on_data_connected();
	virtual void	on_data_closed();
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code);		//参考CHANNEL_ERROR_CODE定义
	virtual int32_t on_data_recv_split(const void* buf, const size_t size);
	virtual void	on_data_recv_pkg(const void* buf, const size_t size);	//这是一个待处理的完整包
	virtual void	on_data_bindcmd_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);		//代理服务器用于监听“目标服务器过来的连接”地址
	
private:
	std::shared_ptr<reactor_loop>		_reactor;
};

class socks5_bindcmd_client_handler_origin : public socks5_bindcmd_client_handler_base
{
public:
	socks5_bindcmd_client_handler_origin(std::shared_ptr<reactor_loop>	reactor, const std::string& proxy_addr, const std::string& server_addr,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// 如果账号为空，将忽略密码，认为是无需鉴权
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0);
	virtual ~socks5_bindcmd_client_handler_origin();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	chain_final();

	//ctrl channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	ctrl_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	ctrl_close();
	virtual void	ctrl_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	ctrl_connect();
	virtual TCP_CLTCONN_STATE	ctrl_state();

	virtual void	on_ctrl_connected();
	virtual void	on_ctrl_closed();	
	CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code);				//参考CHANNEL_ERROR_CODE定义	

	//data channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	data_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	data_close();
	virtual void	data_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	data_connect();
	virtual TCP_CLTCONN_STATE	data_state();

	virtual void	on_data_connected();
	virtual void	on_data_closed();
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code);		//参考CHANNEL_ERROR_CODE定义

	virtual void	on_data_bindcmd_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);		//代理服务器用于监听“目标服务器过来的连接”地址

private:		
	socks5_connectcmd_embedbind_client_channel*	_ctrl_channel;
	socks5_bindcmd_originalbind_client_channel*	_data_channel;
};

class socks5_bindcmd_client_handler_terminal : public socks5_bindcmd_client_handler_base, public std::enable_shared_from_this<socks5_bindcmd_client_handler_terminal>
{
public:
	socks5_bindcmd_client_handler_terminal(std::shared_ptr<reactor_loop>	reactor);
	virtual ~socks5_bindcmd_client_handler_terminal();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

	//ctrl channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	ctrl_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	ctrl_close();
	virtual void	ctrl_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	ctrl_connect();
	virtual TCP_CLTCONN_STATE	ctrl_state();

	virtual void	on_ctrl_connected();
	virtual void	on_ctrl_closed();
	CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code);				//参考CHANNEL_ERROR_CODE定义	

	//data channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual int32_t	data_send(const void* buf, const size_t len);			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void	data_close();
	virtual void	data_shutdown(int32_t howto);							// 参数参考全局函数 ::shutdown
	virtual void	data_connect();
	virtual TCP_CLTCONN_STATE	data_state();

	virtual void	on_data_connected();
	virtual void	on_data_closed();
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code);		//参考CHANNEL_ERROR_CODE定义

	virtual void	on_data_bindcmd_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);		//代理服务器用于监听“目标服务器过来的连接”地址
	
	//override------------------
	virtual void	chain_inref();
	virtual void	chain_deref();

protected:
	virtual void	on_release();
};

using socks5_bindcmd_client_channel_factory_t = std::function<complex_ptr<socks5_bindcmd_client_handler_base>(std::shared_ptr<reactor_loop>)>;


