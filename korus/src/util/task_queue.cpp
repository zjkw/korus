#include <assert.h>
#include "task_queue.h"
#include "unique_optional_lock.h"

task_queue::task_queue(bool persistence, bool nolock)
	: _persistence(persistence), _nolock(nolock), _seq(0)
{
}

task_queue::~task_queue()
{
	clear();
}

void task_queue::add(const async_task_t& task, const task_owner& owner/* = null_owner*/)
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	task_item	item(task, owner);
	_task_list[++_seq] = item;
	if (null_owner != owner)
	{
		std::map<task_owner, std::map<uint64_t, uint64_t>>::iterator it = _owner_list.find(owner);
		if (it == _owner_list.end())
		{
			std::map<uint64_t, uint64_t> invert;
			invert.insert(std::make_pair(_seq, _seq));
			_owner_list[owner] = invert;
		}
		else
		{
			it->second.insert(std::make_pair(_seq, _seq));
		}
	}
}

void task_queue::del(const task_owner& owner/* = null_owner*/)
{
	if (null_owner != owner)
	{
		std::map<task_owner, std::map<uint64_t, uint64_t>>::iterator it = _owner_list.find(owner);
		if (it != _owner_list.end())
		{
			for (std::map<uint64_t, uint64_t>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++)
			{
				_task_list.erase(it2->second);
			}

			_owner_list.erase(owner);
		}
	}
}

void task_queue::execute()
{
	unique_optional_lock <std::recursive_mutex> lck(_mutex, _nolock);
	uint64_t	seq_old = _seq;
	uint64_t	seq_cursor = 0;
	while (true)
	{
		std::map<uint64_t, task_item>::iterator it = _task_list.upper_bound(seq_cursor);
		if (it == _task_list.end())
		{
			break;
		}
		seq_cursor = it->first;
		if (it->first > seq_old)
		{
			continue;
		}

		task_item	item = it->second;
		if (!_persistence)
		{
			if (null_owner != item.owner)
			{
				std::map<task_owner, std::map<uint64_t, uint64_t>>::iterator it2 = _owner_list.find(item.owner);
				assert(it2 != _owner_list.end());
				it2->second.erase(seq_cursor);
				if (!it2->second.size())
				{
					_owner_list.erase(item.owner);
				}
			}

			_task_list.erase(seq_cursor);
		}

		item.task();
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
	_owner_list.clear();
}