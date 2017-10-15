#pragma once

#include "tcp_channel_base.h"
#include "..\reactor\reactor_loop.h"
#include "..\reactor\backend_poller.h"

class tcp_server_callback;
//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如先close后可能还能send/recv

//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作
class tcp_server_channel : public std::enable_shared_from_this<tcp_server_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	tcp_server_channel(SOCKET fd, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_callback> cb,
				const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	// 下面四个函数可能运行在多线程环境下	
	int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包
	void		close();	
	void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<tcp_server_callback>	_cb;

	friend class tcp_listen;
	void		invalid();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);
};

// 可能处于多线程环境下
class tcp_server_callback : public std::enable_shared_from_this<tcp_server_callback>, public thread_safe_objbase
{
public:
	tcp_server_callback(){}
	virtual ~tcp_server_callback(){}

	//override------------------
	virtual void	on_accept(std::shared_ptr<tcp_server_channel> channel) = 0;	//连接已经建立
	virtual void	on_closed(std::shared_ptr<tcp_server_channel> channel) = 0;
	//参考TCP_ERROR_CODE定义
	virtual void	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_server_channel> channel) = 0;
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel) = 0;
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel) = 0;
};