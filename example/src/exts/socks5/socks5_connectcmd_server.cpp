#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

//connectcmd
class tcp_connectcmd_server_handler : public tcp_server_handler_terminal
{
public:
	tcp_connectcmd_server_handler(std::shared_ptr<reactor_loop> reactor) : tcp_server_handler_terminal(reactor){}
	virtual ~tcp_connectcmd_server_handler()
	{
		printf("\nexit: 0x%p\n", this);
	}

	//override------------------
	virtual void	on_chain_init()
	{
	}
	virtual void	on_chain_final()
	{
	}

	virtual void	on_accept()	//连接已经建立
	{
		char szTest[] = "hello client, i am server!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\nConnected/accepted, then Send %s, ret: %d\n", szTest, ret);
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

	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len)
	{
		printf("\non_recv_split len: %u\n", len);
		return (int32_t)len;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len)
	{
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

complex_ptr<tcp_server_handler_base> connectcmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<tcp_connectcmd_server_handler> handler = std::make_shared<tcp_connectcmd_server_handler>(reactor);
	std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(handler);
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

	tcp_server<uint16_t> connectcmd_server(thread_num, addr, connectcmd_channel_factory);
	connectcmd_server.start();
	
	for (;;)
	{
		sleep(1);
	}
	return 0;
}