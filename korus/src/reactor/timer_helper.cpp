#include <assert.h>
#include "timer_helper.h"
#include "reactor_loop.h"

timer_helper::timer_helper()
	: _reactor(nullptr), _task(nullptr), _is_start(false)
{
}

timer_helper::timer_helper(reactor_loop* reatcor)
	: _reactor(reatcor), _task(nullptr), _is_start(false)
{
}

timer_helper::~timer_helper()
{
	stop();
	clear();
	_reactor = nullptr;
}

void	timer_helper::reactor(reactor_loop* reatcor)
{
	_reactor = reatcor;
}

void	timer_helper::bind(reactor_timer_callback_t task)
{
	assert(task);
	_task = task;
}

void	timer_helper::clear()
{
	_task = nullptr;
}

void	timer_helper::start(const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval)
{
	if (!_task || !_reactor)
	{
		assert(false);
		return;
	}
	_is_start = true;
	_reactor->start_timer(this, begin, interval);
}

void	timer_helper::start(const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval)
{
	if (!_task || !_reactor)
	{
		assert(false);
		return;
	}
	_is_start = true;
	_reactor->start_timer(this, begin, interval);
}

void	timer_helper::stop()
{
	_is_start = false;
	if (_reactor)
	{
		_reactor->stop_timer(this);	
	}
}

bool	timer_helper::exist()
{
	return _is_start;// _reactor->exist_timer(this);
}

void	timer_helper::action()
{
	if (!_task || !_reactor)
	{
		assert(false);
		return;
	}

	_task(this);
}
