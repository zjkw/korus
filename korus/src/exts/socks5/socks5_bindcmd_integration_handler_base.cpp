#include <assert.h>
#include "korus/src/tcp/tcp_client_channel.h"
#include "socks5_bindcmd_integration_handler_base.h"

socks5_bindcmd_integration_handler_base::socks5_bindcmd_integration_handler_base()
{

}

socks5_bindcmd_integration_handler_base::~socks5_bindcmd_integration_handler_base()
{
}

//override------------------
void	socks5_bindcmd_integration_handler_base::on_init()
{
}

void	socks5_bindcmd_integration_handler_base::on_final()
{
}

//ctrl channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_integration_handler_base::ctrl_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::ctrl_close()
{

}

void	socks5_bindcmd_integration_handler_base::ctrl_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{

}

void	socks5_bindcmd_integration_handler_base::ctrl_connect()
{

}


TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::ctrl_state()
{
	return CNS_CLOSED;
}

void	socks5_bindcmd_integration_handler_base::on_ctrl_connected()
{

}

void	socks5_bindcmd_integration_handler_base::on_ctrl_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_integration_handler_base::on_ctrl_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
{
	return CMS_INNER_AUTO_CLOSE;

}

int32_t socks5_bindcmd_integration_handler_base::on_ctrl_recv_split(const void* buf, const size_t len)	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)	
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::on_ctrl_recv_pkg(const void* buf, const size_t len)	//这是一个待处理的完整包
{

}

//data channel--------------
// 下面四个函数可能运行在多线程环境下	
int32_t	socks5_bindcmd_integration_handler_base::data_send(const void* buf, const size_t len)			// 保证原子, 认为是整包，返回值若<0参考CHANNEL_ERROR_CODE
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::data_close()
{

}

void	socks5_bindcmd_integration_handler_base::data_shutdown(int32_t howto)							// 参数参考全局函数 ::shutdown
{

}

void	socks5_bindcmd_integration_handler_base::data_connect()
{

}

TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::data_state()
{
	return CNS_CLOSED;
}

void	socks5_bindcmd_integration_handler_base::on_data_connected()
{

}

void	socks5_bindcmd_integration_handler_base::on_data_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_integration_handler_base::on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义
{
	return CMS_INNER_AUTO_CLOSE;

}

int32_t socks5_bindcmd_integration_handler_base::on_data_recv_split(const void* buf, const size_t len)	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::on_data_recv_pkg(const void* buf, const size_t len)	//这是一个待处理的完整包
{

}
