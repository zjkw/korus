#include <assert.h>
#include <functional>
#include <assert.h>
#include "korus/src/util/socket_ops.h"
#include "domain_cache_mgr.h"
#include "tcp_client_channel_domain.h"

/////////////////base
tcp_client_channel_domain::tcp_client_channel_domain(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::chrono::seconds connect_timeout/* = std::chrono::seconds(0)*/, std::chrono::seconds connect_retry_wait/* = std::chrono::seconds(-1)*/,
	const uint32_t self_read_size/* = DEFAULT_READ_BUFSIZE*/, const uint32_t self_write_size/* = DEFAULT_WRITE_BUFSIZE*/, const uint32_t sock_read_size/* = 0*/, const uint32_t sock_write_size/* = 0*/)
	: _server_addr_tmp(server_addr), tcp_client_handler_origin(reactor, server_addr, connect_timeout, connect_retry_wait,	self_read_size, self_write_size, sock_read_size, sock_write_size)
{
	_resolve.bind(std::bind(&tcp_client_channel_domain::on_resolve_result, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

tcp_client_channel_domain::~tcp_client_channel_domain()
{ 
}

void	tcp_client_channel_domain::connect()
{
	std::string host, port;
	if (!sockaddr_from_string(_server_addr_tmp, host, port))
	{
		assert(false);
		return;
	}

	SOCK_ADDR_TYPE	sat = addrtype_from_string(_server_addr_tmp);
	if (sat == SAT_DOMAIN)
	{
		std::string ip;
		if (query_ip_by_domain(host, ip))
		{
			// 就用原来的
			std::string addr;
			if (!sockaddr_from_string(addr, ip, port))
			{
				assert(false);
				return;
			}
			server_addr(addr);
			tcp_client_channel_domain::connect();
		}
		else
		{
			DOMAIN_RESOLVE_STATE state = _resolve.start(host, ip);	//无视成功，强制更新
			if (DRS_SUCCESS == state)
			{
				// 就用原来的
				std::string addr;
				if (!sockaddr_from_string(addr, ip, port))
				{
					assert(false);
					return;
				}
				server_addr(addr);
				tcp_client_channel_domain::connect();
			}
			else if (DRS_PENDING == state)
			{
				
			}
			else
			{
				
			}
		}
	}
	else if (sat == SAT_IPV4)
	{
		tcp_client_channel_domain::connect();
	}
	else
	{
		assert(false);
		return;
	}
}

void	tcp_client_channel_domain::on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
{
	if (result == DRS_SUCCESS)
	{
		std::string addr, host, port;
		if (!sockaddr_from_string(addr, host, port))
		{
			assert(false);
			return;
		}
		printf("on_resolve_result result: %d, domain: %s, ip: %s\n", (int)result, domain.c_str(), ip.c_str());
		server_addr(addr);
		tcp_client_channel_domain::connect();
	}
	else
	{
		//tbd...
	}
}
