#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include "korus/src/util/object_state.h"
#include "korus/src/util/chain_sharedobj_base.h"
#include "korus/src/reactor/timer_helper.h"
#include "korus/src/reactor/sockio_helper.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_channel_base.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class tcp_client_handler_base;

using tcp_client_channel_factory_t = std::function<std::shared_ptr<tcp_client_handler_base>(std::shared_ptr<reactor_loop>)>;

enum TCP_CLTCONN_STATE
{
	CNS_CLOSED = 0,
	CNS_CONNECTING = 2,
	CNS_CONNECTED = 3,
};

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class tcp_client_handler_base : public chain_sharedobj_base<tcp_client_handler_base>
{
public:
	tcp_client_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~tcp_client_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();

	virtual void	on_connected();
	virtual void	on_closed();
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len);

	virtual int32_t	send(const void* buf, const size_t len);
	virtual void	close();
	virtual void	shutdown(int32_t howto);
	virtual void	connect();
	virtual TCP_CLTCONN_STATE	state();
	virtual std::shared_ptr<reactor_loop>	reactor();
		
private:
	std::shared_ptr<reactor_loop>		_reactor;
};

//外部最好确保tcp_client能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作

class tcp_client_channel : public tcp_channel_base, public tcp_client_handler_base, public multiform_state
{
public:
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// 下面四个函数可能运行在多线程环境下	
	virtual int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void		close();
	virtual void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown
	virtual void		connect();
	virtual TCP_CLTCONN_STATE	state()	{ return _conn_state; }

private:
	SOCKET									_conn_fd;
	std::atomic<TCP_CLTCONN_STATE>			_conn_state;
	std::string								_server_addr;

	std::chrono::seconds					_connect_timeout;	//触发时机：执行connect且等待状态下；当connect结果在超时前出来，将关掉定时器；触发动作：强行切换到CLOSED
	std::chrono::seconds					_connect_retry_wait;//触发时机：已经明确connect失败/超时情况下启动；在connect时候关掉定时器；触发动作：自动执行connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	template<typename T> friend class tcp_client;
	virtual void		set_release();

	sockio_helper	_sockio_helper_connect;
	sockio_helper	_sockio_helper;
	virtual void on_sockio_write_connect(sockio_helper* sockio_id);
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);
	
	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
