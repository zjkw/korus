#pragma once

#include <functional>
#include "korus/src/util/basic_defines.h"

class sockio_helper;
using reactor_sockio_callback_t = std::function<void(sockio_helper*)>;

enum SOCKIO_TYPE
{
	SIT_NONE = 0,
	SIT_READ = 1,
	SIT_WRITE = 2,
	SIT_READWRITE = 3
};

class reactor_loop;

// 非线程安全，用户不得直接使用
class sockio_helper
{
public:
	sockio_helper();
	sockio_helper(reactor_loop* reatcor);
	virtual ~sockio_helper();

	void	reactor(reactor_loop* reatcor);
	void	set(SOCKET fd);
	void	bind(const reactor_sockio_callback_t read, const reactor_sockio_callback_t write);
	void	clear();
	void	start(SOCKIO_TYPE type);
	void	stop();
	bool	exist();
	void	action();

private:
	reactor_loop*				_reactor;
	reactor_sockio_callback_t	_read;
	reactor_sockio_callback_t	_write;
	SOCKIO_TYPE					_type;
	SOCKET						_fd;
};

