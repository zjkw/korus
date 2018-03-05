#include <string.h>
#include <assert.h>
#include "buffer_thunk.h"

//预估size
static inline size_t fls(size_t x)
{
	if (x < 64)
	{
		return 64;
	}

	size_t i = (x >> 7); //6 for 64
	int position = 0;
	for (; i != 0; ++position)
	{
		i >>= 1;
	}

	return 1 << (position + 7); //6 for 64
}

static inline size_t round(size_t x)
{
	return (((x)+1024 - 1) & ~(1024 - 1));
}

static inline size_t smart_size(size_t x)
{
	if (x < 64)
	{
		return 64;
	}
	else if (x < 1024)
	{
		return 1024;
	}
	else
	{
		return round(fls(x));
	}
}
/////////////////////////////////////////////////////

buffer_thunk::buffer_thunk() : _buf(nullptr), _size(0), _begin(0), _used(0)
{
}

buffer_thunk::buffer_thunk(const void* data, const size_t size) : _buf(nullptr), _size(0), _begin(0), _used(0)
{
	push_back(data, size);
}

buffer_thunk::buffer_thunk(buffer_thunk* rhs)
{
	push_back(rhs->ptr(), rhs->used());
}

buffer_thunk::~buffer_thunk()
{
	delete []_buf;
	_buf = nullptr;
}

void	buffer_thunk::push_front(const size_t size)
{
	adjust_size_by_head(size);

	if (_begin)
	{
		assert(_begin >= size);
		_begin -= size;
	}

	_used += size;
}

void	buffer_thunk::push_back(const void* data, const size_t size)
{
	adjust_size_by_tail(size);

	memcpy(_buf + _begin + _used, data, size);
	_used += size;
}

void	buffer_thunk::push_back(buffer_thunk* rhs)
{
	push_back(rhs->ptr(), rhs->used());
}

size_t	buffer_thunk::pop_front(const size_t size)
{
	if (!_buf)
	{
		return 0;
	}

	if (size >= _used)
	{
		size_t real = _used;

		_begin = 0;
		_used = 0;
		return real;
	}
	else
	{
	//	memmove(_buf + _begin, _buf + _begin + size, _used - size);
		_begin += size;
		_used -= size;
		return size;
	}
}

bool	buffer_thunk::prepare(const size_t new_size)
{
	return adjust_size_by_tail(new_size);
}

void	buffer_thunk::append_move(buffer_thunk* rhs)
{
	push_back(rhs->ptr(), rhs->used());
	rhs->reset();
}

void	buffer_thunk::reset()
{
	_begin = 0;
	_used = 0;
}

bool	buffer_thunk::adjust_size_by_head(const size_t append_size)
{
	//1，整体空间是否足够容纳新数据
	if (_size >= _used + append_size)
	{
		//2，基于已有begin位置开始算是否足够容纳新数据
		if (_begin < append_size)
		{
			memmove(_buf + append_size, _buf + _begin, _used);
			_begin = append_size;
		}
	}
	else
	{
		size_t new_size = smart_size(_size + append_size);
		uint8_t* new_buf = new uint8_t[new_size];

		if (_buf)
		{
			memcpy(new_buf + append_size, _buf + _begin, _used);
			delete[]_buf;
		}

		_buf = new_buf;
		_begin = append_size;
		_size = new_size;
	}

	return true;
}

bool	buffer_thunk::adjust_size_by_tail(const size_t append_size)
{
	//1，整体空间是否足够容纳新数据
	if (_size >= _used + append_size)
	{
		//2，基于已有begin位置开始算是否足够容纳新数据
		if (_size < _begin + _used + append_size)
		{
			memmove(_buf, _buf + _begin, _used);
			_begin = 0;
		}
	}
	else
	{
		size_t new_size = smart_size(_size + append_size);
		uint8_t* new_buf = new uint8_t[new_size];

		if (_buf)
		{
			memcpy(new_buf, _buf + _begin, _used);
			delete[]_buf;
		}

		_buf = new_buf;
		_begin = 0;
		_size = new_size;
	}

	return true;
}