#pragma once

#include "socks5_client_channel_base.h"

class socks5_bindcmd_client_handler_origin;

class socks5_bindcmd_filterbind_client_channel : public tcp_client_handler_base
{
public:
	socks5_bindcmd_filterbind_client_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_filterbind_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();

	virtual void	chain_inref();
	virtual void	chain_deref();

	void set_integration(socks5_bindcmd_client_handler_origin* integration) { _integration = integration; }

	virtual void	on_connected();
	virtual void	on_closed();
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const std::shared_ptr<buffer_thunk>& data);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data);
	
private:
	socks5_bindcmd_client_handler_origin* _integration;
};
