#pragma once

#include "korus/src/util/basic_defines.h"
#include "korus/src/util/buffer_thunk.h"

// 本类函数均不判断fd有效性，由派生类负责
// 仅仅send可以处于非创建线程，其他函数均在当前线程，所以recv可以不加锁
class tcp_channel_base
{
public:
	tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_channel_base();

	// 下面三个函数可能运行在多线程环境下	
	virtual	int32_t		send_raw(const std::shared_ptr<buffer_thunk>& data);// 外部数据发送
	virtual	void		close();
	virtual	void		shutdown(int32_t howto);
	virtual	int32_t		on_recv_buff(std::shared_ptr<buffer_thunk>& data, bool& left_partial_pkg) = 0;
	
	virtual bool		peer_addr(std::string& addr);
	virtual bool		local_addr(std::string& addr);

protected:
	int32_t				send_alone();								// 内部数据发送
	SOCKET				_fd;
	void				set_fd(SOCKET fd);
	int32_t				do_recv();	//请勿在派生类/外部非终结处调用,因为接受中不会再check接收状态

private:
	std::shared_ptr<buffer_thunk> _read_thunk;
	std::shared_ptr<buffer_thunk> _write_thunk;

	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send(const std::shared_ptr<buffer_thunk>& data);	
};
