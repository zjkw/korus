#include <assert.h>
#include "idle_helper.h"
#include "reactor_loop.h"

idle_helper::idle_helper(reactor_loop* reatcor)
	: _reactor(reatcor), _task(nullptr), _is_start(false)
{
}

idle_helper::idle_helper()
	: _reactor(nullptr), _task(nullptr), _is_start(false)
{
}

idle_helper::~idle_helper()
{
	stop();
	clear();
	_reactor = nullptr;
}

void	idle_helper::reactor(reactor_loop* reatcor)
{
	assert(!_reactor);
	_reactor = reatcor;
}

void	idle_helper::bind(reactor_idle_callback_t task)
{
	assert(task);
	_task = task;
}

void	idle_helper::clear()
{
	_task = nullptr;
}

void	idle_helper::start()
{
	if (!_task || !_reactor)
	{
		assert(false);
		return;
	}
	if (!_is_start)
	{
		_is_start = true;
		_reactor->start_idle(this);
	}
}

void	idle_helper::stop()
{
	if (_is_start)
	{
		_is_start = false;
		if (_reactor)
		{
			_reactor->stop_idle(this);
		}
	}
}

bool	idle_helper::exist()
{
	return 	_is_start;// _reactor->exist_idle(this);
}

void	idle_helper::action()
{
	if (!_task || !_reactor)
	{
		assert(false);
		return;
	}

	_task(this);
}