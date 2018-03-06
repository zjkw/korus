#include <assert.h>
#include "tcp_server_channel.h"

///////////////////////////base
tcp_server_handler_base::tcp_server_handler_base(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor)
{
}

tcp_server_handler_base::~tcp_server_handler_base()
{
	// 必须执行chain_final
	assert(!_tunnel_next);
	assert(!_tunnel_prev);
}

//override------------------
void	tcp_server_handler_base::on_chain_init()
{
}

void	tcp_server_handler_base::on_chain_final() 
{
}

void	tcp_server_handler_base::on_accept()	
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_accept();
}

void	tcp_server_handler_base::on_closed()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_closed();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	tcp_server_handler_base::on_error(CHANNEL_ERROR_CODE code)	
{
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_next->on_error(code);
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t tcp_server_handler_base::on_recv_split(const std::shared_ptr<buffer_thunk>& data)
{
	if (!_tunnel_next)
	{
		assert(false);
		return CEC_SPLIT_FAILED;
	}

	return _tunnel_next->on_recv_split(data);
}

//这是一个待处理的完整包
void	tcp_server_handler_base::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data)
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	return _tunnel_next->on_recv_pkg(data);
}


int32_t	tcp_server_handler_base::send(const std::shared_ptr<buffer_thunk>& data)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	return _tunnel_prev->send(data);
}

void	tcp_server_handler_base::close()								
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->close();
}

void	tcp_server_handler_base::shutdown(int32_t howto)				
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->shutdown(howto);
}

std::shared_ptr<reactor_loop>	tcp_server_handler_base::reactor()
{
	return _reactor;
}

bool	tcp_server_handler_base::peer_addr(std::string& addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}

	_tunnel_prev->peer_addr(addr);
}

bool	tcp_server_handler_base::local_addr(std::string& addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}

	_tunnel_prev->local_addr(addr);
}

///////////////////////////channel
tcp_server_handler_origin::tcp_server_handler_origin(std::shared_ptr<reactor_loop> reactor, SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: tcp_server_handler_base(reactor), tcp_channel_base(fd, self_read_size, self_write_size, sock_read_size, sock_write_size)
										
{
	assert(INVALID_SOCKET != fd);
	_sockio_helper.set(fd);
	_sockio_helper.bind(std::bind(&tcp_server_handler_origin::on_sockio_read, this, std::placeholders::_1), std::bind(&tcp_server_handler_origin::on_sockio_write, this, std::placeholders::_1));

	set_prepare();
}

//析构需要发生在产生线程
tcp_server_handler_origin::~tcp_server_handler_origin()
{
}

bool tcp_server_handler_origin::start()
{
	if (!is_prepare())
	{
		assert(false);
		return false;
	}
	if (!reactor())
	{
		assert(false);
		return false;
	}

	_sockio_helper.reactor(reactor().get());
	_sockio_helper.start(SIT_READWRITE);

	set_normal();

	return true;
}

int32_t	tcp_server_handler_origin::send(const std::shared_ptr<buffer_thunk>& data)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	return tcp_channel_base::send_raw(data);
}

void	tcp_server_handler_origin::close()
{	
	if (is_dead())
	{
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_handler_origin::close, this), this);
		return;
	}	

	set_release();
}

// 参数参考全局函数 ::shutdown
void	tcp_server_handler_origin::shutdown(int32_t howto)
{
	if (!is_prepare() && !is_normal())
	{
		return;
	}

	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_handler_origin::shutdown, this, howto), this);
		return;
	}
	tcp_channel_base::shutdown(howto);
}

//////////////////////////////////
void	tcp_server_handler_origin::on_sockio_write(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}
	int32_t ret = tcp_channel_base::send_alone();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	tcp_server_handler_origin::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}
	int32_t ret = tcp_channel_base::do_recv();
	if (ret < 0)
	{
		if (is_normal())
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);			
		}
	}
}

void	tcp_server_handler_origin::set_release()
{
	if (is_dead())
	{
		return;
	}

	multiform_state::set_release();

	_sockio_helper.clear();
	reactor()->stop_async_task(this);
	tcp_channel_base::close();
	on_closed();
}

