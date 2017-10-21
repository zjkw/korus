#include <string.h>
#include <functional>

#include "korus/src/util/tcp_util.h"
#include "tcp_listen.h"

#define SCAN_STEP_ONCE	(1)

tcp_listen::tcp_listen(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, uint32_t backlog, uint32_t defer_accept)
	: _reactor(reactor), _fd(INVALID_SOCKET), _listen_addr(listen_addr), _backlog(backlog), _defer_accept(defer_accept), _last_pos(0)
{

}

tcp_listen::~tcp_listen()
{
	_reactor->stop_async_task(this);
	_reactor->stop_sockio(this);
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}	
}

bool	tcp_listen::start()
{
	// 1检查
	if (INVALID_SOCKET != _fd)
	{
		return true;
	}

	struct sockaddr_in	si;
	if (!sockaddr_from_string(_listen_addr, si))
	{
		return false;
	}
	
	// 2构造listen socket
	SOCKET s = listen_nonblock_reuse_socket(_backlog, _defer_accept, si);
	if (INVALID_SOCKET == s)
	{
		return false;
	}

	// 3开启
	_fd = s;
	_reactor->start_sockio(this, SIT_READ);
	
	return true;
}

void	tcp_listen::add_accept_handler(const newfd_handle_t handler)
{
	if (!_reactor->is_current_thread())
	{
		_reactor->start_async_task(std::bind(&tcp_listen::add_accept_handler, this, handler), this);
		return;
	}

	_handler_list.emplace_back(handler);
}

void tcp_listen::on_sockio_read()
{
	while (true)
	{
		struct sockaddr_in client_addr;
		SOCKET newfd = accept_sock(_fd, &client_addr);
		if (newfd >= 0)
		{
			if (!_handler_list.size())
			{
				//log
				::close(newfd);
			}
			else
			{
				_last_pos %= _handler_list.size();
				_handler_list[_last_pos](newfd, client_addr);
				_last_pos++;
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
}

SOCKET	tcp_listen::get_fd()
{
	return _fd;
}


