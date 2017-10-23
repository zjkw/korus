#pragma once

#include <string>
#include "basic_defines.h"

bool set_reuse_port_sock(SOCKET fd, int reuse);

bool set_reuse_addr_sock(SOCKET fd, int reuse);

bool set_nonblock_sock(SOCKET fd, unsigned int nonblock);

bool set_socket_sendbuflen(SOCKET fd, uint32_t buf_size);

bool set_socket_recvbuflen(SOCKET fd, uint32_t buf_size);

bool bind_sock(SOCKET fd, const struct sockaddr_in& addr);

bool sockaddr_from_string(const std::string& address, struct sockaddr_in& si);


