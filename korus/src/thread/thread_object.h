#pragma once

#include <assert.h>
#include <condition_variable>
#include <thread>
#include <mutex>

#include "korus/src/util/object_state.h"
#include "korus/src/util/task_queue.h"

//支持常驻任务队列(不提供显式撤销接口)
//支持一次性任务队列
//thread_object的创建和销毁必须在同一个线程

class thread_object : public noncopyable
{
public:
	explicit thread_object(uint16_t index);
	virtual ~thread_object();

	//线程常驻和临时任务
	void add_resident_task(const async_task_t& task)
	{
		add_task_helper(&_resident_task_queue, task);
	}
	void add_disposible_task(const async_task_t& task)
	{
		add_task_helper(&_disposible_task_queue, task);
	}
	//线程初始和退出任务
	void add_init_task(const async_task_t& task)
	{
		add_task_helper(&_init_task_queue, task, false);
	}
	void add_exit_task(const async_task_t& task)
	{
		add_task_helper(&_exit_task_queue, task, false);
	}
	void start();
	void clear_regual_task();
	
private:
	uint16_t						_thread_index;

	bool							_is_start;
	std::condition_variable			_cond_start;			//调用者阻塞等待工作线程创建完成
	std::mutex						_mutex_start;

	bool							_is_quit;				//由join阻塞等待
	bool							_is_main_loop_exit;		//退出主循环，主要是线程内部登记/设置

	bool							_is_taskempty;
	std::condition_variable			_cond_taskempty;		//无任务时候工作线程阻塞等待
	std::mutex						_mutex_taskempty;

	task_queue						_resident_task_queue;
	task_queue						_disposible_task_queue;

	task_queue						_init_task_queue;
	task_queue						_exit_task_queue;
	
	std::shared_ptr<std::thread>	_thread_ptr;
	
	std::thread::id					_tid;		

	void	thread_routine();
	void	clear();

	void add_task_helper(task_queue* tq, const async_task_t& task, bool wakeup = true)
	{
		std::unique_lock <std::mutex> lck(_mutex_taskempty);
		tq->add(task);

		if (wakeup)
		{
			if (_is_taskempty)
			{
				_is_taskempty = false;
				lck.unlock();
				_cond_taskempty.notify_one();
			}
		}
	}
};