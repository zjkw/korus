#pragma once

#include "socks5_connectcmd_client_channel.h"

class socks5_connectcmd_embedbind_client_channel : public socks5_connectcmd_client_channel
{
public:
	socks5_connectcmd_embedbind_client_channel(const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_connectcmd_embedbind_client_channel();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_connected();
	virtual void	on_closed();
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ����������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);
};