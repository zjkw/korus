#pragma once

#include <assert.h>
#include "tcp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/backend_poller.h"

class tcp_server_handler_base;
//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作
class tcp_server_channel : public std::enable_shared_from_this<tcp_server_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	tcp_server_channel(SOCKET fd, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_handler_base> cb,
				const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	// 下面四个函数可能运行在多线程环境下	
	int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	void		close();	
	void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<tcp_server_handler_base>	_cb;

	friend class tcp_server_channel_creator;
	void		invalid();
	void		detach();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class tcp_server_handler_base : public std::enable_shared_from_this<tcp_server_handler_base>, public thread_safe_objbase
{
public:
	tcp_server_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~tcp_server_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_accept() = 0;	//连接已经建立
	virtual void	on_closed() = 0;
	//参考TCP_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code) = 0;
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len) = 0;
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len) = 0;

protected:
	int32_t	send(const void* buf, const size_t len)	{ if (!_channel) return CEC_INVALID_SOCKET; return _channel->send(buf, len); }
	void	close()									{ if (_channel)_channel->close(); }
	void	shutdown(int32_t howto)					{ if (_channel)_channel->shutdown(howto); }
	std::shared_ptr<reactor_loop>	get_reactor()	{	return _reactor; }

private:
	friend class tcp_server_channel_creator;
	friend class tcp_server_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_channel> channel)
	{
		_reactor = reactor;
		_channel = channel;
	}
	void	inner_uninit()
	{
		_reactor = nullptr;
		_channel = nullptr;
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_server_channel> _channel;
};