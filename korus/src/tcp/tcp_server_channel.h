#pragma once

#include <assert.h>
#include <list>
#include "tcp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

class tcp_server_handler_base;

using tcp_server_channel_factory_t = std::function<std::shared_ptr<tcp_server_handler_base>()>;
using tcp_server_channel_factory_chain_t = std::list<tcp_server_channel_factory_t>;

//这个类用户可以操作，而且是可能多线程环境下操作，对外是shared_ptr，需要保证线程安全
//考虑到send可能在工作线程，close在主线程，应排除同时进行操作，所以仅仅此两个进行了互斥，带来的坏处：
//1，是send在最恶劣情况下仅仅是拷贝到本库的缓存，并非到了内核缓存，不同于::send的语义
//2，close/shudown都可能是跨线程的，导致延迟在fd所在线程执行，此情况下无法做到实时效果，比如外部先close后可能还能send

//外部最好确保tcp_server能包裹channel/handler生命期，这样能保证资源回收，否则互相引用的两端(channel和handler)没有可设计的角色来触发回收时机的函数调用check_detach_relation

// 可能处于多线程环境下
// on_error不能纯虚 tbd，加上close默认处理
class tcp_server_handler_base : public std::enable_shared_from_this<tcp_server_handler_base>, public double_state
{
public:
	tcp_server_handler_base() : _reactor(nullptr), _tunnel_prev(nullptr), _tunnel_next(nullptr){}
	virtual ~tcp_server_handler_base(){ assert(!_tunnel_prev); assert(!_tunnel_next); }	// 必须执行inner_final

	//override------------------
	virtual void	on_init() {}
	virtual void	on_final() {}
	virtual void	on_accept()	{ if (_tunnel_prev)_tunnel_prev->on_accept(); }
	virtual void	on_closed()	{ if (_tunnel_prev)_tunnel_prev->on_closed(); }
	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)	{ if (!_tunnel_prev) return CMS_INNER_AUTO_CLOSE; return _tunnel_prev->on_error(code); }
	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len)	{ if (!_tunnel_prev) return 0; return _tunnel_prev->on_recv_split(buf, len); }
	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len)	{ if (!_tunnel_prev) return; return _tunnel_prev->on_recv_pkg(buf, len); }

	virtual int32_t	send(const void* buf, const size_t len)	{ if (!_tunnel_next) return CEC_INVALID_SOCKET; return _tunnel_next->send(buf, len); }
	virtual void	close()									{ if (_tunnel_next)_tunnel_next->close(); }
	virtual void	shutdown(int32_t howto)					{ if (_tunnel_next)_tunnel_next->shutdown(howto); }

	std::shared_ptr<reactor_loop>	reactor()	{ return _reactor; }
	virtual bool	can_delete(long call_ref_count)			//端头如果有别的对象引用此，需要重载
	{
		if (is_valid())
		{
			return false;
		}
		long ref = 0;
		if (_tunnel_prev)
		{
			ref++;
		}
		if (_tunnel_next)
		{
			ref++;
		}
		if (call_ref_count + ref + 1 == shared_from_this().use_count())
		{
			// 成立，尝试向上查询
			if (_tunnel_prev)
			{
				return _tunnel_prev->can_delete(0);
			}
			else
			{
				return true;
			}
		}

		return false;
	}
private:
	template<typename T> friend bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain);
	friend class tcp_server_channel_creator;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_handler_base> tunnel_prev, std::shared_ptr<tcp_server_handler_base> tunnel_next)
	{
		_reactor = reactor;
		_tunnel_prev = tunnel_prev;
		_tunnel_next = tunnel_next;

		on_init();
	}
	void	inner_final()
	{
		if (_tunnel_prev)
		{
			_tunnel_prev->inner_final();
		}
		_reactor = nullptr;
		_tunnel_prev = nullptr;
		_tunnel_next = nullptr;

		on_final();
	}	

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_server_handler_base> _tunnel_prev;
	std::shared_ptr<tcp_server_handler_base> _tunnel_next;
};

//有效性优先级：is_valid > INVALID_SOCKET,即所有函数都会先判断is_valid这是个原子操作
class tcp_server_channel : public tcp_channel_base, public tcp_server_handler_base
{
public:
	tcp_server_channel(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	virtual bool		start();
	// 下面三个函数可能运行在多线程环境下	
	virtual int32_t		send(const void* buf, const size_t len);// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
	virtual void		close();
	virtual void		shutdown(int32_t howto);// 参数参考全局函数 ::shutdown

private:
	friend class tcp_server_channel_creator;
	void		invalid();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
