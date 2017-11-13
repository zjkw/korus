#pragma once

#include <functional>
#include "korus/src/util/chain_sharedobj_base.h"
#include "socks5_bindcmd_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

class socks5_bindcmd_integration_handler_base : public chain_sharedobj_base<socks5_bindcmd_integration_handler_base>
{
public:
	socks5_bindcmd_integration_handler_base();
	virtual ~socks5_bindcmd_integration_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();
	virtual void	chain_init(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl_channel, std::shared_ptr<socks5_bindcmd_client_channel> data_channel);
	virtual void	chain_final();	
	virtual void	chain_zomby();

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
	virtual int32_t on_ctrl_recv_split(const void* buf, const size_t len);	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)	
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t len);	//这是一个待处理的完整包

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
	virtual int32_t on_data_recv_split(const void* buf, const size_t len);	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual void	on_data_recv_pkg(const void* buf, const size_t len);	//这是一个待处理的完整包

private:		
	std::shared_ptr<reactor_loop>								_reactor;
	std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	_ctrl_channel;
	std::shared_ptr<socks5_bindcmd_client_channel>				_data_channel;
};

using socks5_bindcmd_integration_handler_factory_t = std::function<std::shared_ptr<socks5_bindcmd_integration_handler_base>()>;

bool build_relation(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl, std::shared_ptr<socks5_bindcmd_client_channel> data, std::shared_ptr<socks5_bindcmd_integration_handler_base> integration);

