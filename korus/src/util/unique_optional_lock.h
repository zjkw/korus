#pragma once

#include "object_state.h"

template<class _Mutex>
class unique_optional_lock : public noncopyable
{
public:
	unique_optional_lock(_Mutex& Mtx, bool nolock)		
		: _nolock(nolock)
	{
		if (_nolock)
		{
			_Pmtx = &Mtx;
			_Pmtx->lock();
		}
	}
	virtual ~unique_optional_lock()
	{
		if (_nolock)
		{
			_Pmtx->unlock();
		}
	}

private:
	bool	_nolock;
	_Mutex *_Pmtx;
};

