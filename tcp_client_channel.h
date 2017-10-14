#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include "thread_safe_objbase.h"
#include "tcp_channel_base.h"
#include "timer_helper.h"
#include "reactor_loop.h"

//connect不要把错误(EINTR/EINPROGRESS/EAGAIN)当成Fatal.

class tcp_client_callback;
class tcp_client_channel : public std::enable_shared_from_this<tcp_client_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	enum CONN_STATE
	{
		CNS_CLOSED = 0,
		CNS_CONNECTING = 2,
		CNS_CONNECTED = 3,
	};
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_callback> cb,
		std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// 下面四个函数可能运行在多线程环境下	
	int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包
	void		close();
	void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown
	void		connect();
	CONN_STATE	get_state()	{ return _conn_state; }
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	SOCKET									_conn_fd;
	std::atomic<CONN_STATE>					_conn_state;
	std::string								_server_addr;
	std::shared_ptr<reactor_loop>			_reactor;
	std::shared_ptr<tcp_client_callback>	_cb;

	std::chrono::seconds					_connect_timeout;	//触发时机：执行connect且等待状态下；当connect结果在超时前出来，将关掉定时器；触发动作：强行切换到CLOSED
	std::chrono::seconds					_connect_retry_wait;//触发时机：已经明确connect失败/超时情况下启动；在connect时候关掉定时器；触发动作：自动执行connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	void		invalid();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);
};

// 可能处于多线程环境下
class tcp_client_callback : public std::enable_shared_from_this<tcp_client_callback>, public thread_safe_objbase
{
public:
	tcp_client_callback(){}
	virtual ~tcp_client_callback(){}

	//override------------------
	virtual void	on_connect(std::shared_ptr<tcp_client_channel> channel) = 0;
	virtual void	on_closed(std::shared_ptr<tcp_client_channel> channel) = 0;
	//参考TCP_ERROR_CODE定义
	virtual void	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_client_channel> channel) = 0;
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
};