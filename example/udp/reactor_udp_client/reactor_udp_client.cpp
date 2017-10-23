#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class udp_client_handler : public udp_client_callback
{
public:
	udp_client_handler(){}
	virtual ~udp_client_handler(){}

	//override------------------
	virtual void	on_ready(std::shared_ptr<udp_client_channel> channel)	//连接已经建立
	{
		char szTest[] = "hello server, i am client!";
//		int32_t ret = channel->send(szTest, strlen(szTest));
//		printf("\non_ready, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed(std::shared_ptr<udp_client_channel> channel)
	{
		printf("\nClosed\n");
	}

	//参考udp_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<udp_client_channel> channel)
	{
		printf("\nError code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr, std::shared_ptr<udp_client_channel> channel)
	{
		char szTest[1024] = {0};
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

int main(int argc, char* argv[]) 
{
	std::string	addr = "0.0.0.0:9099";

	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
		return 0;
	}

	addr = std::string("127.0.0.1:") + argv[1];
			
	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();

	std::shared_ptr<udp_client_handler> handler = std::make_shared<udp_client_handler>();
	std::shared_ptr<udp_client_callback> cb = std::dynamic_pointer_cast<udp_client_callback>(handler);

	udp_client<reactor_loop> client(reactor, addr, cb);

	reactor->run();

	return 0;
}