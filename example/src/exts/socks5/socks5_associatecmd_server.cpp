#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

//associatecmd
class udp_associatecmd_server_handler : public udp_server_handler_terminal
{
public:
	udp_associatecmd_server_handler(std::shared_ptr<reactor_loop> reactor) : udp_server_handler_terminal(reactor){}
	virtual ~udp_associatecmd_server_handler(){}

	//override------------------
	virtual void	on_chain_init()
	{
	}
	virtual void	on_chain_final()
	{
	}

	virtual void	on_ready()
	{
//		char szTest[] = "hello server, i am client!";

//		struct sockaddr_in	si;
//		if (!sockaddr_from_string(server_addr, si))
		{
//			return;
		}
//		send(szTest, strlen(szTest), si);
//		printf("\non_ready, then Send %s\n", szTest);
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
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

complex_ptr<udp_server_handler_base> associatecmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<udp_associatecmd_server_handler> handler = std::make_shared<udp_associatecmd_server_handler>(reactor);
	std::shared_ptr<udp_server_handler_base> cb = std::dynamic_pointer_cast<udp_server_handler_base>(handler);
	return cb;
}

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

#ifndef REUSEPORT_OPTION
	udp_server<uint16_t> associatecmd_server(addr, associatecmd_channel_factory);
#else
	udp_server<uint16_t> associatecmd_server(thread_num, addr, associatecmd_channel_factory);
#endif
	associatecmd_server.start();

	for (;;)
	{
		sleep(1);
	}
	return 0;
}