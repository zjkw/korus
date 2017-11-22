#include <assert.h>
#include "socks5_bindcmd_integration_handler_base.h"
#include "korus/src/util/net_serialize.h"
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_channel::socks5_bindcmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: socks5_client_channel_base(reactor, socks_user, socks_psw)
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

void	socks5_bindcmd_client_channel::on_connected()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_connected();
}

void	socks5_bindcmd_client_channel::on_closed()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_channel::on_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	if (!_integration)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _integration->on_ctrl_error(code);
}

//这是一个待处理的完整包
void	socks5_bindcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_recv_pkg(buf, len);
}

long	socks5_bindcmd_client_channel::chain_refcount()
{
	long ref = 0;
	if (_integration)
	{
		ref++;
	}

	return ref + socks5_client_channel_base::chain_refcount();
}

std::shared_ptr<chain_sharedobj_interface> socks5_bindcmd_client_channel::chain_terminal()
{
	if (!_integration)
	{
		assert(false);
		return nullptr;
	}

	return _integration->chain_terminal();
}

int32_t	socks5_bindcmd_client_channel::make_tunnel_pkg(void* buf, const uint16_t size)
{
	return 0;
}

void	socks5_bindcmd_client_channel::on_tunnel_pkg(const void* buf, const uint16_t size)
{

}