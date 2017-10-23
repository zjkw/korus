#pragma once

#include "korus/src/util/socket_ops.h"

bool set_linger_sock(SOCKET fd, int onoff, int linger);

bool listen_sock(SOCKET fd, int backlog);

bool set_defer_accept_sock(SOCKET fd, int32_t defer);

SOCKET listen_nonblock_reuse_socket(uint32_t backlog, uint32_t defer_accept, const struct sockaddr_in& addr);

SOCKET accept_sock(SOCKET fd, struct sockaddr_in* addr);

SOCKET create_tcp_socket(const struct sockaddr_in& addr);
