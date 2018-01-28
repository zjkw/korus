#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include "korus/src/util/socket_ops.h"
#include "korus/src/tcp/tcp_client_channel.h"
#include "domain_async_resolve_helper.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class tcp_client_channel_domain : public tcp_client_handler_origin
{
public:
	tcp_client_channel_domain(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0);
	virtual ~tcp_client_channel_domain();

	virtual void		connect();

private:
	std::string						_server_addr_tmp;
	domain_async_resolve_helper		_resolve;
	
	void	on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip);
};
