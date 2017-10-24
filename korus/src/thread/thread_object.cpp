#include "thread_object.h"

thread_object::thread_object(uint16_t index)
	: _thread_index(index), _is_start(false), _is_taskempty(true), _is_quit(false), _resident_task_queue(true, false), _disposible_task_queue(false, false), _init_task_queue(false, false), _exit_task_queue(false, false)
{

}

thread_object::~thread_object()
{
	clear();
}

void thread_object::start()
{
	std::unique_lock <std::mutex> lck(_mutex_start);
	if (!_thread_ptr)
	{		
		_tid = std::this_thread::get_id();
		_thread_ptr = std::make_shared<std::thread>(std::bind(&thread_object::thread_routine, this));

		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(_thread_index, &cpuset);
		int32_t rc = pthread_setaffinity_np(_thread_ptr->native_handle(), sizeof(cpu_set_t), &cpuset);
		if (rc != 0) 
		{
			//std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
		}

		while (!_is_start)
		{
			_cond_start.wait(lck);
		}
	}
}

void thread_object::thread_routine()
{
	_tid = std::this_thread::get_id();

	{
		std::unique_lock <std::mutex> lck(_mutex_start);
		_is_start = true;
		_cond_start.notify_one();
	}

	//init任务
	_init_task_queue.execute();
	
	//常规任务
	while (1)
	{
		//是否退出
		{
			//1, 是否退出或等待
			_is_taskempty = _resident_task_queue.is_empty() && _disposible_task_queue.is_empty();
			std::unique_lock <std::mutex> lck(_mutex_taskempty);
			while (!_is_quit && _is_taskempty)
			{
				_cond_taskempty.wait(lck);
			}
			if (_is_quit)
			{
				break;
			}
		} 

		//2, 常驻队列先轮一圈
		_resident_task_queue.execute();

		//3, 临时队列再轮一圈
		_disposible_task_queue.execute();
	}

	//exit任务
	_exit_task_queue.execute();
}

void	thread_object::clear()
{
	{
		std::unique_lock <std::mutex> lck(_mutex_taskempty);
		_is_quit = true;
		_resident_task_queue.clear();
		_disposible_task_queue.clear();
		_cond_taskempty.notify_one();
	}

	if (_thread_ptr)
	{
		if (_thread_ptr->joinable())
		{
			try
			{
				_thread_ptr->join();
			}
			catch (...)
			{
				//tbd log exception
			}
		}
		_thread_ptr = nullptr;
	}
}