#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

std::string	server_addr = "0.0.0.0:9099";

class udp_client_handler : public udp_client_handler_base
{
public:
	udp_client_handler(){}
	virtual ~udp_client_handler(){}

	//override------------------
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
		char szTest[1024] = {0};
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

std::shared_ptr<udp_client_handler_base> channel_factory()
{
	std::shared_ptr<udp_client_handler> handler = std::make_shared<udp_client_handler>();
	std::shared_ptr<udp_client_handler_base> cb = std::dynamic_pointer_cast<udp_client_handler_base>(handler);
	return cb;
}

int main(int argc, char* argv[]) 
{

	uint16_t		thread_num = 4;

#ifndef REUSEPORT_OPTION
	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
#else
	if (argc != 3)
	{
		printf("Usage: %s <port> <thread-num>\n", argv[0]);
		printf("  e.g: %s 9099 12\n", argv[0]);
#endif
		return 0;
	}
	
	server_addr = std::string("127.0.0.1:") + argv[1];
	if (argc >= 3)
	{
		thread_num = (uint16_t)atoi(argv[2]);
	}		

#ifndef REUSEPORT_OPTION
	udp_client<uint16_t> client(channel_factory);
#else
	udp_client<uint16_t> client(thread_num, channel_factory);
#endif
	client.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}