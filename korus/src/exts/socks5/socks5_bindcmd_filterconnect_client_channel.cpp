#include <assert.h>
#include "socks5_bindcmd_client_channel.h"
#include "socks5_bindcmd_filterconnect_client_channel.h"

socks5_bindcmd_filterconnect_client_channel::socks5_bindcmd_filterconnect_client_channel(std::shared_ptr<reactor_loop> reactor)
: _integration(nullptr), tcp_client_handler_base(reactor)
{

}

socks5_bindcmd_filterconnect_client_channel::~socks5_bindcmd_filterconnect_client_channel()
{
}

//override------------------
void	socks5_bindcmd_filterconnect_client_channel::on_chain_init()
{
}

void	socks5_bindcmd_filterconnect_client_channel::on_chain_final()
{
}

void	socks5_bindcmd_filterconnect_client_channel::chain_inref()
{
	_integration->chain_inref();
}

void	socks5_bindcmd_filterconnect_client_channel::chain_deref()
{
	_integration->chain_deref();
}

void	socks5_bindcmd_filterconnect_client_channel::on_connected()
{
	_integration->on_ctrl_connected();
}

void	socks5_bindcmd_filterconnect_client_channel::on_closed()
{
	_integration->on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_filterconnect_client_channel::on_error(CHANNEL_ERROR_CODE code)
{
	return _integration->on_ctrl_error(code);
}

int32_t socks5_bindcmd_filterconnect_client_channel::on_recv_split(const std::shared_ptr<buffer_thunk>& data)
{
	return _integration->on_ctrl_recv_split(data);
}

void	socks5_bindcmd_filterconnect_client_channel::on_recv_pkg(const std::shared_ptr<buffer_thunk>& data)
{
	_integration->on_ctrl_recv_pkg(data);
}