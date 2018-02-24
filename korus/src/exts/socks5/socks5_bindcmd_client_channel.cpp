#include <assert.h>
#include "korus/src/tcp/tcp_client_channel.h"
#include "socks5_connectcmd_client_channel.h"
#include "socks5_bindcmd_originalbind_client_channel.h"
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_handler_base::socks5_bindcmd_client_handler_base(std::shared_ptr<reactor_loop>	reactor)
: _reactor(reactor)
{

}

socks5_bindcmd_client_handler_base::~socks5_bindcmd_client_handler_base()
{
}

//override------------------
void	socks5_bindcmd_client_handler_base::on_chain_init()
{

}

void	socks5_bindcmd_client_handler_base::on_chain_final()
{

}

//ctrl channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_base::ctrl_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_tunnel_prev)
	{
		assert(false);
		return 0;
	}
	return _tunnel_prev->ctrl_send(buf, len);
}

void	socks5_bindcmd_client_handler_base::ctrl_close()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->ctrl_close();
}

void	socks5_bindcmd_client_handler_base::ctrl_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->ctrl_shutdown(howto);
}

void	socks5_bindcmd_client_handler_base::ctrl_connect()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->ctrl_connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_base::ctrl_state()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _tunnel_prev->ctrl_state();
}

void	socks5_bindcmd_client_handler_base::on_ctrl_connected()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_ctrl_connected();
}

void	socks5_bindcmd_client_handler_base::on_ctrl_closed()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_base::on_ctrl_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}
	return _tunnel_next->on_ctrl_error(code);
}

int32_t socks5_bindcmd_client_handler_base::on_ctrl_recv_split(const void* buf, const size_t size)
{
	if (!_tunnel_next)
	{
		assert(false);
		return 0;
	}
	return _tunnel_next->on_ctrl_recv_split(buf, size);
}

void	socks5_bindcmd_client_handler_base::on_ctrl_recv_pkg(const void* buf, const size_t size)	//这是一个待处理的完整包
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_ctrl_recv_pkg(buf, size);
}

//data channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_base::data_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_tunnel_prev)
	{
		assert(false);
		return 0;
	}
	return _tunnel_prev->data_send(buf, len);
}

void	socks5_bindcmd_client_handler_base::data_close()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->data_close();
}

void	socks5_bindcmd_client_handler_base::data_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->data_shutdown(howto);
}

void	socks5_bindcmd_client_handler_base::data_connect()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	_tunnel_prev->data_connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_base::data_state()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _tunnel_prev->data_state();
}

void	socks5_bindcmd_client_handler_base::on_data_connected()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_data_connected();
}

void	socks5_bindcmd_client_handler_base::on_data_closed()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_data_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_base::on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义
{
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}
	return _tunnel_next->on_data_error(code);
}

int32_t socks5_bindcmd_client_handler_base::on_data_recv_split(const void* buf, const size_t size)
{
	if (!_tunnel_next)
	{
		assert(false);
		return 0;
	}
	return _tunnel_next->on_data_recv_split(buf, size);
}

void	socks5_bindcmd_client_handler_base::on_data_recv_pkg(const void* buf, const size_t size)	//这是一个待处理的完整包
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_data_recv_pkg(buf, size);
}

void	socks5_bindcmd_client_handler_base::on_data_prepare(const std::string& proxy_listen_target_addr)		//代理服务器用于监听“目标服务器过来的连接”地址
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	_tunnel_next->on_data_prepare(proxy_listen_target_addr);
}

//////////////////////////////////

socks5_bindcmd_client_handler_origin::socks5_bindcmd_client_handler_origin(std::shared_ptr<reactor_loop>	reactor, const std::string& proxy_addr, const std::string& server_addr,
	const std::string& socks_user/* = ""*/, const std::string& socks_psw/* = ""*/,	// 如果账号为空，将忽略密码，认为是无需鉴权
	std::chrono::seconds connect_timeout/* = std::chrono::seconds(0)*/, std::chrono::seconds connect_retry_wait/* = std::chrono::seconds(-1)*/,
	const uint32_t self_read_size/* = DEFAULT_READ_BUFSIZE*/, const uint32_t self_write_size/* = DEFAULT_WRITE_BUFSIZE*/, const uint32_t sock_read_size/* = 0*/, const uint32_t sock_write_size/* = 0*/)
	: _ctrl_channel(nullptr), _data_channel(nullptr), socks5_bindcmd_client_handler_base(reactor)
{
	//ctrl
	tcp_client_handler_origin*							ctrl_origin_channel =	new tcp_client_handler_origin(reactor, proxy_addr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size);
	socks5_connectcmd_client_channel*					ctrl_socks5			=	new socks5_connectcmd_client_channel(reactor, server_addr, socks_user, socks_psw);
	_ctrl_channel															=	new socks5_bindcmd_filterconnect_client_channel(reactor);
	build_channel_chain_helper((tcp_client_handler_base*)ctrl_origin_channel, (tcp_client_handler_base*)ctrl_socks5, (tcp_client_handler_base*)_ctrl_channel);

	//data
	tcp_client_handler_origin*							data_origin_channel =	new tcp_client_handler_origin(reactor, proxy_addr, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size);
	socks5_bindcmd_originalbind_client_channel*			data_socks5			=	new socks5_bindcmd_originalbind_client_channel(reactor, server_addr, socks_user, socks_psw);
	_data_channel															=	new socks5_bindcmd_filterbind_client_channel(reactor);
	build_channel_chain_helper((tcp_client_handler_base*)data_origin_channel, (tcp_client_handler_base*)data_socks5, (tcp_client_handler_base*)_data_channel);

	_ctrl_channel->set_integration(this);
	data_socks5->set_integration(this);
	_data_channel->set_integration(this);
}

