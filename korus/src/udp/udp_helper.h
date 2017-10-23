#pragma once

#include "korus/src/util/socket_ops.h"

SOCKET bind_udp_nonblock_socket(const struct sockaddr_in& addr);

SOCKET create_udp_socket(const struct sockaddr_in& addr);
