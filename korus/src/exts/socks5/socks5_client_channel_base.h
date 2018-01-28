#pragma once

#include "socks5_proto.h"
#include "korus/src/tcp/tcp_client_channel.h"
#include "korus/src/udp/udp_client_channel.h"
#include "korus/src/reactor/timer_helper.h"

class socks5_client_channel_base : public tcp_client_handler_base, public multiform_state
{
public:
	socks5_client_channel_base(std::shared_ptr<reactor_loop> reactor, const std::string& socks_user, const std::string& socks_psw);
	virtual ~socks5_client_channel_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_connected();
	virtual void	on_closed();

protected:
	enum SOCKS_CLIENT_STATE
	{
		SCS_NONE = 0,
		SCS_METHOD = 1,		//等待服务器回应METHOD
		SCS_AUTH = 2,
		SCS_TUNNEL = 3,
		SCS_NORMAL = 4,
	};
	SOCKS_CLIENT_STATE	_shakehand_state;
	
	int32_t			make_method_pkg(void* buf, const uint16_t size);
	void			on_method_pkg(const void* buf, const uint16_t size);

	int32_t			make_auth_pkg(void* buf, const uint16_t size);
	void			on_auth_pkg(const void* buf, const uint16_t size);

	virtual int32_t	make_tunnel_pkg(void* buf, const uint16_t size) = 0;
	virtual void	on_tunnel_pkg(const void* buf, const uint16_t size) = 0;

	CHANNEL_ERROR_CODE convert_error_code(uint8_t u8rep);

private:
	std::string _socks_user;
	std::string _socks_psw;

	//不暴露接口，因为内部已做了特化处理
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	virtual void	on_recv_pkg(const void* buf, const size_t size);
};
