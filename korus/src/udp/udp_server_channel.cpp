#include <assert.h>
#include "udp_server_channel.h"

udp_server_channel::udp_server_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, std::shared_ptr<udp_server_handler_base> cb,
										const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
										: _reactor(reactor), _cb(cb), _sockio_helper(reactor.get()), udp_channel_base(local_addr, self_read_size, self_write_size, sock_read_size, sock_write_size)
										
{
	_sockio_helper.bind(std::bind(&udp_server_channel::on_sockio_read, this, std::placeholders::_1), nullptr);
}

//析构需要发生在产生线程
udp_server_channel::~udp_server_channel()
{
	assert(!_reactor);
	assert(!_cb);
}

bool	udp_server_channel::start()
{
	if (!is_valid())
	{
		return false;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!_reactor->is_current_thread())
	{
		// udp_client_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&udp_server_channel::start, this), this);
		return true;
	}
	bool ret = udp_channel_base::init_socket();
	if (ret)
	{
		_sockio_helper.set(_fd);
		_sockio_helper.start(SIT_READ);
		_cb->on_ready();
	}
	return ret;
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t	udp_server_channel::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}
	
	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

void	udp_server_channel::close()
{	
	if (!is_valid())
	{
		return;
	}

	//线程调度，对于服务端的链接而言，close即意味着死亡，不存在重新连接的可能性
	if (!_reactor->is_current_thread())
	{
		// udp_server_channel生命期一般比reactor短，所以加上引用计数
		_reactor->start_async_task(std::bind(&udp_server_channel::close, this), this);
		return;
	}	

	invalid();
}

void	udp_server_channel::on_sockio_read(sockio_helper* sockio_id)
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

bool	udp_server_channel::check_detach_relation(long call_ref_count)
{
	if (!is_valid())
	{
		if (_cb)
		{
			if (_cb.unique() && shared_from_this().use_count() == 1 + 1 + call_ref_count) //1 for shared_from_this, 1 for _cb
			{
				_cb->inner_final();
				_cb = nullptr;

				_reactor = nullptr;

				return true;
			}
		}
		else
		{
			assert(false);
		}
	}

	return false;
}

void	udp_server_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	thread_safe_objbase::invalid();
	_sockio_helper.clear();
	_reactor->stop_async_task(this);
	udp_channel_base::close();
	_cb->on_closed();
}

int32_t	udp_server_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}

	_cb->on_recv_pkg((uint8_t*)buf, len, peer_addr);
		
	return len;
}

void udp_server_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}