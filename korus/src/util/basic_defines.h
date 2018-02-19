#pragma once

#include <stdint.h>
#include <string>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif

#define INVALID_SOCKET			(-1)

#define DEFAULT_READ_BUFSIZE	(100 * 1024)
#define DEFAULT_WRITE_BUFSIZE	(100 * 1024)
/*
#ifndef int8_t
typedef signed char int8_t;
#endif

#ifndef int16_t
typedef short int16_t;
#endif

#ifndef int32_t
typedef long int32_t;
#endif

#ifndef int64_t
typedef long long int64_t;
#endif

#ifndef uint8_t
typedef unsigned char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

#ifndef uint32_t
typedef unsigned long uint32_t;
#endif

#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif

#ifndef nullptr
#define nullptr ((void*)0)
#endif
*/


//除非特别说明，内部不会主动close/shutdown
enum CHANNEL_ERROR_CODE
{
	CEC_NONE = 0,
	CEC_CLOSE_BY_PEER = -1,	//对端关闭
	CEC_RECVBUF_SHORT = -2,	//接收缓存区不够
	CEC_SENDBUF_FULL = -3,	//发送缓存区满了
	CEC_RECVBUF_FULL = -4,	//接收缓存区满了
	CEC_SPLIT_FAILED = -5,	//接收缓存区解析包失败
	CEC_WRITE_FAILED = -6,	//写异常
	CEC_READ_FAILED = -7,	//读异常
	CEC_INVALID_SOCKET = -8,//无效socket

	CEC_SOCKS5_METHOD_NOSUPPORTED = -9,//没有支持的方法
	CEC_SOCKS5_AUTH_FAILED = -10,//验证失败
	CEC_SOCKS5_REQUEST_FAILED = -11,//普通的SOCKS服务器请求失败
	CEC_SOCKS5_REGUAL_DISALLOW = -12,//现有的规则不允许的连接
	CEC_SOCKS5_NET_UNREACHABLE = -13,//网络不可达
	CEC_SOCKS5_HOST_UNREACHABLE = -14,//主机不可达
	CEC_SOCKS5_CONN_REFUSE = -15,//连接被拒
	CEC_SOCKS5_TTL_TIMEOUT = -16,//TTL超时
	CEC_SOCKS5_CMD_NOSUPPORTED = -17,//不支持的命令超时
	CEC_SOCKS5_ADDR_NOSUPPORTED = -18,//不支持的地址类型
	CEC_SOCKS5_UNKNOWN = -19,//未定义
};

enum CLOSE_MODE_STRATEGY
{
	CMS_MANUAL_CONTROL = 0,	//外部用户手工控制
	CMS_INNER_AUTO_CLOSE = 1,//内部自动关闭
};
