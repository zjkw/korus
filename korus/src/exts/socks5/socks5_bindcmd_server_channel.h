#pragma once

#include <vector>
#include "korus/src/tcp/tcp_server.h"
#include "socks5_bindcmd_tunnel_server_channel.h"

class socks5_bindcmd_server_channel : public tcp_server_handler_terminal
{
public:
	socks5_bindcmd_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	virtual void	init(const std::string& addr, const std::string& listen_ip);

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t size);
	
	void	on_tunnel_accept(std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel);
	
private:	
	tcp_server<reactor_loop>*									_bindcmd_server;
	std::weak_ptr<socks5_bindcmd_tunnel_server_channel>			_bindcmd_tunnel_server_channel;	
	bool														_is_valid;//能否正常传输
	
	complex_ptr<tcp_server_handler_base> binccmd_channel_factory(std::shared_ptr<reactor_loop> reactor);
};