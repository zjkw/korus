#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

std::string	server_addr = "0.0.0.0:9099";

class udp_server_handler : public udp_server_handler_terminal
{
public:
	udp_server_handler(std::shared_ptr<reactor_loop> reactor) : udp_server_handler_terminal(reactor){}
	virtual ~udp_server_handler(){}

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
		int32_t ret = send(szTest, strlen(szTest), si);
		printf("\non_ready, then Send %s, ret: %d\n", szTest, ret);
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

complex_ptr<udp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<udp_server_handler> handler = std::make_shared<udp_server_handler>(reactor);
	std::shared_ptr<udp_server_handler_base> cb = std::dynamic_pointer_cast<udp_server_handler_base>(handler);
	return cb;
}

int main(int argc, char* argv[])
{

	uint16_t		thread_num = 4;

	if (argc != 4)
	{
		printf("Usage: %s <proxy_port> <server_port> <thread-num> \n", argv[0]);
		printf("  e.g: %s 9099 9100 12\n", argv[0]);
		return 0;
	}

	std::string	proxy_addr = std::string("127.0.0.1:") + argv[1];
	server_addr = std::string("127.0.0.1:") + argv[2];
	thread_num = (uint16_t)atoi(argv[3]);
	
	socks5_associatecmd_client<uint16_t> client(thread_num, proxy_addr, server_addr, channel_factory, "test", "123");
	
	client.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}