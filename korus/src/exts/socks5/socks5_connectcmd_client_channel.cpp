#include <assert.h>
#include "socks5_connectcmd_client_channel.h"

socks5_connectcmd_client_channel::socks5_connectcmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: _server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw),
	socks5_client_channel_base(reactor)
{

}

socks5_connectcmd_client_channel::~socks5_connectcmd_client_channel()
{
}

//override------------------
void	socks5_connectcmd_client_channel::on_chain_init()
{
}

void	socks5_connectcmd_client_channel::on_chain_final()
{
}

void	socks5_connectcmd_client_channel::on_connected()
{
}

void	socks5_connectcmd_client_channel::on_closed()
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_connectcmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//这是一个待处理的完整包
void	socks5_connectcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}