#include <sys/epoll.h>
#include "epoll_imp.h"

#define DEFAULT_EPOLL_SIZE	(2000)

epoll_imp::epoll_imp()
{
	_epoll_event.resize(DEFAULT_EPOLL_SIZE);
	_fd = epoll_create1(EPOLL_CLOEXEC);	//tbd 异常处理
}

epoll_imp::~epoll_imp()
{
	clear();
}

int32_t epoll_imp::poll(int32_t mill_sec)
{
	int32_t ret = epoll_wait(_fd, &_epoll_event[0], _epoll_event.size(), mill_sec);
	if (ret < 0)
	{
		if (EINTR != errno)
		{
			// tbd failed:
		}
	}
	else
	{
		for (int32_t i = 0; i < ret; ++i)
		{
			sockio_channel* channel = (channel)_epoll_event[i].data.ptr;
			assert(channel);

			if (_epoll_event[i].events & (EPOLLERR | EPOLLHUP))
			{			
				_epoll_event[i].events |= EPOLLIN | EPOLLOUT;
			}

#if 0
			if (_epoll_event[i].events & ~(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)) 
			{
				// log
			}
#endif
			// 对比nginx，我们处理[i]的时候需要考虑是否影响[i]后面的对象是否存在，而一个线程中的对象是否回收由listen的idle/server_client的生命期决定
			if (_epoll_event[i].events & (POLLERR | EPOLLIN)) 
			{	
				channel->on_sockio_read();
			}
			if (events & EPOLLOUT)
			{
				channel->on_sockio_write();
			}			
		}
		// 参考muduo扩容，下次poll将会有更多fd进来
		if (ret == (int32_t)_epoll_event.size())
		{
			_epoll_event.resize(_epoll_event.size() * 2);
		}
	}

	return ret;
}

// 初始新增
void epoll_imp::add_sock(sockio_channel* channel, SOCKIO_TYPE type)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = convert_event_flag(type);
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_ADD, fd, &ev);
}

// 存在更新
void epoll_imp::upt_type(sockio_channel* channel, SOCKIO_TYPE type)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = convert_event_flag(type);
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_MOD, fd, &ev);
}

// 删除
void epoll_imp::del_sock(sockio_channel* channel)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = 0;
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_MOD, fd, &ev);
}

void epoll_imp::clear()
{
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

inline uint32_t	epoll_imp::convert_event_flag(SOCKIO_TYPE type)
{
	// 默认ET模式，需要和recv/send结合使用
	uint32_t flag = EPOLLET;
	switch (type)
	{
	case SIT_NONE:
		assert(false);
		break;
	case SIT_READ:
		flag |= EPOLLIN;
		break;
	case SIT_WRITE:
		flag |= EPOLLOUT;
		break;
	case SIT_READWRITE:
		flag |= EPOLLIN | EPOLLOUT;
		break;
	default:
		assert(false);
		break;
	}

	return flag;
}