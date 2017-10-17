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
	uint64_t							_idle_seq;
	std::map<idle_helper*, uint64_t>	_idle_list;
	std::recursive_mutex				_mutex_idle;
};

