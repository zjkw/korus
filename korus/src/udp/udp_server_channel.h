#pragma once

#include <list>
#include "udp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

class udp_server_handler_base;

using udp_server_channel_factory_t = std::function<std::shared_ptr<udp_server_handler_base>()>;
using udp_server_channel_factory_chain_t = std::list<udp_server_channel_factory_t>;

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//外部最好确保udp_server能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class udp_server_handler_base : public std::enable_shared_from_this<udp_server_handler_base>
{
public:
	udp_server_handler_base();
	virtual ~udp_server_handler_base();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	virtual int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual void	close();
	std::shared_ptr<reactor_loop>	reactor();
	virtual bool	can_delete(bool force, long call_ref_count);//force为真表示强制查询，比如母体退出

private:
	template<typename T> friend bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain);
	template<typename T> friend class udp_server;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_handler_base> tunnel_prev, std::shared_ptr<udp_server_handler_base> tunnel_next);
	void	inner_final();

	std::shared_ptr<reactor_loop>				_reactor;
	std::shared_ptr<udp_server_handler_base>	_tunnel_prev;
	std::shared_ptr<udp_server_handler_base>	_tunnel_next;
};

class udp_server_channel : public udp_channel_base, public udp_server_handler_base, public multiform_state
{
public:
	udp_server_channel(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_server_channel();

	// 下面四个函数可能运行在多线程环境下	
	virtual int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void		close();
	bool				start();

private:
	template<typename T> friend class udp_server;
	virtual void	set_release();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual bool	can_delete(bool force, long call_ref_count);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
