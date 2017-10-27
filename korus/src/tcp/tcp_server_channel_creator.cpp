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
		it->second->invalid();
		it->second->check_detach_relation(1);
	}
	_channel_list.clear();
}

//using tcp_server_channel_factory_t = std::function<std::shared_ptr<tcp_server_handler_base>()>;
//using tcp_server_channel_factory_chain_t = std::list<tcp_server_channel_factory_t>;
template<typename T>
T chain_handler(std::shared_ptr<reactor_loop> reactor, std::list< std::function<T()> >& chain)
{	
#if 0
	std::list<T> objs;
	for (std::list< std::function<T()> >::iterator it = chain.begin(); it != chain.end(); it++)
	{
		T t = (*it)();
		if (!t)
		{
			return nullptr;
		}

		objs.push_back(t);
	}

	for (std::list<T>::iterator it = objs.begin(); it != objs.end(); it++)
	{

	}
#endif
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
			//std::shared_ptr<tcp_server_handler_base> cb = chain_handler(_reactor, _factory_chain);
			std::shared_ptr<tcp_server_channel>	channel = std::make_shared<tcp_server_channel>(fd, _reactor, cb, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			cb->inner_init(_reactor, channel);
			_channel_list[fd] = channel;

			cb->on_accept();
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
		if (!it->second->check_detach_relation(1))
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