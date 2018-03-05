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
void	udp_client_handler_base::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_recv_pkg(data, peer_addr); 
}

bool	udp_client_handler_base::start()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}
	
	return _tunnel_prev->start();
}

void	udp_client_handler_base::send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->send(data, peer_addr);
}

int32_t	udp_client_handler_base::connect(const sockaddr_in& server_addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return 0;
	}

	return _tunnel_prev->connect(server_addr);
}

void	udp_client_handler_base::send(const std::shared_ptr<buffer_thunk>& data)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	return _tunnel_prev->send(data);
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
udp_client_handler_origin::udp_client_handler_origin(std::shared_ptr<reactor_loop> reactor, const std::string& bind_addr/* = ""*/, const uint32_t self_read_size/* = DEFAULT_READ_BUFSIZE*/, const uint32_t self_write_size/* = DEFAULT_WRITE_BUFSIZE*/, const uint32_t sock_read_size/* = 0*/, const uint32_t sock_write_size/* = 0*/)
	: udp_client_handler_base(reactor),
	udp_channel_base(bind_addr, self_read_size, self_write_size, sock_read_size, sock_write_size)
{
	_sockio_helper.bind(std::bind(&udp_client_handler_origin::on_sockio_read, this, std::placeholders::_1), nullptr);
	set_prepare();
}

//析构需要发生在产生线程
udp_client_handler_origin::~udp_client_handler_origin()
{
}

bool	udp_client_handler_origin::start()
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
		// udp_client_handler_origin生命期一般比reactor短，所以加上引用计数
		reactor()->start_async_task(std::bind(&udp_client_handler_origin::start, this), this);
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
void	udp_client_handler_origin::send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	udp_channel_base::send_raw(data, peer_addr);	
}

int32_t	udp_client_handler_origin::connect(const sockaddr_in& server_addr)								// 保证原子, 返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::connect(server_addr);
	return ret;
}

void	udp_client_handler_origin::send(const std::shared_ptr<buffer_thunk>& data)								// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	udp_channel_base::send_raw(data);	
}

void	udp_client_handler_origin::close()
{
	if (!reactor())
	{
		assert(false);
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!reactor()->is_current_thread())
	{
		// udp_client_handler_origin生命期一般比reactor短，所以加上引用计数
		reactor()->start_async_task(std::bind(&udp_client_handler_origin::close, this), this);
		return;
	}

	set_release();
}

void	udp_client_handler_origin::on_sockio_read(sockio_helper* sockio_id)
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

void	udp_client_handler_origin::set_release()
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

void	udp_client_handler_origin::on_recv_buff(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	on_recv_pkg(data, peer_addr);
}

void udp_client_handler_origin::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}

/////////////////
udp_client_handler_terminal::udp_client_handler_terminal(std::shared_ptr<reactor_loop> reactor) : udp_client_handler_base(reactor)
{

}

udp_client_handler_terminal::~udp_client_handler_terminal()
{
	chain_final();
}

//override------------------
void	udp_client_handler_terminal::on_chain_init()
{

}

void	udp_client_handler_terminal::on_chain_final()
{

}

void	udp_client_handler_terminal::on_ready()
{

}

void	udp_client_handler_terminal::on_closed()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	udp_client_handler_terminal::on_error(CHANNEL_ERROR_CODE code)
{
	return CMS_INNER_AUTO_CLOSE;
}

bool	udp_client_handler_terminal::start()
{

}

//这是一个待处理的完整包
void	udp_client_handler_terminal::on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{

}

void	udp_client_handler_terminal::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	std::shared_ptr<buffer_thunk>	thunk = std::make_shared<buffer_thunk>(buf, len);
	udp_client_handler_terminal::send(thunk, peer_addr);
}

int32_t	udp_client_handler_terminal::connect(const sockaddr_in& server_addr)								// 保证原子, 返回值若<0参考CHANNEL_ERROR_CODE
{
	return udp_client_handler_base::connect(server_addr);
}

void	udp_client_handler_terminal::send(const void* buf, const size_t len)
{
	std::shared_ptr<buffer_thunk>	thunk = std::make_shared<buffer_thunk>(buf, len);
	udp_client_handler_terminal::send(thunk);
}

void	udp_client_handler_terminal::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	on_recv_pkg(data->ptr(), data->used(), peer_addr);
}

void	udp_client_handler_terminal::send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(static_cast<void (udp_client_handler_terminal::*)(const std::shared_ptr<buffer_thunk>&, const sockaddr_in&)>(&udp_client_handler_terminal::send), this, data, peer_addr), this);
		return;
	}
	udp_client_handler_base::send(data, peer_addr);
}

void	udp_client_handler_terminal::send(const std::shared_ptr<buffer_thunk>& data)
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(static_cast<void (udp_client_handler_terminal::*)(const std::shared_ptr<buffer_thunk>&)>(&udp_client_handler_terminal::send), this, data), this);
		return;
	}
	udp_client_handler_base::send(data);
}

void	udp_client_handler_terminal::close()
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&udp_client_handler_terminal::close, this), this);
		return;
	}
	udp_client_handler_base::close();
}

//override------------------
void	udp_client_handler_terminal::chain_inref()
{
	this->shared_from_this().inref();
}

void	udp_client_handler_terminal::chain_deref()
{
	this->shared_from_this().deref();
}

void	udp_client_handler_terminal::on_release()
{
	//默认删除,屏蔽之
}