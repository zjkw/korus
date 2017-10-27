#include <stdlib.h>
#include <assert.h>

#include "reactor_loop.h"
#include "epoll_imp.h"
#include "kqueue_imp.h"

#include "timer_helper.h"
#include "idle_helper.h"


#define DEFAULT_POLL_WAIT_MILLSEC	(100)

reactor_loop::reactor_loop() : _task_queue(false, true)
{
	_tid = std::this_thread::get_id();

#if defined(__linux__) || defined(__linux)  
	_backend_poller = new epoll_imp;
//#elif defined(__APPLE__) && defined(__GNUC__)  
	//	_backend_poller = new kqueue_imp;
//#elif defined(__MACOSX__)  
//	_backend_poller = new kqueue_imp;
#else
	_backend_poller = nullptr;
	#error "platform unsupported"
#endif
	_timer_sheduler = new timer_sheduler;
	_idle_distributor = new idle_distributor;
}

reactor_loop::~reactor_loop()
{
	delete _idle_distributor;
	delete _timer_sheduler;
	delete _backend_poller;
}

void	reactor_loop::run()
{
	if (_tid != std::this_thread::get_id())
	{
		assert(false);
		return;
	}
	if (!is_valid())
	{
		return;
	}
	for (;;)
	{
		run_once_inner();
	}
}

void reactor_loop::run_once()
{
	if (_tid != std::this_thread::get_id())
	{
		assert(false);
		return;
	}
	if (!is_valid())
	{
		return;
	}
	run_once_inner();
}

void reactor_loop::run_once_inner()
{
	uint32_t	wait_millsec	= _timer_sheduler->min_interval().count();
	if (!wait_millsec || wait_millsec > DEFAULT_POLL_WAIT_MILLSEC)
	{
		wait_millsec = DEFAULT_POLL_WAIT_MILLSEC;
	}
	int32_t ret = _backend_poller->poll(wait_millsec);
	_timer_sheduler->tick();

	_task_queue.execute();

	_idle_distributor->idle();
}

void	reactor_loop::invalid()
{
	if (_tid != std::this_thread::get_id())
	{
		assert(false);
		return;
	}
	if (is_valid())
	{
		return;
	}

	thread_safe_objbase::invalid();
	_backend_poller->clear();
	_timer_sheduler->clear();
	_idle_distributor->clear();

	_task_queue.clear();
}

bool	reactor_loop::is_current_thread()
{
	return _tid == std::this_thread::get_id();
}

void	reactor_loop::start_async_task(const async_task_t& task, const task_owner& owner)
{
	if (!is_valid())
	{
		return;
	}
	_task_queue.add(task, owner);
}

void	reactor_loop::stop_async_task(const task_owner& owner)
{
	if (!is_valid())
	{
		return;
	}
	_task_queue.del(owner);
}

// begin表示起始时间，为0表示无，interval表示后续每步，为0表示无
void	reactor_loop::start_timer(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval)
{
	if (!is_valid())
	{
		return;
	}
	_timer_sheduler->start_timer(timer_id, begin, interval);
}

void	reactor_loop::start_timer(timer_helper* timer_id, const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval)
{
	if (!is_valid())
	{
		return;
	}
	_timer_sheduler->start_timer(timer_id, std::chrono::system_clock::now() + begin, interval);
}

void	reactor_loop::stop_timer(timer_helper* timer_id)
{
	if (!is_valid())
	{
		return;
	}
	_timer_sheduler->stop_timer(timer_id);
}

bool	reactor_loop::exist_timer(timer_helper* timer_id)
{
	if (!is_valid())
	{
		return false;
	}
	return _timer_sheduler->exist_timer(timer_id);
}

void	reactor_loop::start_idle(idle_helper* idle_id)
{
	if (!is_valid())
	{
		return;
	}
	_idle_distributor->start_idle(idle_id);
}

void	reactor_loop::stop_idle(idle_helper* idle_id)
{
	if (!is_valid())
	{
		return;
	}
	_idle_distributor->stop_idle(idle_id);
}

bool	reactor_loop::exist_idle(idle_helper* idle_id)
{
	if (!is_valid())
	{
		return false;
	}
	return _idle_distributor->exist_idle(idle_id);
}

void reactor_loop::start_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type)
{
	assert(_tid == std::this_thread::get_id());
	assert(is_valid());

	_backend_poller->add_sock(sockio_id, fd, type);
}

void reactor_loop::update_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type)
{
	assert(_tid == std::this_thread::get_id());
	assert(is_valid());

	_backend_poller->upt_type(sockio_id, fd, type);
}

void reactor_loop::stop_sockio(sockio_helper* sockio_id, SOCKET fd)
{
	assert(_tid == std::this_thread::get_id());
	assert(is_valid());

	_backend_poller->del_sock(sockio_id, fd);
}
