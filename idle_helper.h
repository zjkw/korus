#pragma once

#include "reactor_loop.h"

class reactor_loop;
class idle_helper;
using reactor_idle_callback_t = std::function<void(idle_helper*)>;

//¾¡Á¿ÇáµÄ¸ºÔØ
class idle_helper
{
public:
	idle_helper(reactor_loop* reatcor);
	virtual ~idle_helper();

	void	bind(reactor_idle_callback_t task);
	void	clear();
	void	start();
	void	stop();
	bool	exist();
	void	action();
	
private:
	reactor_loop*				_reactor;
	reactor_idle_callback_t		_task;
};

