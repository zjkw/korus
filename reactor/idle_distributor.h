#pragma once

#include <map>
#include <functional>
#include <mutex>
#include "idle_helper.h"

class idle_distributor
{
public:
	idle_distributor();
	virtual ~idle_distributor();

	void	idle();
	void	clear();
	void	start_idle(idle_helper* idle_id);
	void	stop_idle(idle_helper* idle_id);
	bool	exist_idle(idle_helper* idle_id);

private:
	struct idle_data
	{
		uint64_t				seq;
		idle_data(uint64_t seq_)
		{
			seq = seq_;
		}
	};

	uint64_t							_idle_seq;
	std::map<idle_helper*, idle_data>	_idle_list;
	std::recursive_mutex				_mutex_idle;
};

