#include <assert.h>
#include "udp_client_channel.h"

/////////////////////////base
udp_client_handler_base::udp_client_handler_base(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor)
{
}

udp_client_handler_base::~udp_client_handler_base()
{ 
}	

//override------------------
void	udp_client_handler_base::on_chain_init()
{
}

void	udp_client_handler_base::on_chain_final()
{
}

void	udp_client_handler_base::on_chain_zomby()
{

}

void	udp_client_handler_base::on_ready()	
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	
	_tunnel_next->on_ready(); 
}

void	udp_client_handler_base::on_closed()	
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_closed();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	udp_client_handler_base::on_error(CHANNEL_ERROR_CODE code)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_next->on_error(code); 
}

//这是一个待处理的完整包
void	udp_client_handler_base::on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_recv_pkg(buf, len, peer_addr); 
}

bool	udp_client_handler_base::start()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	return _tunnel_prev->start();
}

void	udp_client_handler_base::close()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	
	_tunnel_prev->close();
}

std::shared_ptr<reactor_loop>	udp_client_handler_base::reactor()	
{ 
	return _reactor; 
}

////////////////////////channel
udp_client_channel::udp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& bind_addr/* = ""*/, const uint32_t self_read_size/* = DEFAULT_READ_BUFSIZE*/, const uint32_t self_write_size/* = DEFAULT_WRITE_BUFSIZE*/, const uint32_t sock_read_size/* = 0*/, const uint32_t sock_write_size/* = 0*/)
	: udp_client_handler_base(reactor),
	udp_channel_base(bind_addr, self_read_size, self_write_size, sock_read_size, sock_write_size)

{
	_sockio_helper.bind(std::bind(&udp_client_channel::on_sockio_read, this, std::placeholders::_1), nullptr);
	set_prepare();
}

//析构需要发生在产生线程
udp_client_channel::~udp_client_channel()
{
}

bool	udp_client_channel::start()
{
	if (!is_prepare())
	{
		return false;
	}

	if (!reactor())
	{
		assert(false);
		return false;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!reactor()->is_current_thread())
	{
		// udp_client_channel生命期一般比reactor短，所以加上引用计数
		reactor()->start_async_task(std::bind(&udp_client_channel::start, this), this);
		return true;
	}
	
	_sockio_helper.reactor(reactor().get());

	bool ret = udp_channel_base::init_socket();
	if (ret)
	{
		set_normal();
		_sockio_helper.set(_fd);
		_sockio_helper.start(SIT_READ);
		on_ready();
	}
	return ret;
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t	udp_client_channel::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

int32_t	udp_client_channel::connect(const sockaddr_in& server_addr)								// 保证原子, 返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::connect(server_addr);
	return ret;
}

int32_t	udp_client_channel::send(const void* buf, const size_t len)								// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::send(buf, len);
	return ret;
}

void	udp_client_channel::close()
{
	if (!reactor())
	{
		assert(false);
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!reactor()->is_current_thread())
	{
		// udp_client_channel生命期一般比reactor短，所以加上引用计数
		reactor()->start_async_task(std::bind(&udp_client_channel::close, this), this);
		return;
	}

	set_release();
}

void	udp_client_channel::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		if (is_normal())
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);		
		}
	}
}

void	udp_client_channel::set_release()
{
	if (is_release() || is_dead())
	{
		assert(false);
		return;
	}

	multiform_state::set_release();
	_sockio_helper.clear();
	reactor()->stop_async_task(this);
	udp_channel_base::close();
	on_closed();
}

int32_t	udp_client_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	on_recv_pkg((uint8_t*)buf, len, peer_addr);

	return len;
}

void udp_client_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}
