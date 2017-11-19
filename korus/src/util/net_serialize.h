#pragma once

#include <stdint.h>

// ±¾»únative×Ö½ÚÐò
class net_serialize
{
public:
	net_serialize();
	net_serialize(void* data, uint32_t size);
	virtual ~net_serialize();

	// global
	void attach(void* data, uint32_t size);
	void detach(void*& data, uint32_t& size);
	void reset();
	operator bool() const;
	void* data() const;
	uint32_t size() const;

	// step
	static uint32_t INVALID_SIZE;
	uint32_t wpos();
	bool wpos(uint32_t pos);
	net_serialize& write_skip(uint32_t step);
	uint32_t rpos();
	bool rpos(uint32_t pos);
	net_serialize& read_skip(uint32_t step);

	// serialize access
	net_serialize& write(const void* data, uint32_t size);
	net_serialize& read(void* data, uint32_t size);
	net_serialize& operator << (const uint8_t t);
	net_serialize& operator << (const uint16_t t);
	net_serialize& operator << (const uint32_t t);
	net_serialize& operator << (const uint64_t t);
	net_serialize& operator << (const int8_t t);
	net_serialize& operator << (const int16_t t);
	net_serialize& operator << (const int32_t t);
	net_serialize& operator << (const int64_t t);	
	net_serialize& operator >> (uint8_t& t);
	net_serialize& operator >> (uint16_t& t);
	net_serialize& operator >> (uint32_t& t);
	net_serialize& operator >> (uint64_t& t);
	net_serialize& operator >> (int8_t& t);
	net_serialize& operator >> (int16_t& t);
	net_serialize& operator >> (int32_t& t);
	net_serialize& operator >> (int64_t& t);

private:
	bool		_is_valid;
	void*		_data;
	uint32_t	_size;
	uint32_t	_read_pos;
	uint32_t	_write_pos;

	template <typename T>
	net_serialize& buildin_write(const T& t);
	template <typename T>
	net_serialize& buildin_read(T& t);
};


