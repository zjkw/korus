#include <assert.h>
#include "timer_sheduler.h"


timer_sheduler::timer_sheduler()
	: _unique_seq(0)
{
}


timer_sheduler::~timer_sheduler()
{
}

void	timer_sheduler::tick()
{
	// 此刻最大的id
	timer_key	now_id(std::chrono::system_clock::now(), (uint64_t)-1);
	uint64_t	old_seq = _unique_seq;//为了避免回调插入导致的循环：tick -> cb -> map::insert
	timer_key	cursor_id;//默认为zero
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	// 取出满足条件最小的
	while (true)
	{
		std::map<timer_key, timer_item>::iterator it = _timer_list.upper_bound(cursor_id);
		if (it == _timer_list.end())
		{
			break;
		}
		if (it->first > now_id)
		{
			break;
		}
		if (it->first.unique_seq > old_seq)
		{
			break;
		}
		cursor_id = it->first;
		timer_key	tid = it->first;
		timer_item	item = it->second;		

		stop_timer_inlock(tid.id);
		if (item.interval.count())
		{
			start_timer_inlock(tid.id, tid.triggle + item.interval, item.interval);
		}
		
		tid.id->action();
	}
}

void	timer_sheduler::clear()
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	_timer_list.clear();
	_object_list.clear();
}

void	timer_sheduler::start_timer(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval)
{
	std::chrono::milliseconds interval_tmp = interval;
	if (!interval_tmp.count())
	{
		interval_tmp += std::chrono::milliseconds(1000);
	}
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	start_timer_inlock(timer_id, begin, interval_tmp);
}

void	timer_sheduler::start_timer(timer_helper* timer_id, const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval)
{
	start_timer(timer_id, std::chrono::system_clock::now() + begin, interval);
}

void	timer_sheduler::stop_timer(timer_helper* timer_id)
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	stop_timer_inlock(timer_id);
}

bool	timer_sheduler::exist_timer(timer_helper* timer_id)
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	return _object_list.find(timer_id) != _object_list.end();
}

std::chrono::milliseconds	timer_sheduler::min_interval()
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_timer);
	do
	{
		std::map<timer_key, timer_item>::iterator it = _timer_list.begin();
		if (it == _timer_list.end())
		{
			break;
		}
		return std::chrono::duration_cast<std::chrono::milliseconds>(it->first.triggle - std::chrono::system_clock::now());
	} while (0);

	return std::chrono::milliseconds(0);
}

void	timer_sheduler::start_timer_inlock(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval)
{
	// 1, 是否存在
	stop_timer_inlock(timer_id);

	// 2, 重建新
	timer_key	key(begin, ++_unique_seq, timer_id);
	timer_item	item(interval);
	_object_list[timer_id] = key;
	_timer_list[key] = item;
}

void	timer_sheduler::stop_timer_inlock(timer_helper* timer_id)
{
	std::map<timer_helper*, timer_key>::iterator it = _object_list.find(timer_id);
	if (it != _object_list.end())
	{
		assert(_timer_list.find(it->second) != _timer_list.end());
		_timer_list.erase(it->second);
		_object_list.erase(it);
	}
}