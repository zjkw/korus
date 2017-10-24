#pragma once

#include <mutex>
#include "korus/src/util/basic_defines.h"

// 本类函数均不判断fd有效性，由派生类负责
// 仅仅send可以处于非创建线程，其他函数均在当前线程，所以recv可以不加锁
class tcp_channel_base
{
public:
	tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_channel_base();

	// 下面三个函数可能运行在多线程环境下	
	virtual	int32_t		send(const void* buf, const size_t len);// 外部数据发送
	virtual	void		close();
	virtual	void		shutdown(int32_t howto);
	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg) = 0;
protected:
	//>0 表示还可以继续recv,=0表示收取到顶了，<0表示错误
	int32_t				do_recv();									// 结果存放在_self_read_buff，触发on_after_recv
	int32_t				send_alone();								// 内部数据发送
	SOCKET				_fd;
	void				set_fd(SOCKET fd);

private:
	std::mutex			_mutex_write;

	bool				_recving;
	uint32_t			_self_read_size;
	uint32_t			_self_read_pos;
	uint8_t*			_self_read_buff;
	uint32_t			_self_write_size;
	uint32_t			_self_write_pos;
	uint8_t*			_self_write_buff;

	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send_inlock(const void* buf, uint32_t len);
	int32_t				do_recv_nolock();
	
};
