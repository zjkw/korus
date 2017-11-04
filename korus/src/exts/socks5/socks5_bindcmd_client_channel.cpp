#include <assert.h>
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_channel::socks5_bindcmd_client_channel()
{

}

socks5_bindcmd_client_channel::~socks5_bindcmd_client_channel()
{
}

//override------------------
void	socks5_bindcmd_client_channel::on_init()
{
}

void	socks5_bindcmd_client_channel::on_final()
{
}

void	socks5_bindcmd_client_channel::on_connected()
{
}

void	socks5_bindcmd_client_channel::on_closed()
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_bindcmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//这是一个待处理的完整包
void	socks5_bindcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}