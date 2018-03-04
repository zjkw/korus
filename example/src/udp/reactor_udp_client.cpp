#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

std::string	server_addr = "0.0.0.0:9099";

class udp_client_handler : public udp_client_handler_terminal
{
public:
	udp_client_handler(std::shared_ptr<reactor_loop> reactor) : udp_client_handler_terminal(reactor){}
	virtual ~udp_client_handler(){}

	//override------------------
	virtual void	on_chain_init()
	{
	}
	virtual void	on_chain_final()
	{
	}

	virtual void	on_ready()	
	{
		char szTest[] = "hello server, i am client!";

		struct sockaddr_in	si;
		if (!sockaddr_from_string(server_addr, si))
		{
			return;
		}
		send(szTest, strlen(szTest), si);
		printf("\non_ready, then Send %s\n", szTest);
	}

	virtual void	on_closed()
	{
		printf("\nClosed\n");
	}

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)
	{
		printf("\nError code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
	{
		char szTest[1024] = {0};
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

complex_ptr<udp_client_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<udp_client_handler> handler = std::make_shared<udp_client_handler>(reactor);
	std::shared_ptr<udp_client_handler_base> cb = std::dynamic_pointer_cast<udp_client_handler_base>(handler);
	return cb;
}

int main(int argc, char* argv[]) 
{
	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
		return 0;
	}

	server_addr = std::string("127.0.0.1:") + argv[1];
			
	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();
	udp_client<reactor_loop> client(reactor, channel_factory);
	client.start();
	reactor->run();

	return 0;
}