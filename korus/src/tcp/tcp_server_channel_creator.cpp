#include <unistd.h>
#include "tcp_helper.h"
#include "tcp_server_channel_creator.h"
#include "korus/src/util/unique_optional_lock.h"

#define SCAN_STEP_ONCE	(1)

tcp_server_channel_creator::tcp_server_channel_creator(std::shared_ptr<reactor_loop> reactor, const tcp_server_channel_factory_chain_t& factory_chain,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
		: _reactor(reactor), _factory_chain(factory_chain), _idle_helper(reactor.get()), _last_recover_scan_fd(0),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
{
	_idle_helper.bind(std::bind(&tcp_server_channel_creator::on_idle_recover, this, std::placeholders::_1));
}

tcp_server_channel_creator::~tcp_server_channel_creator()
{
	_reactor->stop_async_task(this);
	_idle_helper.stop();
	for (std::map<SOCKET, std::shared_ptr<tcp_server_channel>>::iterator it = _channel_list.begin(); it != _channel_list.end(); it++)
	{
		it->second->set_release();
		if (!it->second->can_delete(true, 1))
		{
			it->second->inner_final();
		}
	}
	_channel_list.clear();
}

void tcp_server_channel_creator::on_newfd(const SOCKET fd, const struct sockaddr_in& addr)
{
	if (!_reactor->is_current_thread())
	{
		_reactor->start_async_task(std::bind(&tcp_server_channel_creator::on_newfd, this, fd, addr), this);
		return;
	}

	if (fd >= 0)
	{
		if (!set_nonblock_sock(fd, 1))
		{
			::close(fd);
		}
		else
		{
			std::shared_ptr<tcp_server_handler_base> cb = _factory_chain.front()();
			std::shared_ptr<tcp_server_channel>	channel = std::make_shared<tcp_server_channel>(fd, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			build_chain(_reactor, std::dynamic_pointer_cast<tcp_server_handler_base>(channel), _factory_chain);
			_channel_list[fd] = channel;
			channel->start();
			channel->on_accept();
		}

		if (_channel_list.size())
		{
			if (!_idle_helper.exist())
			{
				_idle_helper.start();
			}
		}
	}
}

void tcp_server_channel_creator::on_idle_recover(idle_helper* idle_id)
{
	//找到上次位置
	std::map<SOCKET, std::shared_ptr<tcp_server_channel>>::iterator it = _channel_list.upper_bound(_last_recover_scan_fd);
	if (it == _channel_list.end())
	{
		it = _channel_list.begin();
	}
	for (size_t i = 0; i < SCAN_STEP_ONCE && it != _channel_list.end(); i++)
	{
		// 无效才可剔除，引用为1表示仅仅tcp_server_channel_creator引用这个channel，而channel是这个对象创建（同线程）
		if (!it->second->can_delete(false, 1))
		{
			it->second->inner_final();
			_channel_list.erase(it++);
		}
		else
		{
			it++;
		}
	}
	_last_recover_scan_fd = (it == _channel_list.end()) ? 0 : it->first;

	if (!_channel_list.size())
	{
		_idle_helper.stop();
	}
}