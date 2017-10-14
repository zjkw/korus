#pragma once

class reactor_loop;
class timer_helper;
using reactor_timer_callback_t = std::function<void(timer_helper*)>;

class timer_helper
{
public:
	timer_helper(reactor_loop* reatcor);
	virtual ~timer_helper();
	void	bind(reactor_timer_callback_t task);
	void	clear();
	void	start(const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval);
	void	start(const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval);
	void	stop();
	bool	exist();
	void	action();

private:
	reactor_loop*				_reactor;
	reactor_timer_callback_t	_task;
};