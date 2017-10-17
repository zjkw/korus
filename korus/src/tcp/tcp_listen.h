#pragma once

#include <unistd.h>
#include <string>
#include <map>
#include "tcp_server_channel.h"
#include "korus/src/reactor/idle_helper.h"

//线程安全，不会在非所属线程运行
class tcp_listen
{
public:
	// thread_obj为空表示无须跨线程调度
	tcp_listen(std::shared_ptr<reactor_loop> reactor);
	virtual ~tcp_listen();

	bool	add_listen(	const void* key, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog, uint32_t defer_accept,
						const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	void	del_listen(	const void* key, const std::string& listen_addr);
	 
private:
	class listen_meta : public sockio_channel
	{
	public:
		const void*	void_ptr;
		struct sockaddr_in	listen_addr;
		tcp_listen*	listener;
		std::shared_ptr<reactor_loop> reactor;
		uint32_t	self_read_size;
		uint32_t	self_write_size;
		uint32_t	sock_read_size;
		uint32_t	sock_write_size;
		SOCKET		fd;
		std::shared_ptr<tcp_server_callback> cb;

		listen_meta(const void* key_, const struct sockaddr_in& addr_, tcp_listen* listener_, std::shared_ptr<reactor_loop> reactor_, std::shared_ptr<tcp_server_callback> cb_, 
					SOCKET fd_, const uint32_t self_read_size_,	const uint32_t self_write_size_, const uint32_t sock_read_size_, const uint32_t sock_write_size_)
						:	void_ptr(key_), listen_addr(addr_), listener(listener_), reactor(reactor_), cb(cb_), fd(fd_), 
							self_read_size(self_read_size_), self_write_size(self_write_size_), sock_read_size(sock_read_size_), sock_write_size(sock_write_size_)
		{
			reactor->start_sockio(this, SIT_READ);
		}
		virtual ~listen_meta()
		{
			reactor->stop_sockio(this);
			::close(fd);
		}
		virtual void on_sockio_read()
		{
			listener->on_sockio_accept(this);
		}
		virtual SOCKET	get_fd()
		{
			return fd;
		}
	};

	std::shared_ptr<reactor_loop>	_reactor;
	std::map<SOCKET, listen_meta*>	_listen_list;
	std::map<SOCKET, std::shared_ptr<tcp_server_channel>>	_channel_list;
	SOCKET	_last_recover_scan_fd;	//上次扫描到的位置

	void on_sockio_accept(listen_meta* meta);
	idle_helper	_idle_helper;
	void on_idle_recover(idle_helper* idle_id);
};

