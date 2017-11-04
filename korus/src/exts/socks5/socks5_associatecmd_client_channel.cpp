#include <assert.h>
#include "socks5_associatecmd_client_channel.h"

socks5_associatecmd_client_channel::socks5_associatecmd_client_channel(const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw, const udp_client_channel_factory_t& udp_factory)
{

}

socks5_associatecmd_client_channel::~socks5_associatecmd_client_channel()
{
}

//override------------------
void	socks5_associatecmd_client_channel::on_init()
{
}

void	socks5_associatecmd_client_channel::on_final()
{
}

void	socks5_associatecmd_client_channel::on_connected()
{
}

void	socks5_associatecmd_client_channel::on_closed()
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_associatecmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//这是一个待处理的完整包
void	socks5_associatecmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}