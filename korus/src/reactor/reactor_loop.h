#pragma once

#include <memory>
#include <thread>
#include "korus/src/util/basic_defines.h"
#include "korus/src/util/task_queue.h"
#include "korus/src/util/object_state.h"
#include "backend_poller.h"
#include "timer_sheduler.h"
#include "idle_distributor.h"

class reactor_loop : public std::enable_shared_from_this<reactor_loop>, public double_state
{
public:
	reactor_loop();
	virtual ~reactor_loop();
	
	void	run();	
	void	run_once();

	bool	is_current_thread();

	// 如果不是所属线程，加入到其任务队列
	void	start_async_task(const async_task_t& task, const task_owner& owner);
	void	stop_async_task(const task_owner& owner);

	virtual void	invalid();

private:
	backend_poller*					_backend_poller;
	timer_sheduler*					_timer_sheduler;
	idle_distributor*				_idle_distributor;
	task_queue						_task_queue;
	std::thread::id					_tid;

	//为了避免外部程序直接调用下面的timer/idle系列函数导致的对象内部数据状态与管理器不一致，改成private，这要求外部只能用timer_helper/idle_helper而不能直接调用reactor_loop函数
	friend timer_helper;
	// begin表示起始时间，为0表示无，interval表示后续每步，为0表示无
	void	start_timer(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval);
	void	start_timer(timer_helper* timer_id, const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval);
	void	stop_timer(timer_helper* timer_id);
	bool	exist_timer(timer_helper* timer_id);
	friend idle_helper;
	void	start_idle(idle_helper* idle_id);
	void	stop_idle(idle_helper* idle_id);
	bool	exist_idle(idle_helper* idle_id);

	// poll后端，非线程安全，sockio_helper也仅仅限于内部使用
	friend	class sockio_helper;
	void start_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	void update_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	void stop_sockio(sockio_helper* sockio_id, SOCKET fd);

	void run_once_inner();
};
