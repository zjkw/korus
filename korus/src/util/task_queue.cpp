#include "task_queue.h"
#include "unique_optional_lock.h"

task_queue::task_queue(bool persistence, bool nolock)
	: _persistence(persistence), _nolock(nolock), _seq(0)
{
}

task_queue::~task_queue()
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	_task_list.clear();
}

void task_queue::add(const async_task_t& task)
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	_task_list[++_seq] = task;
}

void task_queue::execute()
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	uint64_t	seq_old = _seq;
	uint64_t	seq_cursor = 0;
	while (true)
	{
		std::map<uint64_t, async_task_t>::iterator it = _task_list.upper_bound(seq_cursor);
		if (it == _task_list.end())
		{
			break;
		}
		seq_cursor = it->first;
		if (it->first > seq_old)
		{
			continue;
		}
		
		async_task_t task = it->second;

		if (!_persistence)
		{
			_task_list.erase(seq_cursor);
		}

		task();
	}
}

bool	task_queue::is_empty()
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	return _task_list.size() == 0;
}

void	task_queue::clear()
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	_task_list.clear();
}