int32_t	tcp_server_handler_origin::on_recv_buff(std::shared_ptr<buffer_thunk>& data, bool& left_partial_pkg)
{
	if (!is_normal())
	{
		return CEC_INVALID_SOCKET;
	}
	left_partial_pkg = false;
	int32_t size = 0;
	while (data->used())
	{
		int32_t ret = on_recv_split(data);
		if (!is_normal())
		{
			return CEC_INVALID_SOCKET;
		}
		if (ret == 0)
		{
			left_partial_pkg = true;	//剩下的不是一个完整的包
			break;
		}
		else if (ret < 0)
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
			break;
		}
		else if (ret > data->used())
		{
			CLOSE_MODE_STRATEGY cms = on_error(CEC_RECVBUF_SHORT);
			handle_close_strategy(cms);
			break;
		}

		data->rpos(ret);
		on_recv_pkg(data);
		data->pop_front(ret);
		size += ret;

		if (!is_normal())
		{
			return CEC_INVALID_SOCKET;
		}
	}
	
	return size;
}

void tcp_server_handler_origin::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}

bool	tcp_server_handler_origin::peer_addr(std::string& addr)
{
	if (!is_normal())
	{
		return false;
	}
	if (!reactor()->is_current_thread())
	{
		return false;
	}
	return tcp_channel_base::peer_addr(addr);
}

bool	tcp_server_handler_origin::local_addr(std::string& addr)
{
	if (!is_normal())
	{
		return false;
	}
	if (!reactor()->is_current_thread())
	{
		return false;
	}
	return tcp_channel_base::local_addr(addr);
}

//////////////
tcp_server_handler_terminal::tcp_server_handler_terminal(std::shared_ptr<reactor_loop> reactor) : tcp_server_handler_base(reactor)
{

}

tcp_server_handler_terminal::~tcp_server_handler_terminal()
{
	chain_final();
}

//override------------------
void	tcp_server_handler_terminal::on_chain_init()
{

}

void	tcp_server_handler_terminal::on_chain_final()
{

}

void	tcp_server_handler_terminal::on_accept()
{

}

void	tcp_server_handler_terminal::on_closed()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	tcp_server_handler_terminal::on_error(CHANNEL_ERROR_CODE code)
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t tcp_server_handler_terminal::on_recv_split(const void* buf, const size_t len)
{

}

//这是一个待处理的完整包
void	tcp_server_handler_terminal::on_recv_pkg(const void* buf, const size_t len)
{

}

int32_t tcp_server_handler_terminal::on_recv_split(const std::shared_ptr<buffer_thunk>& data)
{
	return on_recv_split(data->ptr(), data->used());
}

void	tcp_server_handler_terminal::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data)
{
	on_recv_pkg(data->ptr(), data->rpos());
}

int32_t	tcp_server_handler_terminal::send(const void* buf, const size_t len)
{
	std::shared_ptr<buffer_thunk>	thunk = std::make_shared<buffer_thunk>(buf, len);
	return tcp_server_handler_terminal::send(thunk);
}

int32_t	tcp_server_handler_terminal::send(const std::shared_ptr<buffer_thunk>& data)
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_handler_terminal::send_async, this, data), this);
		return data->used();
	}
	return tcp_server_handler_base::send(data);
}

void	tcp_server_handler_terminal::send_async(const std::shared_ptr<buffer_thunk>& data)
{
	int32_t ret = tcp_server_handler_base::send(data);
	if (ret < 0)
	{
		tcp_server_handler_base::close();
	}
}

void	tcp_server_handler_terminal::close()
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_handler_terminal::close, this), this);
		return;
	}
	tcp_server_handler_base::close();
}

void	tcp_server_handler_terminal::shutdown(int32_t howto)
{
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_handler_terminal::shutdown, this, howto), this);
		return;
	}
	tcp_server_handler_base::shutdown(howto);
}

bool	tcp_server_handler_terminal::peer_addr(std::string& addr)
{
	return tcp_server_handler_base::peer_addr(addr);
}

bool	tcp_server_handler_terminal::local_addr(std::string& addr)
{
	return tcp_server_handler_base::local_addr(addr);
}

//override------------------
void	tcp_server_handler_terminal::chain_inref()
{
	this->shared_from_this().inref();
}

void	tcp_server_handler_terminal::chain_deref()
{
	this->shared_from_this().deref();
}

void	tcp_server_handler_terminal::transfer_ref(const int32_t& ref)
{
	if (ref > 0)
	{
		for (int32_t i = 0; i < ref; i++)
		{
			this->shared_from_this().inref();
		}
	}
	else if(ref < 0)
	{
		for (int32_t i = 0; i > ref; i--)
		{
			this->shared_from_this().deref();
		}
	}
}

void	tcp_server_handler_terminal::on_release()
{
	//默认删除,屏蔽之
}

