#pragma once

#include <vector>
#include "korus/src/tcp/tcp_server.h"
#include "socks5_connectcmd_tunnel_client_channel.h"

class socks5_connectcmd_server_channel : public tcp_server_handler_terminal
{
public:
	socks5_connectcmd_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_connectcmd_server_channel();

	void	init(const std::string& addr);

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t size);

	void			on_tunnel_connect(const std::string& addr);
	
private:
	std::shared_ptr<socks5_connectcmd_tunnel_client_channel>		_connectcmd_tunnel_client_channel;
	bool			_tunnel_valid;//仅仅当收到on_tunnel_connect触发
};