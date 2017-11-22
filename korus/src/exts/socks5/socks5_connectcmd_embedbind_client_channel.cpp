#include <assert.h>
#include "socks5_bindcmd_integration_handler_base.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

socks5_connectcmd_embedbind_client_channel::socks5_connectcmd_embedbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: socks5_connectcmd_client_channel(reactor, server_addr, socks_user, socks_psw)
{

}

socks5_connectcmd_embedbind_client_channel::~socks5_connectcmd_embedbind_client_channel()
{
}

//override------------------
void	socks5_connectcmd_embedbind_client_channel::on_connected()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_connected();
}

void	socks5_connectcmd_embedbind_client_channel::on_closed()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_closed();
}

CLOSE_MODE_STRATEGY	socks5_connectcmd_embedbind_client_channel::on_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	if (!_integration)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _integration->on_data_error(code);
}

//这是一个待处理的完整包
void	socks5_connectcmd_embedbind_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_recv_pkg(buf, len);
}

std::shared_ptr<chain_sharedobj_interface> socks5_connectcmd_embedbind_client_channel::chain_terminal()
{
	if (!_integration)
	{
		assert(false);
		return nullptr;
	}

	return _integration->chain_terminal();
}