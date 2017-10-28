#include <assert.h>
#include "udp_client_channel.h"

udp_client_channel::udp_client_channel(const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: udp_channel_base("", self_read_size, self_write_size, sock_read_size, sock_write_size)

{
	_sockio_helper.bind(std::bind(&udp_client_channel::on_sockio_read, this, std::placeholders::_1), nullptr);
}

//析构需要发生在产生线程
udp_client_channel::~udp_client_channel()
{
}

bool	udp_client_channel::start()
{
	if (!is_valid())
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
		_sockio_helper.set(_fd);
		_sockio_helper.start(SIT_READ);
		on_ready();
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

	invalid();
}

void	udp_client_channel::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_valid())
	{
		return;
	}
	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
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
	_sockio_helper.clear();
	reactor()->stop_async_task(this);
	udp_channel_base::close();
	on_closed();
}

int32_t	udp_client_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
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