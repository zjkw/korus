#include <fcntl.h>
#include <unistd.h>
#include <string.h>
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

bool string_from_sockaddr(std::string& address, const struct sockaddr_in& si)
{
	char ip[128] = {0};
	if ((inet_ntop(si.sin_family, &si.sin_addr, ip, sizeof(ip))) == NULL)
	{
		return false;
	}
	size_t len = strlen(ip);
	snprintf(&ip[len], sizeof(ip) - len, ":%u", ntohs(si.sin_port));

	address = ip;
	return true;
}

bool string_from_ipport(std::string& address, const std::string& ip, const uint16_t port)
{
	char addr[128] = {0};
	snprintf(addr, sizeof(addr), "%s:%u", ip.c_str(), port);

	address = addr;
	return true;
}

bool string_from_ipport(std::string& address, const uint32_t& ip, const uint16_t port)
{
	struct sockaddr_in si;
	if (!sockaddr_from_string(ip, port, si))
	{
		return false;
	}

	return string_from_sockaddr(address, si);
}

bool sockaddr_from_string(const std::string& ip, const uint16_t port, struct sockaddr_in& si)
{
	si.sin_family = AF_INET;
	si.sin_port = htons(port);
	inet_aton(ip.c_str(), &si.sin_addr);

	return true;
}

bool sockaddr_from_string(const uint32_t ip, const uint16_t port, struct sockaddr_in& si)		//ip port为host字节序
{
	si.sin_family = AF_INET;
	si.sin_addr.s_addr = htonl(ip);
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

bool sockaddr_from_string(const std::string& address, std::string& host, std::string& port)
{
	uint16_t port_digit;
	bool ret = split_hostport(address, host, port_digit);
	char port_str[16];
	snprintf(port_str, sizeof(port_str), "%d", port_digit);
	port = port_str;
	return ret;
}

//address可能包括端口
SOCK_ADDR_TYPE	addrtype_from_string(const std::string& host)	//  if (inet_aton (*str, &addr.sin_addr) != 0) {
{
	struct sockaddr_in adr_inet; 
	if (!sockaddr_from_string(host, adr_inet))
	{
		return SAT_DOMAIN;
	}
	if (AF_INET6 == adr_inet.sin_family)
	{
		return SAT_IPV6;
	}
	else if (AF_INET == adr_inet.sin_family)
	{
		return SAT_IPV4;
	}
	else
	{
		return SAT_NONE;
	}
}

bool	peeraddr_from_fd(SOCKET fd, std::string& addr)
{
	if (INVALID_SOCKET != fd)
	{
		struct sockaddr_in si;
		socklen_t addrlen = sizeof(si);

		if (!getpeername(fd, (struct sockaddr*)&si, &addrlen))
		{
			if (string_from_sockaddr(addr, si))
			{
				return true;
			}
		}
	}

	return false;
}

bool	localaddr_from_fd(SOCKET fd, std::string& addr)
{
	if (INVALID_SOCKET != fd)
	{
		struct sockaddr_in si;
		socklen_t addrlen = sizeof(si);

		if (!getsockname(fd, (struct sockaddr*)&si, &addrlen))
		{
			if (string_from_sockaddr(addr, si))
			{
				return true;
			}
		}
	}

	return false;
}