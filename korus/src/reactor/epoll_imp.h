#pragma once

#include <vector>
#include "backend_poller.h"

class epoll_imp : public backend_poller
{
public:
	epoll_imp();
	virtual ~epoll_imp();

protected:
	virtual int32_t poll(int32_t mill_sec);
	// 初始新增
	virtual void add_sock(sockio_channel* channel, SOCKIO_TYPE type);
	// 存在更新
	virtual void upt_type(sockio_channel* channel, SOCKIO_TYPE type);
	// 删除
	virtual void del_sock(sockio_channel* channel);
	virtual void clear();

private:
	int								_fd;
	int32_t							_current_handler_index;//当前epoll_wait正在处理的索引，这里设定epoll_wait不存在重复的fd返回
	std::vector<struct epoll_event> _epoll_event;

	uint32_t	convert_event_flag(SOCKIO_TYPE type);
};

