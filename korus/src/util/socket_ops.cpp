#include <fcntl.h>
#include <unistd.h>
#include "socket_ops.h"

bool split_hostport(const std::string& address, std::string& host, uint16_t& port)
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

	port = (uint16_t)std::atoi(&a[index + 1]);
	host = address.substr(0, index);
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
	uint16_t port = 0;
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

bool set_reuse_addr_sock(SOCKET fd, int reuse)
{
	int val = reuse > 0 ? 1 : 0;
	return !setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)(&val), sizeof(val));
}

bool set_reuse_port_sock(SOCKET fd, int reuse)
{
#ifndef REUSEPORT_OPTION
	int val = reuse > 0 ? 1 : 0;
	return !setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (const char *)(&val), sizeof(val));
#else
	return true;
#endif
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
		return !fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
	else
	{
		return !fcntl(fd, F_SETFL, flags & (~O_NONBLOCK));
	}
}

bool set_socket_sendbuflen(SOCKET fd, uint32_t buf_size)
{
	return !setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size));
}

bool set_socket_recvbuflen(SOCKET fd, uint32_t buf_size)
{
	return !setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size));
}

bool bind_sock(SOCKET fd, const struct sockaddr_in& addr)
{
	return !bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
}