socks5_bindcmd_client_handler_origin::~socks5_bindcmd_client_handler_origin()
{
}

//override------------------
void	socks5_bindcmd_client_handler_origin::on_chain_init()
{

}

void	socks5_bindcmd_client_handler_origin::on_chain_final()
{

}

void	socks5_bindcmd_client_handler_origin::chain_final()
{
	socks5_bindcmd_client_handler_base::on_chain_final();

	if (_ctrl_channel)
	{
		delete _ctrl_channel;
		_ctrl_channel = nullptr;
	}
	if (_data_channel)
	{
		delete _data_channel;
		_data_channel = nullptr;
	}
}

//ctrl channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_origin::ctrl_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_ctrl_channel)
	{
		assert(false);
		return 0;
	}
	return _ctrl_channel->send(buf, len);
}

void	socks5_bindcmd_client_handler_origin::ctrl_close()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->close();
}

void	socks5_bindcmd_client_handler_origin::ctrl_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->shutdown(howto);
}

void	socks5_bindcmd_client_handler_origin::ctrl_connect()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_origin::ctrl_state()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _ctrl_channel->state();
}

void	socks5_bindcmd_client_handler_origin::on_ctrl_connected()
{
	socks5_bindcmd_client_handler_base::on_ctrl_connected();
}

void	socks5_bindcmd_client_handler_origin::on_ctrl_closed()
{
	socks5_bindcmd_client_handler_base::on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_origin::on_ctrl_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	socks5_bindcmd_client_handler_base::on_ctrl_error(code);
}

//data channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_origin::data_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_data_channel)
	{
		assert(false);
		return 0;
	}
	return _data_channel->send(buf, len);
}

void	socks5_bindcmd_client_handler_origin::data_close()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->close();
}

void	socks5_bindcmd_client_handler_origin::data_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->shutdown(howto);
}

void	socks5_bindcmd_client_handler_origin::data_connect()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_origin::data_state()
{
	if (!_data_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _data_channel->state();
}

void	socks5_bindcmd_client_handler_origin::on_data_connected()
{
	socks5_bindcmd_client_handler_base::on_data_connected();
}

void	socks5_bindcmd_client_handler_origin::on_data_closed()
{
	socks5_bindcmd_client_handler_base::on_data_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_origin::on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义
{
	return socks5_bindcmd_client_handler_base::on_data_error(code);
}

void	socks5_bindcmd_client_handler_origin::on_data_prepare(const std::string& proxy_listen_target_addr)		//代理服务器用于监听“目标服务器过来的连接”地址
{
	socks5_bindcmd_client_handler_base::on_data_prepare(proxy_listen_target_addr);
}

/////////////////////////
socks5_bindcmd_client_handler_terminal::socks5_bindcmd_client_handler_terminal(std::shared_ptr<reactor_loop>	reactor) : socks5_bindcmd_client_handler_base(reactor)
{

}

socks5_bindcmd_client_handler_terminal::~socks5_bindcmd_client_handler_terminal()
{
}

//override------------------
void	socks5_bindcmd_client_handler_terminal::on_chain_init()
{

}

void	socks5_bindcmd_client_handler_terminal::on_chain_final()
{

}

//ctrl channel--------------
// 下面五个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_terminal::ctrl_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	return socks5_bindcmd_client_handler_base::ctrl_send(buf, len);
}

void	socks5_bindcmd_client_handler_terminal::ctrl_close()
{
	socks5_bindcmd_client_handler_base::ctrl_close();
}

void	socks5_bindcmd_client_handler_terminal::ctrl_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	socks5_bindcmd_client_handler_base::ctrl_shutdown(howto);
}

void	socks5_bindcmd_client_handler_terminal::ctrl_connect()
{
	socks5_bindcmd_client_handler_base::ctrl_connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_terminal::ctrl_state()
{
	return socks5_bindcmd_client_handler_base::ctrl_state();
}

void	socks5_bindcmd_client_handler_terminal::on_ctrl_connected()
{

}

void	socks5_bindcmd_client_handler_terminal::on_ctrl_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_terminal::on_ctrl_error(CHANNEL_ERROR_CODE code)				//参考CHANNEL_ERROR_CODE定义	
{
	return CMS_INNER_AUTO_CLOSE;
}

//data channel--------------
// 下面五个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_handler_terminal::data_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	return socks5_bindcmd_client_handler_base::data_send(buf, len);
}

void	socks5_bindcmd_client_handler_terminal::data_close()
{
	socks5_bindcmd_client_handler_base::data_close();
}

void	socks5_bindcmd_client_handler_terminal::data_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	socks5_bindcmd_client_handler_base::data_shutdown(howto);
}

void	socks5_bindcmd_client_handler_terminal::data_connect()
{
	socks5_bindcmd_client_handler_base::data_connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_handler_terminal::data_state()
{
	return socks5_bindcmd_client_handler_base::data_state();
}

void	socks5_bindcmd_client_handler_terminal::on_data_connected()
{

}

void	socks5_bindcmd_client_handler_terminal::on_data_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_handler_terminal::on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义
{
	return CMS_INNER_AUTO_CLOSE;
}

void	socks5_bindcmd_client_handler_terminal::on_data_prepare(const std::string& proxy_listen_target_addr)		//代理服务器用于监听“目标服务器过来的连接”地址
{

}

//override------------------
void	socks5_bindcmd_client_handler_terminal::chain_inref()
{
	this->shared_from_this().inref();
}

void	socks5_bindcmd_client_handler_terminal::chain_deref()
{
	this->shared_from_this().deref();
}

void	socks5_bindcmd_client_handler_terminal::on_release()
{
	//默认删除,屏蔽之
}