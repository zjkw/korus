#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mutex>
#include "udp_helper.h"
#include "udp_channel_base.h"

// 目前为用到self_write_size
udp_channel_base::udp_channel_base(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _fd(INVALID_SOCKET), _local_addr(local_addr), _self_read_size(self_read_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size),
	_self_read_buff(new uint8_t[self_read_size])
{

}

//析构需要发生在产生线程
udp_channel_base::~udp_channel_base()
{
	delete[]_self_read_buff;
}

bool	udp_channel_base::init_socket()
{
	// 1检查
	if (INVALID_SOCKET != _fd)
	{
		return true;
	}

	SOCKET s = INVALID_SOCKET;
	if (_local_addr.empty())
	{
		s = create_udp_socket();
	}
	else
	{
		struct sockaddr_in	si;
		if (!sockaddr_from_string(_local_addr, si))
		{
			return false;
		}

		// 2构造listen socket
		s = create_udp_socket();
		if (INVALID_SOCKET != s)
		{
			if (!bind_sock(s, si))
			{
				::close(s);
				return false;
			}
		}
	}
	if (INVALID_SOCKET == s)
	{
		return false;
	}

	// 3开启
	set_fd(s);

	return true;
}

void	udp_channel_base::set_fd(SOCKET fd)
{
	if (fd != _fd)
	{
		if (INVALID_SOCKET != fd)
		{
			assert(INVALID_SOCKET == _fd);
			if (_sock_write_size)
			{
				set_socket_sendbuflen(fd, _sock_write_size);
			}
			if (_sock_read_size)
			{
				set_socket_recvbuflen(fd, _sock_read_size);
			}
		}
		std::unique_lock <std::mutex> lck(_mutex_write);
		_fd = fd;
	}
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t		udp_channel_base::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return do_send_inlock(buf, len, peer_addr);
}

int32_t		udp_channel_base::connect(const sockaddr_in& server_addr)
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	int32_t ret = ::connect(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	else
	{
		return 0;
	}
}

int32_t		udp_channel_base::send(const void* buf, const size_t len)
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return do_send_inlock(buf, len);
}

int32_t		udp_channel_base::do_send_inlock(const void* buf, uint32_t	len, const sockaddr_in& peer_addr)
{
	while (1)
	{
		int32_t ret = ::sendto(_fd, (const char*)buf, len, 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if 0
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
#endif
			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			break;
		}
	}
	return len;
}

// tbd check udp_bind mode
int32_t		udp_channel_base::do_send_inlock(const void* buf, uint32_t	len)
{
	while (1)
	{
		int32_t ret = ::send(_fd, (const char*)buf, len, MSG_NOSIGNAL);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if 0
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
#endif
			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			break;
		}
	}
	return len;
}

int32_t	udp_channel_base::send_alone()
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	return 0;
}

void udp_channel_base::close()
{
	send_alone();
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

int32_t	udp_channel_base::do_recv()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return do_recv_nolock();	
}

// 循环中可能fd会被改变，因为on_recv_buff可能触发上层执行close，但无效fd是一个特殊值，recv是能识别出来的
int32_t	udp_channel_base::do_recv_nolock()
{
	int32_t total_read = 0;
	
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	while (true)
	{
		int32_t ret = ::recvfrom(_fd, (char*)_self_read_buff, _self_read_size, 0, (struct sockaddr*)&addr, &addr_len);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
			assert(errno != EFAULT);
			return (int32_t)CEC_READ_FAILED;		
		}
		else if (ret == 0)
		{
			break;
		}
		else
		{
			total_read += ret;
		}

		on_recv_buff(_self_read_buff, ret, addr);
	}

	return total_read;
}