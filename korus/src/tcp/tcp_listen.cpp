#include <string.h>
#include <functional>

#include "korus/src/util/tcp_util.h"
#include "tcp_listen.h"

#define SCAN_STEP_ONCE	(1)

tcp_listen::tcp_listen(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor), _last_recover_scan_fd(0), _idle_helper(reactor.get())
{
	_idle_helper.bind(std::bind(&tcp_listen::on_idle_recover, this, std::placeholders::_1));
}

tcp_listen::~tcp_listen()
{
	_idle_helper.stop();
	for (std::map<SOCKET, listen_meta*>::iterator it = _listen_list.begin(); it != _listen_list.end(); it++)
	{
		delete it->second;
	}
	_listen_list.clear();	
	for (std::map<SOCKET, std::shared_ptr<tcp_server_channel>>::iterator it = _channel_list.begin(); it != _channel_list.end(); it++)
	{
		it->second->invalid();
	}
	_channel_list.clear();
}

bool	tcp_listen::add_listen(const void* key, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog, uint32_t defer_accept,
	const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
{
	// 0检查
	struct sockaddr_in	si;
	bool ret = sockaddr_from_string(listen_addr, si);
	if (!ret)
	{
		return false;
	}
	
	// 1构造listen socket
	SOCKET s = listen_nonblock_reuse_socket(backlog, defer_accept, si);
	if (INVALID_SOCKET == s)
	{
		return false;
	}

	// 2注册
	
	// 3本地缓存
	_listen_list[s] = new listen_meta(key, si, this, _reactor, cb, s, self_read_size, self_write_size, sock_read_size, sock_write_size);
	
	return true;
}

void	tcp_listen::del_listen(const void* key, const std::string& listen_addr)
{
	// 0检查
	struct sockaddr_in	si;
	if (sockaddr_from_string(listen_addr, si))
	{
		return;
	}

	for (std::map<SOCKET, listen_meta*>::iterator it = _listen_list.begin(); it != _listen_list.end(); it++)
	{
		if (key == it->second->void_ptr && !memcmp(&it->second->listen_addr, &si, sizeof(si)))
		{
			delete it->second;
			_listen_list.erase(it);
			break;
		}
	}
}

void	tcp_listen::on_sockio_accept(listen_meta* meta)
{
	while (true)
	{
		struct sockaddr_in client_addr;
		SOCKET newfd = accept_sock(meta->fd, &client_addr);
		if (newfd >= 0)
		{
			if (!set_nonblock_sock(newfd, 1))
			{
				::close(newfd);
			}
			else
			{
				std::shared_ptr<tcp_server_channel>	channel = std::make_shared<tcp_server_channel>(newfd, _reactor, meta->cb,
					meta->self_read_size, meta->self_write_size, meta->sock_read_size, meta->sock_write_size);
				_channel_list[newfd] = channel;
			}
		}
		else
		{
#if EAGAIN == EWOULDBLOCK
			if (errno == EAGAIN)
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
			else if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
			{
				continue;
			}
			else
			{
				// log errno
				break;
			}
		}
	} 

	if (_channel_list.size())
	{
		_idle_helper.start();
	}	
}

void tcp_listen::on_idle_recover(idle_helper* idle_id)
{
	if (!_channel_list.size())
	{
		_idle_helper.stop();
		return;
	}

	//找到上次位置
	std::map<SOCKET, std::shared_ptr<tcp_server_channel>>::iterator it = _channel_list.upper_bound(_last_recover_scan_fd);
	for (size_t i = 0; i < SCAN_STEP_ONCE && it != _channel_list.end();)
	{
		//就这里引用他
		if (!it->second->is_valid() && it->second.unique())
		{
			_channel_list.erase(it++);
		}
		else
		{
			i++;
		}
	}
	_last_recover_scan_fd = (it == _channel_list.end()) ? 0 : it->first;
}