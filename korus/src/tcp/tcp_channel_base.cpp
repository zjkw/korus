#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mutex>
#include "tcp_helper.h"
#include "tcp_channel_base.h"

tcp_channel_base::tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _fd(INVALID_SOCKET), _recving(false),
	_self_read_size(self_read_size), _self_write_size(self_write_size), _self_read_pos(0), _self_write_pos(0), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size),
	_self_read_buff(new uint8_t[self_read_size]),
	_self_write_buff(new uint8_t[self_write_size])
{
	set_fd(fd);
}

//析构需要发生在产生线程
tcp_channel_base::~tcp_channel_base()
{
	delete[]_self_read_buff;
	delete[]_self_write_buff;
}

void	tcp_channel_base::set_fd(SOCKET fd)
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
int32_t		tcp_channel_base::send(const void* buf, const size_t len)
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	// 不希望有人调用缓存区数据发送，这个应该调用send_alone
	if (_self_write_buff <= (uint8_t*)buf && _self_write_buff + _self_write_size > (uint8_t*)buf)
	{
		assert(false);
		return (int32_t)CEC_WRITE_FAILED;
	}
	
	//考虑到::send失败，而导致的粘包，我们要求剩余空间必须够
	if (len + _self_write_pos >= _self_write_size)
	{
		return (int32_t)CEC_SENDBUF_FULL;
	}
	//作为一个优化，先在条件允许的情况下避免 “入本地缓存再执行send本地数据”	
	int32_t real_write = 0;
	if (!_self_write_pos)
	{
		int32_t ret = do_send_inlock(buf, len);
		if (ret < 0)
		{
			return ret;
		}
		else
		{
			real_write += ret;
		}
	}
	int32_t errno_old = errno;	
	memcpy(_self_write_buff + _self_write_pos, (uint8_t*)buf + real_write, len - real_write);
	_self_write_pos += len - real_write;
	
	if (!real_write)		//如果没调用过send，尝试一下
	{
#if EAGAIN == EWOULDBLOCK
		if (errno_old == EAGAIN)
#else
		if (errno_old == EAGAIN || errno_old == EWOULDBLOCK)
#endif
		{
		}
		else
		{
			int32_t ret = do_send_inlock((const void*)_self_write_buff, _self_write_pos);
			if (ret >= 0)
			{
				_self_write_pos -= ret;
				memmove(_self_write_buff, _self_write_buff + ret, _self_write_pos);
			}
			else
			{
				return ret;	//小于0，无视是否存储在本地，一律认为是失败
			}
		}
	}

	return len;
}

int32_t		tcp_channel_base::do_send_inlock(const void* buf, uint32_t	len)
{
	int32_t real_write = 0;
	while (real_write != len)
	{
		int32_t ret = ::send(_fd, (const char*)buf + real_write, len - real_write, MSG_NOSIGNAL);
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

			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			real_write += ret;
		}
	}
	return real_write;
}

int32_t	tcp_channel_base::send_alone()
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	int32_t ret = do_send_inlock(_self_write_buff, _self_write_pos);
	if (ret > 0)
	{
		_self_write_pos -= ret;
		memmove(_self_write_buff, _self_write_buff + ret, _self_write_pos);
	}
	return ret;
}

void tcp_channel_base::close()
{
	send_alone();
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

void tcp_channel_base::shutdown(int32_t howto)
{
	if (INVALID_SOCKET != _fd)
	{
		::shutdown(_fd, howto);
	}
}

int32_t	tcp_channel_base::do_recv()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	//避免派生类在on_recv_buff的直接或间接调用链中调用本函数
	if (_recving)
	{
		assert(false);
		return CEC_READ_FAILED;
	}
	_recving = true;
	int32_t	ret = do_recv_nolock();	
	_recving = false;
	return ret;
}

// 循环中可能fd会被改变，因为on_recv_buff可能触发上层执行close，但无效fd是一个特殊值，recv是能识别出来的
int32_t	tcp_channel_base::do_recv_nolock()
{
	int32_t total_read = 0;
	
	bool not_again = true;
	bool peer_close = false;
	while (not_again)
	{
		int32_t real_read = 0;
		while (_self_read_size > _self_read_pos)
		{
			int32_t ret = ::recv(_fd, (char*)_self_read_buff + _self_read_pos, _self_read_size - _self_read_pos, MSG_NOSIGNAL);
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
					not_again = false;
					break;
				}
				assert(errno != EFAULT);
				return (int32_t)CEC_READ_FAILED;		//maybe Connection reset by peer / bad file desc
			}
			else if (ret == 0)
			{
				peer_close = true;
				not_again = false;
				break;
			}
			else
			{
				real_read += ret;
				_self_read_pos += (uint32_t)ret;
			}
		}
		if (!real_read)
		{
			break;
		}
	
		bool left_partial_pkg = false;
		int32_t ret = on_recv_buff(_self_read_buff, _self_read_pos, left_partial_pkg);
		if (ret > 0)
		{
			memmove(_self_read_buff, _self_read_buff + ret, _self_read_pos - ret);
			_self_read_pos -= ret;

			if (left_partial_pkg && not_again)	//既没有新数据，又解析不出新包，那就不再尝试
			{
				break;
			}
		}
	}

	if (peer_close)
	{
		return CEC_CLOSE_BY_PEER;
	}
	if (_self_read_pos == _self_read_size)
	{
		return CEC_RECVBUF_FULL;
	}

	return total_read;
}
