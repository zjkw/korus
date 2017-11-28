#include <assert.h>
#include "korus/src/tcp/tcp_client_channel.h"
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_channel::socks5_bindcmd_client_channel()
{

}

socks5_bindcmd_client_channel::~socks5_bindcmd_client_channel()
{
}

//override------------------
void	socks5_bindcmd_client_channel::on_chain_init()
{

}

void	socks5_bindcmd_client_channel::on_chain_final()
{

}

void	socks5_bindcmd_client_channel::on_chain_zomby()
{

}

long	socks5_bindcmd_client_channel::chain_refcount()
{
	long ref = 0;
	if (_ctrl_channel)
	{
		ref += 1 + _ctrl_channel->chain_refcount();
	}
	if (_data_channel)
	{
		ref += 1 + _data_channel->chain_refcount();
	}

	return ref + chain_sharedobj_base<socks5_bindcmd_client_channel>::chain_refcount();
}

void	socks5_bindcmd_client_channel::chain_init(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl_channel, std::shared_ptr<socks5_bindcmd_originalbind_client_channel> data_channel)
{
	_ctrl_channel = ctrl_channel;
	_data_channel = data_channel;
}

void	socks5_bindcmd_client_channel::chain_final()
{
	on_chain_final();

	// 无需前置is_release判断，相信调用者
	std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl_channel = _ctrl_channel;
	std::shared_ptr<socks5_bindcmd_originalbind_client_channel> data_channel = _data_channel;
	_ctrl_channel = nullptr;
	_data_channel = nullptr;

	if (ctrl_channel)
	{
		ctrl_channel->chain_final();
	}
	if (data_channel)
	{
		data_channel->chain_final();
	}
}

void	socks5_bindcmd_client_channel::chain_zomby()
{
	on_chain_zomby();

	if (_ctrl_channel)
	{
		_ctrl_channel->chain_zomby();
	}
	if (_data_channel)
	{
		_data_channel->chain_zomby();
	}
}

//ctrl channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_channel::ctrl_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_ctrl_channel)
	{
		assert(false);
		return 0;
	}
	return _ctrl_channel->send(buf, len);
}

void	socks5_bindcmd_client_channel::ctrl_close()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->close();
}

void	socks5_bindcmd_client_channel::ctrl_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->shutdown(howto);
}

void	socks5_bindcmd_client_channel::ctrl_connect()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_channel::ctrl_state()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _ctrl_channel->state();
}

void	socks5_bindcmd_client_channel::on_ctrl_connected()
{

}

void	socks5_bindcmd_client_channel::on_ctrl_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_channel::on_ctrl_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	return CMS_INNER_AUTO_CLOSE;

}

void	socks5_bindcmd_client_channel::on_ctrl_recv_pkg(const void* buf, const size_t len)	//这是一个待处理的完整包
{

}

//data channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_client_channel::data_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	if (!_data_channel)
	{
		assert(false);
		return 0;
	}
	return _data_channel->send(buf, len);
}

void	socks5_bindcmd_client_channel::data_close()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->close();
}

void	socks5_bindcmd_client_channel::data_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->shutdown(howto);
}

void	socks5_bindcmd_client_channel::data_connect()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_client_channel::data_state()
{
	if (!_data_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _data_channel->state();
}

void	socks5_bindcmd_client_channel::on_data_connected()
{

}

void	socks5_bindcmd_client_channel::on_data_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_channel::on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义
{
	return CMS_INNER_AUTO_CLOSE;
}

void	socks5_bindcmd_client_channel::on_data_recv_pkg(const void* buf, const size_t len)	//这是一个待处理的完整包
{

}

void	socks5_bindcmd_client_channel::on_data_proxy_listen_target_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr)		//代理服务器用于监听“目标服务器过来的连接”地址
{
//	_data_channel->server_addr(proxy_listen_target_addr);
	_data_channel->connect();
}

bool build_relation(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl, std::shared_ptr<socks5_bindcmd_originalbind_client_channel> data, std::shared_ptr<socks5_bindcmd_client_channel> integration)
{
	integration->chain_init(ctrl, data);
	ctrl->set_integration(integration);
	data->set_integration(integration);
	return true;
}