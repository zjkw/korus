#pragma once

#include <memory>
#include <assert.h>
#include "object_state.h"

//基于深拷贝
class buffer_thunk : public noncopyable
{
public:
	buffer_thunk();
	buffer_thunk(const void* data, const size_t size);	
	buffer_thunk(buffer_thunk* rhs);
	virtual ~buffer_thunk();
	
	void	push_front(const size_t size);
	void	push_back(const void* data, const size_t size);
	void	push_back(buffer_thunk* rhs);
	size_t	pop_front(const size_t size);

	bool	prepare(const size_t new_size);	//预申请
	void	append_move(buffer_thunk* rhs);	
	void	reset();

	const char*		ptr() { return ((const char*)(char*)_buf) + _begin; }
	const size_t	used() { return _used; }
	const size_t	rpos() { return _rpos; }
	const void		incr(const size_t size) { _used += size; assert(_used <= _size); }
	const void		rpos(const size_t rpos) { _rpos = rpos; }

protected:
	uint8_t*	_buf;
	size_t		_size;	//总长度
	size_t		_begin;	//有效起点
	size_t		_used;	//有效长度
	size_t		_rpos;	//临时read位置

	bool		adjust_size_by_head(const size_t append_size);
	bool		adjust_size_by_tail(const size_t append_size);
};


