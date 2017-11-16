#pragma once

//域名解析封装

#include <unistd.h>
#include <string>
#include <vector>
#include <mutex>
#include "ares.h"
#include "korus/src/reactor/sockio_helper.h"
#include "korus/src/reactor/idle_helper.h"

enum DOMAIN_RESOLVE_STATE
{
	DRS_NONE = 0,
	DRS_SUCCESS = 1,
	DRS_FAILED = 2,
	DRS_PENDING = 3,
};
using domain_async_resolve_callback_t = std::function<void(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)>;

class domain_async_resolve_helper
{
public:
	// addr格式ip:port
	domain_async_resolve_helper();
	domain_async_resolve_helper(reactor_loop* loop);
	virtual ~domain_async_resolve_helper();
	void					reactor(reactor_loop* reatcor);

	DOMAIN_RESOLVE_STATE	start(const std::string& domain, std::string& ip);
	void					stop();
	void					bind(const domain_async_resolve_callback_t& cb);
	
	void					on_inner_resolve(const std::string& domain, const std::vector<std::string>& iplist);
private:
	static	int32_t			_channel_instance_num;
	static	std::mutex		_mutex;

	domain_async_resolve_callback_t _cb;

	bool					_created;
	ares_channel			_channel;	
	int32_t					_fds_num;
	sockio_helper			_sockio_helper[ARES_GETSOCK_MAXNUM];
	virtual void			on_sockio_read(sockio_helper* sockio_id);
	virtual void			on_sockio_write(sockio_helper* sockio_id);

	idle_helper	_idle_helper;
	void on_idle_cmp(idle_helper* idle_id);
};