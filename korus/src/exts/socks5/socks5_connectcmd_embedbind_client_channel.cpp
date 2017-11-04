#include <assert.h>
#include "socks5_connectcmd_embedbind_client_channel.h"

socks5_connectcmd_embedbind_client_channel::socks5_connectcmd_embedbind_client_channel(const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: socks5_connectcmd_client_channel(server_addr, socks_user, socks_psw)
{

}

socks5_connectcmd_embedbind_client_channel::~socks5_connectcmd_embedbind_client_channel()
{
}

//override------------------
void	socks5_connectcmd_embedbind_client_channel::on_init()
{
}

void	socks5_connectcmd_embedbind_client_channel::on_final()
{
}

void	socks5_connectcmd_embedbind_client_channel::on_connected()
{
}

void	socks5_connectcmd_embedbind_client_channel::on_closed()
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_connectcmd_embedbind_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//这是一个待处理的完整包
void	socks5_connectcmd_embedbind_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}