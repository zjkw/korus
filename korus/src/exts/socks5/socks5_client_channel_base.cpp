#include <assert.h>
#include <string.h>
#include "socks5_client_channel_base.h"

socks5_client_channel_base::socks5_client_channel_base()
{

}

socks5_client_channel_base::~socks5_client_channel_base()
{
}

//override------------------
void	socks5_client_channel_base::on_init()
{
}

void	socks5_client_channel_base::on_final()
{
}

void	socks5_client_channel_base::on_connected()
{
}

void	socks5_client_channel_base::on_closed()
{

}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_client_channel_base::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//这是一个待处理的完整包
void	socks5_client_channel_base::on_recv_pkg(const void* buf, const size_t len)
{

}