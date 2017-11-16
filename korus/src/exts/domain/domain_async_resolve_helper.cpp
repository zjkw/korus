#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include "domain_cache_mgr.h"
#include "domain_async_resolve_helper.h"

int32_t	domain_async_resolve_helper::_channel_instance_num = 0;
std::mutex	domain_async_resolve_helper::_mutex;

// addr格式ip:port
domain_async_resolve_helper::domain_async_resolve_helper() : _created(false), _fds_num(0), _cb(nullptr)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_channel_instance_num++;
		if (_channel_instance_num == 1)
		{
			int32_t	status = ares_library_init(ARES_LIB_INIT_ALL);
			if (status != ARES_SUCCESS)
			{
				printf("ares_library_init: %s\n", ares_strerror(status));
			}
		}
	}
		
	_idle_helper.bind(std::bind(&domain_async_resolve_helper::on_idle_cmp, this, std::placeholders::_1));
	for (size_t i = 0; i < ARES_GETSOCK_MAXNUM; i++)
	{
		_sockio_helper[i].bind(std::bind(&domain_async_resolve_helper::on_sockio_read, this, std::placeholders::_1),
			std::bind(&domain_async_resolve_helper::on_sockio_write, this, std::placeholders::_1));
	}
}

domain_async_resolve_helper::domain_async_resolve_helper(reactor_loop* loop) : _created(false), _fds_num(0), _cb(nullptr), _idle_helper(loop)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_channel_instance_num++;
		if (_channel_instance_num == 1)
		{
			int32_t	status = ares_library_init(ARES_LIB_INIT_ALL);
			if (status != ARES_SUCCESS)
			{
				printf("ares_library_init: %s\n", ares_strerror(status));
			}
		}
	}

	_idle_helper.bind(std::bind(&domain_async_resolve_helper::on_idle_cmp, this, std::placeholders::_1));
	for (size_t i = 0; i < ARES_GETSOCK_MAXNUM; i++)
	{
		_sockio_helper[i].reactor(loop);
		_sockio_helper[i].bind(std::bind(&domain_async_resolve_helper::on_sockio_read, this, std::placeholders::_1),
			std::bind(&domain_async_resolve_helper::on_sockio_write, this, std::placeholders::_1));
	}
}

domain_async_resolve_helper::~domain_async_resolve_helper()
{
	{
		if (_created)
		{
			ares_cancel(_channel);
		}
		std::unique_lock<std::mutex> lck(_mutex);
		_channel_instance_num--;
		if (_channel_instance_num == 0)
		{
			ares_library_cleanup();
		}
	}

	for (size_t i = 0; i < _fds_num; i++)
	{
		_sockio_helper[i].stop();
		_sockio_helper[i].set(INVALID_SOCKET);
	}
}

void	domain_async_resolve_helper::reactor(reactor_loop* reatcor)
{
	_idle_helper.reactor(reatcor);
	for (size_t i = 0; i < _fds_num; i++)
	{
		_sockio_helper[i].reactor(reatcor);
	}
}

static void resolve_callback(void *arg, int status, int timeouts, struct hostent *host)
{
	if (!host || status != ARES_SUCCESS) 
	{
		printf("Failed to lookup %s\n", ares_strerror(status));
		return;
	}
	domain_async_resolve_helper* obj = (domain_async_resolve_helper*)arg;

//	printf("Found address name %s\n", host->h_name);
	std::vector<std::string>	iplist;
	char ip[INET6_ADDRSTRLEN];
	for (int32_t i = 0; host->h_addr_list[i]; ++i) 
	{
		inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
		iplist.push_back(ip);
	}
	obj->on_inner_resolve(host->h_name, iplist);
}

