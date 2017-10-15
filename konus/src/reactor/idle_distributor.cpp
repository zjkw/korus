#include "idle_distributor.h"

idle_distributor::idle_distributor()
	: _idle_seq(0)
{
}

idle_distributor::~idle_distributor()
{
	clear();
}

void	idle_distributor::idle()
{
	std::unique_lock<std::recursive_mutex> lck(_mutex_idle);
	
	uint64_t		old_seq = _idle_seq;
	idle_helper*	id = nullptr;

	while (true)
	{
		std::map<idle_helper*, idle_data>::iterator it = _idle_list.upper_bound(id);//上界，避免递归导致的后续pair被删除而找不到后续
		if (it == _idle_list.end())
		{
			break;
		}
		if (it->second.seq >= old_seq)
		{
			break;
		}

		id = it->first;
		id->action();
	}
}

void	idle_distributor::clear()
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_idle);
	_idle_list.clear();
}

void	idle_distributor::start_idle(idle_helper* idle_id)
{
	idle_data data(++_idle_seq);	
	std::unique_lock <std::recursive_mutex> lck(_mutex_idle);
	_idle_list[idle_id] = data;
}

void	idle_distributor::stop_idle(idle_helper* idle_id)
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_idle);
	_idle_list.erase(idle_id);
}

bool	idle_distributor::exist_idle(idle_helper* idle_id)
{
	std::unique_lock <std::recursive_mutex> lck(_mutex_idle);
	return _idle_list.find(idle_id) != _idle_list.end();
}
