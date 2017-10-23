#include <fcntl.h>
#include <unistd.h>
#include "tcp_helper.h"

SOCKET create_tcp_origin_sock()
{
	return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

bool set_linger_sock(SOCKET fd, int onoff, int linger)
{
	struct linger slinger;

	slinger.l_onoff = onoff ? 1 : 0;
	slinger.l_linger = linger;

	return !setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)(&slinger), sizeof(slinger));
}

SOCKET create_tcp_socket(const struct sockaddr_in& addr)
{
	/* Request socket. */
	SOCKET s = create_tcp_origin_sock();
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

bool listen_sock(SOCKET fd, int backlog)
{
	return !::listen(fd, backlog);
}

SOCKET listen_nonblock_reuse_socket(uint32_t backlog, uint32_t defer_accept, const struct sockaddr_in& addr)
{
	/* Request socket. */
	SOCKET s = create_tcp_socket(addr);
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
