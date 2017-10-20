#pragma once

#include <unistd.h>
#include <string>
#include <vector>
#include "korus/src/util/thread_safe_objbase.h"
#include "korus/src/reactor/reactor_loop.h"

using	newfd_handle_t = std::function<void(const SOCKET fd, const struct sockaddr_in& addr)>;

//线程安全，不会在非所属线程运行
class tcp_listen : public sockio_channel, public noncopyable
{
public:
	// thread_obj为空表示无须跨线程调度
	tcp_listen(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, uint32_t backlog, uint32_t defer_accept);
	virtual ~tcp_listen();

	bool	start();
	void	add_accept_handler(const newfd_handle_t handler);
	 
private:
	virtual void on_sockio_read();
	virtual SOCKET	get_fd();
	
	std::shared_ptr<reactor_loop>	_reactor;
	SOCKET		_fd;
	std::string _listen_addr;
	uint32_t	_backlog;
	uint32_t	_defer_accept;

	uint16_t	_last_pos;
	std::vector<newfd_handle_t>	_handler_list;
};

