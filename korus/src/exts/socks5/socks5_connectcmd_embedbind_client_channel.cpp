#include <assert.h>
#include "socks5_bindcmd_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

socks5_connectcmd_embedbind_client_channel::socks5_connectcmd_embedbind_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: socks5_connectcmd_client_channel(reactor, server_addr, socks_user, socks_psw)
{

}

socks5_connectcmd_embedbind_client_channel::~socks5_connectcmd_embedbind_client_channel()
{
}

//override------------------
void	socks5_connectcmd_embedbind_client_channel::on_chain_init()
{
	socks5_connectcmd_client_channel::on_chain_init();
}

void	socks5_connectcmd_embedbind_client_channel::on_chain_final()
{
	socks5_connectcmd_client_channel::on_chain_final();
}

long	socks5_connectcmd_embedbind_client_channel::chain_refcount()
{
	long ref = 0;
	if (_integration)
	{
		ref++;
	}

	return ref + socks5_connectcmd_client_channel::chain_refcount();
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

void	socks5_connectcmd_embedbind_client_channel::on_connected()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_connected();
}

void	socks5_connectcmd_embedbind_client_channel::on_closed()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_connectcmd_embedbind_client_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (!_integration)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _integration->on_ctrl_error(code);
}

void	socks5_connectcmd_embedbind_client_channel::on_shakehandler_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_data_bindcmd_result(code, proxy_listen_target_addr);

}

int32_t socks5_connectcmd_embedbind_client_channel::on_recv_split(const void* buf, const size_t size)
{
	if (!_integration)
	{
		assert(false);
		return 0;
	}

	return _integration->on_ctrl_recv_split(buf, size);
}

void	socks5_connectcmd_embedbind_client_channel::on_recv_pkg(const void* buf, const size_t size)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_recv_pkg(buf, size);
}