#include <assert.h>
#include "sockio_helper.h"
#include "reactor_loop.h"

sockio_helper::sockio_helper()
	: _reactor(nullptr), _read(nullptr), _write(nullptr), _type(SIT_NONE), _fd(INVALID_SOCKET)
{
}

sockio_helper::sockio_helper(reactor_loop* reatcor)
	: _reactor(reatcor), _read(nullptr), _write(nullptr), _type(SIT_NONE), _fd(INVALID_SOCKET)
{
}

sockio_helper:: ~sockio_helper()
{
	clear();
}

void	sockio_helper::reactor(reactor_loop* reatcor)
{
	assert(!_reactor);
	_reactor = reatcor;
}

void	sockio_helper::set(SOCKET fd)
{
	if (fd == _fd)
	{
		return;
	}
	if (INVALID_SOCKET != _fd)
	{
		stop();		
	}
	_fd = fd;
	if (INVALID_SOCKET != _fd)
	{
		start(_type);
	}
}

void	sockio_helper::bind(const reactor_sockio_callback_t read, const reactor_sockio_callback_t write)
{
	assert(read || write);	//不能同时为空

	_read = read;
	_write = write;
}

void	sockio_helper::clear()
{
	if (INVALID_SOCKET != _fd)
	{
		stop();
		_fd = INVALID_SOCKET;
	}

	_read = nullptr;
	_write = nullptr;
	_reactor = nullptr;
}

void	sockio_helper::start(SOCKIO_TYPE type)
{
	if (SIT_NONE == type)
	{
		return;
	}
	if (type == _type)
	{
		return;
	}
	if (INVALID_SOCKET == _fd)
	{
		assert(false);
		return;
	}

	if (SIT_NONE == _type)
	{
		_reactor->start_sockio(this, _fd, type);
	}
	else
	{
		_reactor->update_sockio(this, _fd, type);
	}
	_type = type;
}

void	sockio_helper::stop()
{
	if (SIT_NONE == _type)
	{
		return;
	}
	if (INVALID_SOCKET == _fd)
	{
		return;
	}
	if (_reactor)
	{
		_reactor->stop_sockio(this, _fd);
	}
	_type = SIT_NONE;
}

inline bool	sockio_helper::exist()
{
	return SIT_NONE != _type;
}

void	sockio_helper::action()
{
	if (!exist())
	{
		return;
	}

	switch (_type)
	{
	case SIT_READ:
		if (_read)
		{
			_read(this);
		}
		break;
	case SIT_WRITE:
		if (_write)
		{
			_write(this);
		}
		break;
	case SIT_READWRITE:
		if (_read)
		{
			_read(this);
		}
		if (exist())	//避免_read处理过程中注销了epoll
		{
			if (_write)
			{
				_write(this);
			}
		}
		break;
	case SIT_NONE:
	default:
		//理论上存在
		//assert(false);	
		break;
	}
}
