#include <string.h>
#include "host_serialize.h"

uint32_t host_serialize::INVALID_SIZE = (uint32_t)-1;

host_serialize::host_serialize() :_is_valid(false), _data(NULL), _size(0), _read_pos(0), _write_pos(0)
{ 
}

host_serialize::host_serialize(const void* data, const uint32_t size) : _is_valid(true), _data((void*)data), _size(size), _read_pos(0), _write_pos(0)
{
}

host_serialize::host_serialize(void* data, const uint32_t size) : _is_valid(true), _data(data), _size(size), _read_pos(0), _write_pos(0)
{
}

host_serialize::~host_serialize()
{
	_is_valid = false;
}

// global
void host_serialize::attach(const void* data, const uint32_t size)
{
	attach((void*)data, size);
}

void host_serialize::attach(void* data, const uint32_t size)
{
	reset();

	_data = data;
	_size = size;

	_is_valid = true;
}

void host_serialize::detach(void*& data, uint32_t& size)
{
	data = _data;
	size = _size;

	_is_valid = false;
}

void host_serialize::reset()
{
	_read_pos = 0;
	_write_pos = 0;
}

host_serialize::operator bool() const
{
	return _is_valid;
}

void* host_serialize::data() const
{
	return _data;
}

uint32_t host_serialize::size() const
{
	return _size;
}

// step
uint32_t host_serialize::wpos()
{
	if (!_is_valid)
	{
		return INVALID_SIZE;
	}

	return _write_pos;
}

bool host_serialize::wpos(uint32_t pos)
{
	if (_is_valid)
	{
		if (pos > _size)
		{
			_is_valid = false;
		}
		else
		{
			_write_pos = pos;
		}
	}

	return _is_valid;
}

host_serialize& host_serialize::write_skip(uint32_t step)
{
	wpos(_write_pos + step);
	return *this;
}

uint32_t host_serialize::rpos()
{
	if (!_is_valid)
	{
		return INVALID_SIZE;
	}

	return _read_pos;
}

bool host_serialize::rpos(uint32_t pos)
{
	if (_is_valid)
	{
		if (pos > _size)
		{
			_is_valid = false;
		}
		else
		{
			_read_pos = pos;
		}
	}

	return _is_valid;
}

host_serialize& host_serialize::read_skip(uint32_t step)
{
	rpos(_read_pos + step);
	return *this;
}

// serialize access
host_serialize& host_serialize::write(const void* data, uint32_t size)
{
	if (_is_valid)
	{
		if (_write_pos + size > _size)
		{
			_is_valid = false;
		}
		else
		{
			memcpy((uint8_t*)_data + _write_pos, data, size);
			_write_pos += size;
		}
	}
	return *this;
}

host_serialize& host_serialize::read(void* data, uint32_t size)
{
	if (_is_valid)
	{
		if (_read_pos + size > _size)
		{
			_is_valid = false;
		}
		else
		{
			memcpy(data, (uint8_t*)_data + _read_pos, size);
			_read_pos += size;
		}
	}
	return *this;
}

template <typename T>
host_serialize& host_serialize::operator << (const T& t)
{
	if (_is_valid)
	{
		if (_write_pos + sizeof(T) > _size)
		{
			_is_valid = false;
		}
		else
		{
			*reinterpret_cast<T*>((uint8_t*)_data + _write_pos) = t;
			_write_pos += sizeof(T);
		}
	}
	return *this;
}

template <typename T>
host_serialize& host_serialize::operator >> (T& t)
{
	if (_is_valid)
	{
		if (_read_pos + sizeof(T) > _size)
		{
			_is_valid = false;
		}
		else
		{
			t = *reinterpret_cast<const T*>((uint8_t*)_data + _read_pos);
			_read_pos += sizeof(T);
		}
	}
	return *this;
}
