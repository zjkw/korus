#pragma once

#include "korus/src/tcp/tcp_client_channel.h"

class socks5_server_channel;
class socks5_connectcmd_tunnel_client_channel : public tcp_client_handler_base, public multiform_state
{
public:
	socks5_connectcmd_tunnel_client_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_channel> server_channel);
	virtual ~socks5_connectcmd_tunnel_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();

	virtual void	on_connected();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ����������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);

private:
	std::shared_ptr<socks5_server_channel> _server_channel;
};
