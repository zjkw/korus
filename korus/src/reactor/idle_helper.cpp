#include <assert.h>
#include "idle_helper.h"
#include "reactor_loop.h"

idle_helper::idle_helper(reactor_loop* reatcor)
	: _reactor(reatcor), _task(nullptr), _is_start(false)
{
}

idle_helper::~idle_helper()
{
	stop();
	clear();
	_reactor = nullptr;
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
	if (!_task)
	{
		assert(false);
		return;
	}
	_is_start = true;
	_reactor->start_idle(this);
}

void	idle_helper::stop()
{
	_is_start = false;
	_reactor->stop_idle(this);
}

bool	idle_helper::exist()
{
	return 	_is_start;// _reactor->exist_idle(this);
}

void	idle_helper::action()
{
	if (!_task)
	{
		assert(false);
		return;
	}

	_task(this);
}