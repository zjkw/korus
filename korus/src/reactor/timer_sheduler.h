#pragma once

#include <map>
#include <chrono>
#include <functional>
#include <mutex>
#include "timer_helper.h"

class timer_sheduler
{
public:
	timer_sheduler();
	virtual ~timer_sheduler();

	void	tick();
	void	clear();
	void	start_timer(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval);
	void	start_timer(timer_helper* timer_id, const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval);
	void	stop_timer(timer_helper* timer_id);
	bool	exist_timer(timer_helper* timer_id);
	std::chrono::milliseconds	min_interval();

private:
	struct timer_key
	{
		std::chrono::system_clock::time_point	triggle;
		uint64_t								unique_seq;	//±‹√‚œ‡Õ¨µƒmap key
		timer_helper*							id;
		timer_key(const std::chrono::system_clock::time_point& triggle_, const uint64_t& unique_seq_, timer_helper* timer_id = nullptr)
		{
			triggle = triggle_;
			unique_seq = unique_seq_;
			id = timer_id;
		}
		timer_key()
		{
			unique_seq = 0;
			id = nullptr;
		}
		bool operator < (const timer_key& a) const
		{
			return triggle < a.triggle;
		}
		bool operator >(const timer_key& a) const
		{
			return triggle > a.triggle;
		}
	};
	struct timer_item
	{
		std::chrono::milliseconds				interval;
		timer_item()
		{

		}
		timer_item(const std::chrono::milliseconds& interval_)
		{
			interval = interval_;
		}
	};

	uint64_t									_unique_seq;
	std::map<timer_helper*, timer_key>			_object_list;
	std::map<timer_key, timer_item>				_timer_list;
	std::recursive_mutex						_mutex_timer;

	void	start_timer_inlock(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval);
	void	stop_timer_inlock(timer_helper* timer_id);
};

