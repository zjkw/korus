#include "socks5_bindcmd_tunnel_server_channel.h"
#include "socks5_server_data_mgr.h"
#include "socks5_server_channel.h"

socks5_bindcmd_tunnel_server_channel::socks5_bindcmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor)
: tcp_server_handler_base(reactor)
{
}

socks5_bindcmd_tunnel_server_channel::~socks5_bindcmd_tunnel_server_channel()
{
}


//override------------------
void	socks5_bindcmd_tunnel_server_channel::on_chain_init()
{
}

void	socks5_bindcmd_tunnel_server_channel::on_chain_final()
{

}

void	socks5_bindcmd_tunnel_server_channel::on_chain_zomby()
{
	// 因为没有被其他对象引用，本对象可在框架要求下退出，可以主动与消去外界引用
}

long	socks5_bindcmd_tunnel_server_channel::chain_refcount()
{
	return tcp_server_handler_base::chain_refcount();
}

void	socks5_bindcmd_tunnel_server_channel::on_accept()	//连接已经建立
{
	std::string addr;
	peer_addr(addr);
	std::shared_ptr<socks5_server_channel>	channel = gac_bindcmd_line(addr);	
	if (channel)
	{
		_origin_channel = channel;
	}
	else
	{
		close();
	}
}

void	socks5_bindcmd_tunnel_server_channel::on_closed()
{

}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	socks5_bindcmd_tunnel_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	if (CEC_CLOSE_BY_PEER == code)
	{
		return CMS_MANUAL_CONTROL;
	}

	return CMS_INNER_AUTO_CLOSE;
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t socks5_bindcmd_tunnel_server_channel::on_recv_split(const void* buf, const size_t size)
{
	return size;
}

//这是一个待处理的完整包
void	socks5_bindcmd_tunnel_server_channel::on_recv_pkg(const void* buf, const size_t size)
{

}