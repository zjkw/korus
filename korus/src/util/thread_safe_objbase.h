#pragma once

#include <atomic>

class noncopyable
{
public:
	noncopyable();
	virtual ~noncopyable();
	
private:
	noncopyable(const noncopyable& o) = delete;
	noncopyable& operator=(const noncopyable& o) = delete;
};

class thread_safe_objbase : public noncopyable
{
public:
	thread_safe_objbase();
	virtual ~thread_safe_objbase();

	virtual void	invalid();
	virtual bool	is_valid();

protected:
	std::atomic<bool>	_invalid;
};

