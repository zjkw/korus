#pragma once

#include "socks5_proto.h"
#include "korus/src/tcp/tcp_client_channel.h"
#include "korus/src/reactor/timer_helper.h"

class socks5_tcp_client_channel : public tcp_client_handler_base, public multiform_state
{
public:
	socks5_tcp_client_channel(const std::map<SOCKS_METHOD_TYPE, SOCKS_METHOD_DATA>& method_list);
	virtual ~socks5_tcp_client_channel();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_connected();
	virtual void	on_closed();
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len);

private:
	std::map<SOCKS_METHOD_TYPE, SOCKS_METHOD_DATA>	_method_list;

	SOCKS_CLIENT_STATE	_socks5_state;
	int32_t				send_method();
	int32_t				send_auth(SOCKS_METHOD_TYPE type);

	// 为了避免send失败，我们将执行重发
	timer_helper		_re_sender_timer;
	void				on_re_sender_timer(timer_helper*);
};
