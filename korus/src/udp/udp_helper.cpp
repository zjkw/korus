#include <fcntl.h>
#include <unistd.h>
#include "udp_helper.h"

SOCKET create_tcp_origin_sock()
{
	return ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
}

SOCKET create_udp_socket()
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

	return s;
}


