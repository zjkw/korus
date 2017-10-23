#pragma once

#include <stdint.h>
#include <functional>
#include <map>
#include <mutex>

using async_task_t = std::function<void()>;
using task_owner = void*;
#define null_owner nullptr

//非线程安全

class task_queue	
{
public:
	task_queue(bool persistence, bool nolock);
	virtual ~task_queue();

	void			add(const async_task_t& task, const task_owner& owner = null_owner);
	void			del(const task_owner& owner = null_owner);
	void			execute();
	bool			is_empty();
	void			clear();
	
private:
	struct task_item
	{
		task_item(const async_task_t& task_ = nullptr, const task_owner& owner_ = null_owner)
		{
			task = task_;
			owner = owner_;
		}
		async_task_t	task;
		task_owner		owner;
	};
	bool								_persistence;//是否持久
	bool								_nolock;

	uint64_t							_seq;
	std::map<uint64_t, task_item>		_task_list;
	std::map<task_owner, std::map<uint64_t, uint64_t>>	_owner_list;
	std::recursive_mutex				_mutex;
};