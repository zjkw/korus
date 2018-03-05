#pragma once

#include "udp_channel_base.h"
#include "korus/src/util/chain_object.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class udp_server_handler_base;

using udp_server_channel_factory_t = std::function<complex_ptr<udp_server_handler_base>(std::shared_ptr<reactor_loop> reactor)>;

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//外部最好确保udp_server能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class udp_server_handler_base : public chain_object_linkbase<udp_server_handler_base>, public obj_refbase<udp_server_handler_base>
{
public:
	udp_server_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~udp_server_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	virtual bool	start();
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data);								// 外部数据发送
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

	std::shared_ptr<reactor_loop>	reactor();

private:
	std::shared_ptr<reactor_loop>	_reactor;
};

class udp_server_handler_origin : public udp_channel_base, public udp_server_handler_base, public multiform_state
{
public:
	udp_server_handler_origin(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0);
	virtual ~udp_server_handler_origin();

	// 下面四个函数可能运行在多线程环境下	
	virtual bool	start();
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data);								// 外部数据发送
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

private:
	template<typename T> friend class udp_server;
	virtual void	set_release();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	void	on_recv_buff(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

//terminal实现
class udp_server_handler_terminal : public udp_server_handler_base, public std::enable_shared_from_this<udp_server_handler_terminal>
{
public:
	udp_server_handler_terminal(std::shared_ptr<reactor_loop> reactor);
	virtual ~udp_server_handler_terminal();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	virtual bool	start();
	virtual int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual int32_t	send(const void* buf, const size_t len);								// 外部数据发送
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

	//override------------------
	virtual void	chain_inref();
	virtual void	chain_deref();

protected:
	virtual void	on_release();

	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual	void	send(const std::shared_ptr<buffer_thunk>& data);
};