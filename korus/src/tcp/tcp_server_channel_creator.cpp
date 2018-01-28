#include <unistd.h>
#include "tcp_helper.h"
#include "tcp_server_channel_creator.h"
#include "korus/src/util/unique_optional_lock.h"

#define SCAN_STEP_ONCE	(1)

tcp_server_channel_creator::tcp_server_channel_creator(std::shared_ptr<reactor_loop> reactor, const tcp_server_channel_factory_t& factory,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
		: _reactor(reactor), _factory(factory), _idle_helper(reactor.get()), _last_recover_scan_fd(0),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
{
	_idle_helper.bind(std::bind(&tcp_server_channel_creator::on_idle_recover, this, std::placeholders::_1));
}

tcp_server_channel_creator::~tcp_server_channel_creator()
{
	_reactor->stop_async_task(this);
	_idle_helper.stop();
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
			tcp_server_handler_origin*	origin_channel = new tcp_server_handler_origin(_reactor, fd, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			std::shared_ptr<tcp_server_handler_base>		terminal_channel = _factory(_reactor);
			build_channel_chain_helper((tcp_server_handler_base*)origin_channel, (tcp_server_handler_base*)terminal_channel.get());
			chain_object_sharedwrapper<tcp_server_handler_origin>	wrap(origin_channel);
			_channel_list[fd] = wrap;
			origin_channel->start();
			origin_channel->on_accept();
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
	std::map<SOCKET, chain_object_sharedwrapper<tcp_server_handler_origin>>::iterator it = _channel_list.upper_bound(_last_recover_scan_fd);
	if (it == _channel_list.end())
	{
		it = _channel_list.begin();
	}
	for (size_t i = 0; i < SCAN_STEP_ONCE && it != _channel_list.end();)
	{
		// 无效才可剔除，引用为1表示仅仅tcp_server_channel_creator引用这个channel，而channel是这个对象创建（同线程）
		if (it->second->is_release())	//origin_channel_list + terminal
		{

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