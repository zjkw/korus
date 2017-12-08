#pragma once

#include <vector>
#include "backend_poller.h"

// tbd tcp/udp流控，recv受限于send等，并模拟边沿触发------

class epoll_imp : public backend_poller
{
public:
	epoll_imp();
	virtual ~epoll_imp();

protected:
	virtual int32_t poll(int32_t mill_sec);
	// 初始新增
	virtual void add_sock(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	// 存在更新
	virtual void upt_type(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	// 删除
	virtual void del_sock(sockio_helper* sockio_id, SOCKET fd);
	virtual void clear();

private:
	int								_fd;
	std::vector<struct epoll_event> _epoll_event;

	uint32_t	convert_event_flag(SOCKIO_TYPE type);
};

