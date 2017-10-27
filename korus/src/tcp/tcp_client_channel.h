#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <list>
#include "korus/src/util/thread_safe_objbase.h"
#include "korus/src/reactor/timer_helper.h"
#include "korus/src/reactor/sockio_helper.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_channel_base.h"

class tcp_client_handler_base;

using tcp_client_channel_factory_t = std::function<std::shared_ptr<tcp_client_handler_base>()>;
using tcp_client_channel_factory_chain_t = std::list<tcp_client_channel_factory_t>;

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//外部最好确保tcp_client能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

enum TCP_CLTCONN_STATE
{
	CNS_CLOSED = 0,
	CNS_CONNECTING = 2,
	CNS_CONNECTED = 3,
};
//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作

class tcp_client_channel : public std::enable_shared_from_this<tcp_client_channel>, public thread_safe_objbase, public tcp_channel_base
{
public:
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_handler_base> cb,
		std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// 下面四个函数可能运行在多线程环境下	
	int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	void		close();
	void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown
	void		connect();
	TCP_CLTCONN_STATE	get_state()	{ return _conn_state; }
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	SOCKET									_conn_fd;
	std::atomic<TCP_CLTCONN_STATE>			_conn_state;
	std::string								_server_addr;
	std::shared_ptr<reactor_loop>			_reactor;
	std::shared_ptr<tcp_client_handler_base>	_cb;

	std::chrono::seconds					_connect_timeout;	//触发时机：执行connect且等待状态下；当connect结果在超时前出来，将关掉定时器；触发动作：强行切换到CLOSED
	std::chrono::seconds					_connect_retry_wait;//触发时机：已经明确connect失败/超时情况下启动；在connect时候关掉定时器；触发动作：自动执行connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	template<typename T> friend class tcp_client;
	// channel要析构/回收要看handler是否还没其他对象持有引用
	bool		check_detach_relation(long call_ref_count);	//true表示已经互相解除关系
	void		invalid();

	sockio_helper	_sockio_helper_connect;
	sockio_helper	_sockio_helper;
	virtual void on_sockio_write_connect(sockio_helper* sockio_id);
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);
	
	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class tcp_client_handler_base : public std::enable_shared_from_this<tcp_client_handler_base>, public thread_safe_objbase
{
public:
	tcp_client_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~tcp_client_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_init() = 0;
	virtual void	on_final() = 0;
	virtual void	on_connect() = 0;
	virtual void	on_closed() = 0;
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code) = 0;
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len) = 0;
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len) = 0;

protected:
	int32_t	send(const void* buf, const size_t len)	{ if (!_channel) return CEC_INVALID_SOCKET; return _channel->send(buf, len); }
	void	close()									{ if (_channel)_channel->close(); }
	void	shutdown(int32_t howto)					{ if (_channel)_channel->shutdown(howto); }
	void	connect()								{ if (_channel)_channel->connect(); }
	TCP_CLTCONN_STATE	get_state()					{ if (!_channel) return CNS_CLOSED;	return _channel->get_state(); }
	std::shared_ptr<reactor_loop>	get_reactor()	{ return _reactor; }

private:
	template<typename T> friend class tcp_client;
	friend class tcp_client_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_client_channel> channel)
	{
		_reactor = reactor;
		_channel = channel;

		on_init();
	}
	void	inner_final()
	{
		_reactor = nullptr;
		_channel = nullptr;

		on_final();
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_client_channel> _channel;
};