#include <assert.h>
#include "tcp_server_channel.h"

tcp_server_channel::tcp_server_channel(SOCKET fd, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_callback> cb,
										const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
										: tcp_channel_base(fd, self_read_size, self_write_size, sock_read_size, sock_write_size),
										_reactor(reactor), _cb(cb)
{
	assert(INVALID_SOCKET != fd);
	_reactor->start_sockio(this, SIT_READWRITE);
}

//析构需要发生在产生线程
tcp_server_channel::~tcp_server_channel()
{
	_reactor->stop_async_task(this);
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t	tcp_server_channel::send(const void* buf, const size_t len)
{
	if (!is_valid())
	{
		return 0;
	}
	
	int32_t ret = tcp_channel_base::send(buf, len);
	if (ret < 0)
	{
	/*
		//报告错误
		if (!_reactor->is_current_thread())
		{
			// tcp_server_channel生命期一般比reactor短，所以加上引用计数
			_reactor->start_async_task(std::bind(&tcp_server_callback::on_error, _cb, (CHANNEL_ERROR_CODE)ret, this), this);		
		}
		else
		{
			_cb->on_error((CHANNEL_ERROR_CODE)ret, shared_from_this());
		}
	*/
	}
	return ret;
}

void	tcp_server_channel::close()
{	
	if (!is_valid())
	{
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!_reactor->is_current_thread())
	{
		// tcp_server_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&tcp_server_channel::invalid, this), this);
		return;
	}	

	invalid();
}

// 参数参考全局函数 ::shutdown
void	tcp_server_channel::shutdown(int32_t howto)
{
	if (!is_valid())
	{
		return;
	}

	if (!_reactor->is_current_thread())
	{
		// tcp_server_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&tcp_server_channel::shutdown, this, howto), this);
		return;
	}
	tcp_channel_base::shutdown(howto);
}

//////////////////////////////////
void	tcp_server_channel::on_sockio_write()
{
	if (!is_valid())
	{
		return;
	}
	int32_t ret = tcp_channel_base::send_alone();
	if (ret < 0)
	{
		_cb->on_error((CHANNEL_ERROR_CODE)ret, shared_from_this());
	}
}

void	tcp_server_channel::on_sockio_read()
{
	if (!is_valid())
	{
		return;
	}
	int32_t ret = tcp_channel_base::do_recv();
	if (ret < 0)
	{
		_cb->on_error((CHANNEL_ERROR_CODE)ret, shared_from_this());
	}
}

void	tcp_server_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	thread_safe_objbase::invalid();
	_reactor->stop_sockio(this);
	tcp_channel_base::close();
	_cb->on_closed(shared_from_this());
}

int32_t	tcp_server_channel::on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg)
{
	if (!is_valid())
	{
		return 0;
	}
	left_partial_pkg = false;
	int32_t size = 0;
	while (len > size)
	{
		int32_t ret = _cb->on_recv_split((uint8_t*)buf + size, len - size, shared_from_this());
		if (ret == 0)
		{
			left_partial_pkg = true;	//剩下的不是一个完整的包
			break;
		}
		else if (ret < 0)
		{
			_cb->on_error((CHANNEL_ERROR_CODE)ret, shared_from_this());
			break;
		}
		else if (ret + size > len)
		{
			_cb->on_error(CEC_RECVBUF_SHORT, shared_from_this());
			break;
		}

		_cb->on_recv_pkg((uint8_t*)buf + size, ret, shared_from_this());
		size += ret;
	}
	
	return size;
}
