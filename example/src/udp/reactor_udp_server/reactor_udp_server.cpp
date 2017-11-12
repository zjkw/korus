#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class udp_server_handler : public udp_server_handler_base
{
public:
	udp_server_handler(std::shared_ptr<reactor_loop> reactor) : udp_server_handler_base(reactor){}
	virtual ~udp_server_handler(){}

	//override------------------
	virtual void	on_chain_init()
	{
	}
	virtual void	on_chain_final()
	{
	}
	virtual void	on_chain_zomby()
	{
		// 因为没有被其他对象引用，本对象可在框架要求下退出，可以主动与消去外界引用
	}
	virtual long	chain_refcount()
	{
		return udp_server_handler_base::chain_refcount();
	}
	virtual void	on_ready()	
	{
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

		char szTest2[] = "hello client, i am server!";
		int32_t ret = send(szTest2, strlen(szTest2), peer_addr);
		printf("\necho, then Send %s, ret: %d\n", szTest2, ret);
	}
};

std::shared_ptr<udp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<udp_server_handler> handler = std::make_shared<udp_server_handler>(reactor);
	std::shared_ptr<udp_server_handler_base> cb = std::dynamic_pointer_cast<udp_server_handler_base>(handler);
	return cb;
}

int main(int argc, char* argv[]) 
{
	std::string	addr = "0.0.0.0:9099";

	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
		return 0;
	}

	addr = std::string("0.0.0.0:") + argv[1];
	
	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();	
	udp_server<reactor_loop> server(reactor, addr, channel_factory);
	server.start();
	reactor->run();

	return 0;
}