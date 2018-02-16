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
	virtual bool				chacke_userpsw(const std::string& user, const std::string& psw)
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
	std::string	addr = "0.0.0.0:9099";
	uint16_t		thread_num = 4;

	if (argc != 3) 
	{
		printf("Usage: %s <port> <thread-num>\n", argv[0]);
		printf("  e.g: %s 9099 12\n", argv[0]);
		return 0;
	}

	addr = std::string("127.0.0.1:") + argv[1];
	thread_num = (uint16_t)atoi(argv[2]);
		
	std::shared_ptr<socks5_server_auth_imp> auth = std::make_shared<socks5_server_auth_imp>();
	socks5_server<uint16_t> server(thread_num, addr, auth);
	server.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}