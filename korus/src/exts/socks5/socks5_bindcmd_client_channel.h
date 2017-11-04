#pragma once

#include "socks5_client_channel_base.h"

class socks5_bindcmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_bindcmd_client_channel();
	virtual ~socks5_bindcmd_client_channel();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_connected();
	virtual void	on_closed();
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len);
};
