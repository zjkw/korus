#include <assert.h>
#include "socks5_connectcmd_client_channel.h"

socks5_connectcmd_client_channel::socks5_connectcmd_client_channel(const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: _server_addr(server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
{

}

socks5_connectcmd_client_channel::~socks5_connectcmd_client_channel()
{
}

//override------------------
void	socks5_connectcmd_client_channel::on_init()
{
}

void	socks5_connectcmd_client_channel::on_final()
{
}

void	socks5_connectcmd_client_channel::on_connected()
{
}

void	socks5_connectcmd_client_channel::on_closed()
{

}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_connectcmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//����һ����������������
void	socks5_connectcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}