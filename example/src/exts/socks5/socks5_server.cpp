#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class socks5_server_auth_imp : public socks5_server_auth
{
public:
	socks5_server_auth_imp() {}
	~socks5_server_auth_imp() {}
	virtual bool				check_userpsw(const std::string& user, const std::string& psw)
	{
		return user == "test" && psw == "123";
	}
	virtual	SOCKS_METHOD_TYPE	select_method(const std::vector<SOCKS_METHOD_TYPE>&	method_list)
	{
		for (std::vector<SOCKS_METHOD_TYPE>::const_iterator it = method_list.begin(); it != method_list.end(); it++)
		{
			if (*it == SMT_USERPSW)
			{
				return SMT_USERPSW;
			}
		}

		return SMT_NONE;
	}
};

int main(int argc, char* argv[]) 
{
	if (argc != 4)
	{
		printf("Usage: %s <tcp_listen_port> <udp_listen_ip> <thread-num>\n", argv[0]);
		printf("  e.g: %s 9099 127.0.0.1 12\n", argv[0]);
		return 0;
	}

	std::string	tcp_listen_addr = std::string("127.0.0.1:") + argv[1];
	std::string	udp_listen_ip = argv[2];
	uint16_t		thread_num = (uint16_t)atoi(argv[3]);
			
	std::shared_ptr<socks5_server_auth_imp> auth = std::make_shared<socks5_server_auth_imp>();
	socks5_server<uint16_t> server(thread_num, tcp_listen_addr, udp_listen_ip, auth);
	server.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}