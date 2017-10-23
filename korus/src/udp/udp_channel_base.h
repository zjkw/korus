#pragma once

#include <mutex>
#include "korus/src/util/basic_defines.h"

// 本类函数均不判断fd有效性，由派生类负责
// 仅仅send可以处于非创建线程，其他函数均在当前线程，所以recv可以不加锁
class udp_channel_base
{
public:
	udp_channel_base(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_channel_base();

	// 下面四个函数可能运行在多线程环境下	
	virtual	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// 外部数据发送
	virtual	void		close();
	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr) = 0;
protected:
	//>0 表示还可以继续recv,=0表示收取到顶了，<0表示错误
	int32_t				do_recv();									
	int32_t				send_alone();								// 内部数据发送
	SOCKET				_fd;
	void				set_fd(SOCKET fd);
	bool				bind_local_addr();

private:
	std::mutex			_mutex_write;

	std::string			_local_addr;
	uint32_t			_self_read_size;
	uint8_t*			_self_read_buff;
	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send_inlock(const void* buf, uint32_t len, const sockaddr_in& peer_addr);
	int32_t				do_recv_nolock();
	
};
