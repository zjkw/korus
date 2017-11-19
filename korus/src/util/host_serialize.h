#pragma once

#include <stdint.h>

// ±¾»únative×Ö½ÚÐò
class host_serialize
{
public:
	host_serialize();
	host_serialize(void* data, uint32_t size);
	virtual ~host_serialize();

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
	host_serialize& write_skip(uint32_t step);
	uint32_t rpos();
	bool rpos(uint32_t pos);
	host_serialize& read_skip(uint32_t step);

	// serialize access
	host_serialize& write(const void* data, uint32_t size);
	host_serialize& read(void* data, uint32_t size);
	template <typename T>
	host_serialize& operator << (const T& t);
	template <typename T>
	host_serialize& operator >> (T& t);

private:
	bool		_is_valid;
	void*		_data;
	uint32_t	_size;
	uint32_t	_read_pos;
	uint32_t	_write_pos;
};