void	domain_async_resolve_helper::on_inner_resolve(const std::string& domain, const std::vector<std::string>& iplist)
{
	if (iplist.empty())
	{
		_cb(DRS_FAILED, domain, "");
		return;
	}

	update_domain_cache(domain, iplist);

	std::string ip;
	if (query_ip_by_domain(domain, ip))
	{
		_cb(DRS_SUCCESS, domain, ip);
		return;
	}

	_cb(DRS_FAILED, domain, "");
}

DOMAIN_RESOLVE_STATE	domain_async_resolve_helper::start(const std::string& domain, std::string& ip)
{
	//0, 到缓存中查询
	if (query_ip_by_domain(domain, ip))
	{
		return DRS_SUCCESS;
	}

	//1, 创建channel
	if (!_created)
	{
		int32_t	status = ares_init(&_channel);
		if (status != ARES_SUCCESS)
		{     // ares 对channel 进行初始化
			printf("ares_library_init: %s\n", ares_strerror(status));
			return DRS_FAILED;
		}
		_created = true;
	}
	else
	{
		for (size_t i = 0; i < _fds_num; i++)
		{
			_sockio_helper[i].stop();
			_sockio_helper[i].set(INVALID_SOCKET);
		}
		_fds_num = 0;
		ares_cancel(_channel);
	}

	//2, 执行查询
	ares_gethostbyname(_channel, domain.c_str(), AF_INET, resolve_callback, this);
	
	//3, 更新监听事件
	if (!_idle_helper.exist())
	{
		_idle_helper.start();
	}
	on_idle_cmp(&_idle_helper);

	return DRS_PENDING;
}

void	domain_async_resolve_helper::stop()
{
	_idle_helper.stop();
	for (size_t i = 0; i < _fds_num; i++)
	{
		_sockio_helper[i].stop();
	}
}

void	domain_async_resolve_helper::bind(const domain_async_resolve_callback_t& cb)
{
	_cb = cb;
}

void	domain_async_resolve_helper::on_sockio_read(sockio_helper* sockio_id)
{
	ares_process_fd(_channel, sockio_id->fd(), ARES_SOCKET_BAD);
}

void	domain_async_resolve_helper::on_sockio_write(sockio_helper* sockio_id)
{
	ares_process_fd(_channel, ARES_SOCKET_BAD, sockio_id->fd());
}

void	domain_async_resolve_helper::on_idle_cmp(idle_helper* idle_id)
{
	int fds[ARES_GETSOCK_MAXNUM];
	int fds_num = ares_getsock(_channel, fds, ARES_GETSOCK_MAXNUM);
	if (fds_num < 0)
	{
		//		printf("ares_getsock failed\n");
		return;
	}
	assert(fds_num <= ARES_GETSOCK_MAXNUM);

	for (int32_t j = 0; j < _fds_num; ++j)	// 先剔除过时的，为新fd腾出空间
	{
		if (INVALID_SOCKET == _sockio_helper[j].fd())
		{
			continue;
		}
		int32_t i = 0;
		for (; i < fds_num; ++i)
		{
			if (fds[i] == _sockio_helper[j].fd())
			{
				break;
			}
		}
		if (i >= fds_num)
		{
			_sockio_helper[j].stop();
			_sockio_helper[j].set(INVALID_SOCKET);
		}
	}
	for (int32_t i = 0; i < fds_num; ++i)
	{
		int32_t j = 0;
		for (; j < _fds_num; ++j)
		{
			if (fds[i] == _sockio_helper[j].fd())
			{
				break;
			}
		}
		if (j >= _fds_num)
		{
			//找到一个未用的helper
			int32_t k = 0;
			for (; k < ARES_GETSOCK_MAXNUM; ++k)
			{
				if (INVALID_SOCKET == _sockio_helper[k].fd())
				{
					_sockio_helper[k].set(fds[i]);
					_sockio_helper[k].start(SIT_READWRITE);
					if (_fds_num < k + 1)
					{
						_fds_num = k + 1;
					}

					break;
				}
			}
			assert(k < ARES_GETSOCK_MAXNUM);
		}
	}
}