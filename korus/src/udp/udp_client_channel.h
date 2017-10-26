#pragma once

#include "udp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//外部最好确保udp_client能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

class udp_client_handler_base;
//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作
class udp_client_channel : public std::enable_shared_from_this<udp_client_channel>, public thread_safe_objbase, public udp_channel_base
{
public:
	udp_client_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_handler_base> cb, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_client_channel();

	// 下面四个函数可能运行在多线程环境下	
	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	void		close();
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }
	bool		start();

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<udp_client_handler_base>	_cb;

	template<typename T> friend class udp_client;
	// channel要析构/回收要看handler是否还没其他对象持有引用
	bool		check_detach_relation(long call_ref_count);	//true表示已经互相解除关系
	void		invalid();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class udp_client_handler_base : public std::enable_shared_from_this<udp_client_handler_base>, public thread_safe_objbase
{
public:
	udp_client_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~udp_client_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_init() = 0;
	virtual void	on_final() = 0;
	virtual void	on_ready() = 0;	//就绪
	virtual void	on_closed() = 0;
	//参考TCP_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE codel) = 0;
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr) = 0;

protected:
	int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr)	{ if (!_channel) return CEC_INVALID_SOCKET;  return _channel->send(buf, len, peer_addr); }
	void	close()									{ if (_channel)_channel->close(); }
	std::shared_ptr<reactor_loop>	get_reactor()	{ return _reactor; }

private:
	template<typename T> friend class udp_client;
	friend class udp_client_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_channel> channel)
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
	std::shared_ptr<udp_client_channel> _channel;
};