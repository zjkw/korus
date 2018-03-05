#pragma once

#include "korus/src//util/net_serialize.h"
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
	
	bool			make_method_pkg(net_serialize&	codec);
	void			on_method_pkg(net_serialize&	decodec);

	bool			make_auth_pkg(net_serialize&	codec);
	void			on_auth_pkg(net_serialize&	decodec);

	virtual bool	make_tunnel_pkg(net_serialize&	codec) = 0;
	virtual void	on_tunnel_pkg(net_serialize&	decodec) = 0;

	CHANNEL_ERROR_CODE convert_error_code(uint8_t u8rep);

private:
	std::string _socks_user;
	std::string _socks_psw;

	//不暴露接口，因为内部已做了特化处理
	virtual int32_t on_recv_split(const std::shared_ptr<buffer_thunk>& data);
	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data);

	bool		send(const net_serialize&	codec);
};
