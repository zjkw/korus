#include <fcntl.h>
#include <unistd.h>
#include "tcp_util.h"

bool split_hostport(const std::string& address, std::string& host, int32_t& port)
{
	std::string a = address;

	size_t index = a.rfind(':');
	if (index == std::string::npos) 
	{
//		LOG_ERROR << "Address specified error <" << address << ">. Cannot find ':'";
		return false;
	}

	if (index + 1 == a.size()) 
	{
		return false;
	}

	port = std::atoi(&a[index + 1]);
	host = std::string(address, index);
	if (host[0] == '[') 
	{
		if (*host.rbegin() != ']') 
		{
	//		LOG_ERROR << "Address specified error <" << address << ">. '[' ']' is not pair.";
			return false;
		}

		// trim the leading '[' and trail ']'
		host = std::string(host.data() + 1, host.size() - 2);
	}

	// Compatible with "fe80::886a:49f3:20f3:add2]:80"
	if (*host.rbegin() == ']') 
	{
		// trim the trail ']'
		host = std::string(host.data(), host.size() - 1);
	}

	return true;
}

bool sockaddr_from_string(const std::string& address, struct sockaddr_in& si)
{
	std::string host;
	int32_t port = 0;
	if (!split_hostport(address, host, port)) 
	{
		return false;
	}

	short family = AF_INET;
	auto index = host.find(':');
	if (index != std::string::npos) 
	{
		family = AF_INET6;
	}

	int rc = ::inet_pton(family, host.data(), &si.sin_addr);
	if (rc == 0) 
	{
//		LOG_INFO << "ParseFromIPPort inet_pton(AF_INET '" << host.data() << "', ...) rc=0. " << host.data() << " is not a valid IP address. Maybe it is a hostname.";
		return false;
	}
	else if (rc < 0) 
	{
		int serrno = errno;
		if (serrno == 0) 
		{
//			LOG_INFO << "[" << host.data() << "] is not a IP address. Maybe it is a hostname.";
		}
		else 
		{
//			LOG_WARN << "ParseFromIPPort inet_pton(AF_INET, '" << host.data() << "', ...) failed : " << strerror(serrno);
		}
		return false;
	}

	si.sin_family = family;
	si.sin_port = htons(port);

	return true;
}

SOCKET create_tcp_sock()
{
	return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

bool set_nonblock_sock(SOCKET fd, unsigned int nonblock)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		return false;
	}
	if (nonblock > 0)
	{
		return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
	else
	{
		return fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
	}
}

bool set_linger_sock(SOCKET fd, int onoff, int linger)
{
	struct linger slinger;

	slinger.l_onoff = onoff ? 1 : 0;
	slinger.l_linger = linger;

	return !setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)(&slinger), sizeof(slinger));
}

bool set_reuse_addr_sock(SOCKET fd, int reuse)
{
	int val = reuse > 0 ? 1 : 0;
	return !setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&val), sizeof(val));
}

bool set_reuse_port_sock(SOCKET fd, int reuse)
{
	int val = reuse > 0 ? 1 : 0;
	return !setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)(&val), sizeof(val));
}

SOCKET create_general_tcp_socket(const struct sockaddr_in& addr)
{
	/* Request socket. */
	SOCKET s = create_tcp_sock();
	if (s == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	/* set nonblock */
	if (!set_nonblock_sock(s, 1))
	{
		close(s);
		return INVALID_SOCKET;
	}
	if (!set_linger_sock(s, 1, 0))
	{
		close(s);
		return INVALID_SOCKET;
	}

	return s;
}

bool set_defer_accept_sock(SOCKET fd, int32_t defer)
{
	int val = defer != 0 ? 1 : 0;
	int len = sizeof(val);
	return 0 == setsockopt(fd, /*SOL_TCP*/IPPROTO_TCP, defer, (const char*)&val, len);
}

bool bind_sock(SOCKET fd, const struct sockaddr_in& addr)
{
	return !bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
}

bool listen_sock(SOCKET fd, int backlog)
{
	return !listen_sock(fd, backlog);
}

SOCKET listen_nonblock_reuse_socket(uint32_t backlog, uint32_t defer_accept, const struct sockaddr_in& addr)
{
	/* Request socket. */
	SOCKET s = create_general_tcp_socket(addr);
	if (s == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	if (defer_accept && !set_defer_accept_sock(s, defer_accept))
	{
		close(s);
		return INVALID_SOCKET;
	}

	if (!set_reuse_addr_sock(s, 1))
	{
		close(s);
		return INVALID_SOCKET;
	}

	if (!set_reuse_port_sock(s, 1))
	{
		close(s);
		return INVALID_SOCKET;
	}

	if (!bind_sock(s, addr))
	{
		close(s);
		return false;
	}

	if (!listen_sock(s, backlog))
	{
		close(s);
		return false;
	}

	return s;
}

SOCKET accept_sock(SOCKET fd, struct sockaddr_in* addr)
{
	socklen_t l = sizeof(struct sockaddr_in);
	return accept4(fd, (struct sockaddr*)addr, &l, SOCK_NONBLOCK | SOCK_CLOEXEC);
}

bool set_socket_sendbuflen(SOCKET fd, uint32_t buf_size)
{
	return !setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size));
}

bool set_socket_recvbuflen(SOCKET fd, uint32_t buf_size)
{
	return !setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size));
}