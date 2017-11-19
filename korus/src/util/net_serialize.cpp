#include <string.h>
#include <assert.h>
#include <endian.h>
#include <byteswap.h>
#include <netinet/in.h>
#include "net_serialize.h"

uint32_t net_serialize::INVALID_SIZE = (uint32_t)-1;

uint64_t htonl64(uint64_t val)
{
	return (__BYTE_ORDER == __BIG_ENDIAN) ? return (val) : __bswap_64(val);
}

uint64_t ntohl64(uint64_t val)
{
	return (__BYTE_ORDER == __BIG_ENDIAN) ? return (val) : __bswap_64(val);
}

net_serialize::net_serialize() :_is_valid(false), _data(NULL), _size(0), _read_pos(0), _write_pos(0)
{
}

net_serialize::net_serialize(void* data, uint32_t size) : _is_valid(true), _data(data), _size(size), _read_pos(0), _write_pos(0)
{
}

net_serialize::~net_serialize()
{
	_is_valid = false;
}

// global
void net_serialize::attach(void* data, uint32_t size)
{
	reset();

	_data = data;
	_size = size;

	_is_valid = true;
}

void net_serialize::detach(void*& data, uint32_t& size)
{
	data = _data;
	size = _size;

	_is_valid = false;
}

void net_serialize::reset()
{
	_read_pos = 0;
	_write_pos = 0;
}

net_serialize::operator bool() const
{
	return _is_valid;
}

void* net_serialize::data() const
{
	return _data;
}

uint32_t net_serialize::size() const
{
	return _size;
}

// step
uint32_t net_serialize::wpos()
{
	if (!_is_valid)
	{
		return INVALID_SIZE;
	}

	return _write_pos;
}

bool net_serialize::wpos(uint32_t pos)
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

net_serialize& net_serialize::write_skip(uint32_t step)
{
	wpos(_write_pos + step);
	return *this;
}

uint32_t net_serialize::rpos()
{
	if (!_is_valid)
	{
		return INVALID_SIZE;
	}

	return _read_pos;
}

bool net_serialize::rpos(uint32_t pos)
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

net_serialize& net_serialize::read_skip(uint32_t step)
{
	rpos(_read_pos + step);
	return *this;
}

// serialize access
net_serialize& net_serialize::write(const void* data, uint32_t size)
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

net_serialize& net_serialize::read(void* data, uint32_t size)
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

net_serialize& net_serialize::operator << (const uint8_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const uint16_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const uint32_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const uint64_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const int8_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const int16_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const int32_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator << (const int64_t t)
{
	return buildin_write(t);
}

net_serialize& net_serialize::operator >> (uint8_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (uint16_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (uint32_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (uint64_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (int8_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (int16_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (int32_t& t)
{
	return buildin_read(t);
}

net_serialize& net_serialize::operator >> (int64_t& t)
{
	return buildin_read(t);
}

template <typename T>
net_serialize& net_serialize::buildin_write(const T& t)
{
	if (_is_valid)
	{
		if (_write_pos + sizeof(T) > _size)
		{
			_is_valid = false;
		}
		else
		{
			switch (sizeof(T))
			{
			case 1:
				*reinterpret_cast<T*>((uint8_t*)_data + _write_pos) = t;
				break;
			case 2:
				*reinterpret_cast<T*>((uint8_t*)_data + _write_pos) = htons(t);
				break;
			case 4:
				*reinterpret_cast<T*>((uint8_t*)_data + _write_pos) = htonl(t);
				break;
			case 8:
				*reinterpret_cast<T*>((uint8_t*)_data + _write_pos) = htonl64(t);
				break;
			default:
				assert(false);
				_is_valid = false;
				break;
			}
			_write_pos += sizeof(T);
		}
	}
	return *this;
}

template <typename T>
net_serialize& net_serialize::buildin_read(T& t)
{
	if (_is_valid)
	{
		if (_read_pos + sizeof(T) > _size)
		{
			_is_valid = false;
		}
		else
		{
			switch (sizeof(T))
			{
			case 1:
				t = *reinterpret_cast<T*>((uint8_t*)_data + _write_pos);
				break;
			case 2:
				t = ntohs(*reinterpret_cast<T*>((uint8_t*)_data + _write_pos));
				break;
			case 4:
				t = ntohl(*reinterpret_cast<T*>((uint8_t*)_data + _write_pos));
				break;
			case 8:
				t = ntohl64(*reinterpret_cast<T*>((uint8_t*)_data + _write_pos));
				break;
			default:
				assert(false);
				_is_valid = false;
				break;
			}
			_read_pos += sizeof(T);
		}
	}
	return *this;
}
