#pragma once

//https://github.com/tostercx/ssocks/blob/d29ed334a461caf9e6e7f7eb025dfbcde5348ca5/src/libsocks/socks-common.h

#include <stdint.h>
#include <string>

#define SOCKS4_V 0x04
#define SOCKS5_V 0x05

enum SOCKS_METHOD_TYPE
{
	SMT_NONE = 0,
	SMT_NOAUTH = 1,
	SMT_GSSAPI = 2,	// 暂不支持
	SMT_USERPSW = 3,
};

union SOCKS_METHOD_DATA
{
	struct USERPSW
	{
		char user[256];
		char psw[256];
	}userpsw;
};

enum SOCKS_CLIENT_STATE
{
	SCS_NONE = 0,
	SCS_METHOD_REQ = 1,		//需要发送请求
	SCS_METHOD_WAITACK = 2,	//已发送METHOD请求，等待回应
	SCS_AUTH_REQ = 3,	
	SCS_AUTH_WAITACK = 4,
	SCS_TUNNEL_REQ = 5,
	SCS_TUNNEL_WAITACK = 6,
	SCS_NORMAL = 7,
};

enum SOCKS_TUNNEL_TYPE
{
	STT_NONE = 0,
	STT_CONNECT = 1,
	STT_BIND = 2,	
	STT_ASSOCATE = 3,
};

/* Socks5 version packet */
typedef struct {
	char ver;
	char nmethods;
	char methods[5];
} Socks5Version;

/* Socks5 version packet ACK */
#pragma pack(push, 2)
typedef struct {
	char ver;
	char method;
} Socks5VersionACK;
#pragma pack(pop)

/* Socks5 authentication packet */
typedef struct {
	char ver;
	char ulen;
	char uname[256];
	char plen;
	char passwd[256];
} Socks5Auth;

/* Socks5 authentication packet ACK */
#pragma pack(push, 2)
typedef struct {
	char ver;
	char status;
} Socks5AuthACK;
#pragma pack(pop)

/* Socks5 request packet */
typedef struct {
	char ver;
	char cmd;
	char rsv;
	char atyp;
	/*char dstadr;
	unsigned short dstport;*/
} Socks5Req;

/* Socks5 request packet ACK
* Need to change alignment 4 -> 2  else sizeof 12 instead of 10 */
#pragma pack(push, 2) /* Change alignment 4 -> 2 */
typedef struct {
	char ver;
	char rep;
	char rsv;
	char atyp;
//	struct in_addr bndaddr; /* uint32_t */
	uint16_t  bndport;
} Socks5ReqACK;

typedef struct {
	char ver;
	char cmd;
	uint16_t dstport;
	char dstadr[4];
	char *uid;
} Socks4Req;

typedef struct {
	char ver; /* Need to be null */
	char rep;
	char ign[2];
	char ign2[4];
} Socks4ReqAck;

#pragma pack(pop) /* End of change alignment */

