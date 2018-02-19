#pragma once

#include <vector>
#include "socks5_proto.h"
#include "korus/src/tcp/tcp_server.h"

class socks5_server_auth : public std::enable_shared_from_this<socks5_server_auth>
{
public:
	socks5_server_auth() {}
	~socks5_server_auth() {}
	virtual bool				check_userpsw(const std::string& user, const std::string& psw) = 0;
	virtual	SOCKS_METHOD_TYPE	select_method(const std::vector<SOCKS_METHOD_TYPE>&	method_list) = 0;
};

//�м̽�ɫ������״̬�л�
class socks5_server_init_channel : public tcp_server_handler_base, public obj_refbase
{
public:
	socks5_server_init_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_auth> auth);
	virtual ~socks5_server_init_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	virtual void	chain_inref();
	virtual void	chain_deref();

	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//����һ����������������
	virtual void	on_recv_pkg(const void* buf, const size_t size);
	
private:
	std::shared_ptr<socks5_server_auth>	_auth;

	enum SOCKS_SERVER_STATE
	{
		SSS_NONE = 0,
		SSS_METHOD = 1,		
		SSS_AUTH = 2,
		SSS_TUNNEL = 3,	
		SSS_NORMAL = 4,
	};
	SOCKS_SERVER_STATE	_shakehand_state;
};