#include <assert.h>
#include <string.h>
#include "socks5_client_channel.h"

socks5_tcp_client_channel::socks5_tcp_client_channel(const std::map<SOCKS_METHOD_TYPE, SOCKS_METHOD_DATA>& method_list)
	: _method_list(method_list), _socks5_state(SCS_NONE)
{
	_re_sender_timer.bind(std::bind(&socks5_tcp_client_channel::on_re_sender_timer, this, std::placeholders::_1));
}


socks5_tcp_client_channel::~socks5_tcp_client_channel()
{
}

//override------------------
void	socks5_tcp_client_channel::on_init()
{
	_re_sender_timer.reactor(reactor().get());
}

void	socks5_tcp_client_channel::on_final()
{
	_re_sender_timer.clear();
}

void	socks5_tcp_client_channel::on_connected()
{
	assert(SCS_NONE == _socks5_state);

	_socks5_state = SCS_METHOD_REQ;
	send_method();
}

int32_t	socks5_tcp_client_channel::send_method()
{
	assert(SCS_METHOD_REQ == _socks5_state);
	
	/*
	*From RFC1928 :
	*The client connects to the server, and sends a version
	* identifier / method selection message :
	*
	*+---- + ---------- + ---------- +
	*| VER | NMETHODS   | METHODS    |
	*+---- + ---------- + ---------- +
	*| 1   | 1          | 1 to 255   |
	*+---- + ---------- + ---------- +
	*/

	assert(_method_list.size() <= 5);

	uint8_t		send_buf_len;
	uint8_t		send_buf[257];
	Socks5Version*	req = (Socks5Version*)send_buf;
	req->ver = SOCKS5_V;
	req->nmethods = (char)_method_list.size();

	uint8_t i = 0;
	for (std::map<SOCKS_METHOD_TYPE, SOCKS_METHOD_DATA>::iterator it = _method_list.begin(); it != _method_list.end(); it++, i++)
	{
		switch (it->first)
		{
		case SMT_NOAUTH:
			req->methods[i] = '0';
			break;
		case SMT_USERPSW:
			req->methods[i] = '2';
			break;
		case SMT_NONE:
		default:
			assert(false);
			return -1;
		}
	}
	send_buf_len = 2 + (uint8_t)_method_list.size();
	int32_t ret = send(send_buf, send_buf_len);
	if (ret < 0)
	{
		close();
	}
	else if (ret == 0)
	{
		_re_sender_timer.start(std::chrono::milliseconds(1000), std::chrono::milliseconds(1000));
	}
	else
	{
		_socks5_state = SCS_METHOD_WAITACK;
	}

	return ret;
}

int32_t	socks5_tcp_client_channel::send_auth(SOCKS_METHOD_TYPE type)
{
	assert(SCS_AUTH_REQ == _socks5_state);
	assert(SMT_USERPSW == type); //除无需验证模式外，===目前仅仅支持账号密码验证，不支持gssapi或其他模式

	std::map<SOCKS_METHOD_TYPE, SOCKS_METHOD_DATA>::iterator it = _method_list.find(SMT_USERPSW);
	if (it == _method_list.end())
	{
		// 或许服务器攻击客户端
		close();
		return -1;
	}

	/*
	From RFC1929:
	* Once the SOCKS V5 server has started, and the client has selected the
	* Username/Password Authentication protocol, the Username/Password
	* subnegotiation begins.  This begins with the client producing a
	* Username/Password request:
	*
	* +----+------+----------+------+----------+
	* |VER | ULEN |  UNAME   | PLEN |  PASSWD  |
	* +----+------+----------+------+----------+
	* | 1  |  1   | 1 to 255 |  1   | 1 to 255 |
	* +----+------+----------+------+----------+
	*
	* The VER field contains the current version of the subnegotiation,
	* which is X'01'
	*/

	uint8_t		send_buf_len;
	uint8_t		send_buf[515];
	Socks5Auth*	req = (Socks5Auth*)send_buf;
	req->ver = SOCKS5_V;
	req->ulen = (char)strnlen(it->second.userpsw.user, sizeof(it->second.userpsw.user));
	strncpy(req->uname, it->second.userpsw.user, sizeof(req->uname));
	req->plen = (char)strnlen(it->second.userpsw.user, sizeof(it->second.userpsw.user));
	strncpy(req->passwd, it->second.userpsw.psw, sizeof(req->passwd));
	send_buf_len = 3 + (uint16_t)req->ulen + (uint16_t)req->plen;

	int32_t ret = send(send_buf, send_buf_len);
	if (ret < 0)
	{
		close();
	}
	else if (ret == 0)
	{
		_re_sender_timer.start(std::chrono::milliseconds(1000), std::chrono::milliseconds(1000));
	}
	else
	{
		_socks5_state = SCS_AUTH_WAITACK;
	}

	return ret;
}

void	socks5_tcp_client_channel::on_re_sender_timer(timer_helper*)
{
	switch (_socks5_state)
	{
	case SCS_METHOD_REQ:
		if (send_method() > 0)
		{
			_re_sender_timer.stop();
		}
		break;
	case SCS_AUTH_REQ:
		break;
	case SCS_TUNNEL_REQ:
		break;
	case SCS_NONE:
	default:
		assert(false);
		break;
	}
}

void	socks5_tcp_client_channel::on_closed()
{
	_socks5_state = SCS_NONE;
	_re_sender_timer.stop();

	//通知后续接收者
	tcp_client_handler_base::on_closed();
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_tcp_client_channel::on_recv_split(const void* buf, const size_t len)
{
	switch (_socks5_state)
	{
	case SCS_METHOD_WAITACK:
		return len >= 2 ? 2 : 0;
	case SCS_AUTH_WAITACK:
		break;
	case SCS_TUNNEL_WAITACK:
		break;
	case SCS_NORMAL:
		return tcp_client_handler_base::on_recv_split(buf, len);
	case SCS_NONE:
	default:
		assert(false);
		break;
	}
	return 0;
}

//这是一个待处理的完整包
void	socks5_tcp_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	switch (_socks5_state)
	{
	case SCS_METHOD_WAITACK:
		break;
	case SCS_AUTH_WAITACK:
		break;
	case SCS_TUNNEL_WAITACK:
		break;
	case SCS_NORMAL:
		tcp_client_handler_base::on_recv_pkg(buf, len);
	case SCS_NONE:
	default:
		assert(false);
		break;
	}
}