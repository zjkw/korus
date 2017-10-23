#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class udp_server_handler : public udp_server_callback
{
public:
	udp_server_handler(){}
	virtual ~udp_server_handler(){}

	//override------------------
	virtual void	on_ready(std::shared_ptr<udp_server_channel> channel)	//连接已经建立
	{
		char szTest[] = "hello client, i am server!";
//		int32_t ret = channel->send(szTest, strlen(szTest));
//		printf("\non_ready/accepted, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed(std::shared_ptr<udp_server_channel> channel)
	{
		printf("\nClosed\n");
	}

	//参考udp_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<udp_server_channel> channel)
	{
		printf("\nError code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr, std::shared_ptr<udp_server_channel> channel)
	{
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

int main(int argc, char* argv[]) 
{
	std::string	addr = "0.0.0.0:9099";
	uint16_t		thread_num = 4;

#ifdef REUSEPORT_TRADITION
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

	addr = std::string("0.0.0.0:") + argv[1];
	if (argc >= 3)
	{
		thread_num = (uint16_t)atoi(argv[2]);
	}
		
	std::shared_ptr<udp_server_handler> handler = std::make_shared<udp_server_handler>();
	std::shared_ptr<udp_server_callback> cb = std::dynamic_pointer_cast<udp_server_callback>(handler);

#ifdef REUSEPORT_TRADITION
	udp_server<uint16_t> server(addr, cb);
#else
	udp_server<uint16_t> server(thread_num, addr, cb);
#endif

	server.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}