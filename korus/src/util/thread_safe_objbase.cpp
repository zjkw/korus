#include "thread_safe_objbase.h"

noncopyable::noncopyable()
{
}

noncopyable::~noncopyable()
{
}

//////////////////////////
thread_safe_objbase::thread_safe_objbase()
	: _invalid(true)
{
}

thread_safe_objbase::~thread_safe_objbase()
{
}

void	thread_safe_objbase::invalid()
{
	_invalid = false;
}

bool	thread_safe_objbase::is_valid()
{
	return _invalid;
}