#pragma once

#include "korus/src/exts/domain/domain_async_resolve_helper.h"
#include "socks5_client_channel_base.h"

// 逻辑时序
// 1，socks5_associatecmd_client_channel 向代理服务器执行associate_cmd操作，udp连接目标服务器

class socks5_associatecmd_server_channel
{
public:
	socks5_associatecmd_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_associatecmd_server_channel();
};
