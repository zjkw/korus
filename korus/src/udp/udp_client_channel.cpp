#include <assert.h>
#include "udp_client_channel.h"

udp_client_channel::udp_client_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_handler_base> cb,
	const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _reactor(reactor), _cb(cb),
	udp_channel_base("", self_read_size, self_write_size, sock_read_size, sock_write_size)

{
}

//析构需要发生在产生线程
udp_client_channel::~udp_client_channel()
{
	assert(!_reactor);
	assert(!_cb);
}

bool	udp_client_channel::start()
{
	if (!is_valid())
	{
		return false;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!_reactor->is_current_thread())
	{
		// udp_client_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&udp_client_channel::start, this), this);
		return true;
	}

	bool ret = udp_channel_base::init_socket();
	if (ret)
	{
		_reactor->start_sockio(this, SIT_READ);
		_cb->on_ready();
	}
	return ret;
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t	udp_client_channel::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}

	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

void	udp_client_channel::close()
{
	if (!is_valid())
	{
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!_reactor->is_current_thread())
	{
		// udp_client_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&udp_client_channel::close, this), this);
		return;
	}

	invalid();
}

//////////////////////////////////
void	udp_client_channel::on_sockio_write()
{
	assert(false); //不存在
}

void	udp_client_channel::on_sockio_read()
{
	if (!is_valid())
	{
		return;
	}
	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = _cb->on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	udp_client_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	thread_safe_objbase::invalid();
	_reactor->stop_sockio(this);
	udp_channel_base::close();
	_cb->on_closed();

	detach();
}

void	udp_client_channel::detach()
{
	if (is_valid())
	{
		assert(false);
		return;
	}

	if (_cb)
	{
		_cb->inner_uninit();
		_cb = nullptr;
	}
	if (_reactor)
	{
		_reactor->stop_sockio(this);
		_reactor->stop_async_task(this);
		_reactor = nullptr;
	}
}

int32_t	udp_client_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}

	_cb->on_recv_pkg((uint8_t*)buf, len, peer_addr);

	return len;
}

void udp_client_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}