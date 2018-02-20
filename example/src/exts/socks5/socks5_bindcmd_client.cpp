#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class tcp_client_handler : public socks5_bindcmd_client_handler_terminal
{
public:
	tcp_client_handler(std::shared_ptr<reactor_loop> reactor) : socks5_bindcmd_client_handler_terminal(reactor){}
	virtual ~tcp_client_handler()
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

	//ctrl channel--------------
	// 下面五个函数可能运行在多线程环境下	
	virtual void	on_ctrl_connected()
	{
		char szTest[] = "hello server, i am ctrl client!";
		int32_t ret = ctrl_send(szTest, strlen(szTest));
		printf("\nCtrl Connected, then Send %s, ret: %d\n", szTest, ret);
	}
	virtual void	on_ctrl_closed()
	{
		printf("\nctrl closed\n");
	}
	CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code)				//参考CHANNEL_ERROR_CODE定义	
	{
		printf("\nctrl error code: %d\n", (int32_t)code);
		return socks5_bindcmd_client_handler_base::on_ctrl_error(code);
	}
	virtual int32_t	on_ctrl_recv_split(const void* buf, const size_t len)	//这是一个待处理的完整包
	{
		printf("\non_ctrl_recv_split len: %u\n", len);
		return 0;
	}
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t len)		//这是一个待处理的完整包
	{
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_ctrl_recv_pkg: %s, len: %u\n", szTest, len);
	}

	//data channel--------------
	virtual void	on_data_connected()
	{
		char szTest[] = "hello server, i am ctrl client!";
		int32_t ret = ctrl_send(szTest, strlen(szTest));
		printf("\ndata Connected, then Send %s, ret: %d\n", szTest, ret);
	}
	virtual void	on_data_closed()
	{
		printf("\ndata Closed\n");
	}
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
	{
		printf("\nCtrl Error code: %d\n", (int32_t)code);
		return socks5_bindcmd_client_handler_base::on_ctrl_error(code);
	}
	virtual int32_t	on_data_recv_split(const void* buf, const size_t len)	//这是一个待处理的完整包
	{
		printf("\non_data_recv_split len: %u\n", len);
		return 0;
	}
	virtual void	on_data_recv_pkg(const void* buf, const size_t len)		//这是一个待处理的完整包
	{
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_data_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

complex_ptr<socks5_bindcmd_client_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
{
	std::shared_ptr<tcp_client_handler> handler = std::make_shared<tcp_client_handler>(reactor);
	std::shared_ptr<socks5_bindcmd_client_handler_base> cb = std::dynamic_pointer_cast<socks5_bindcmd_client_handler_base>(handler);
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
	std::string	server_addr = std::string("127.0.0.1:") + argv[2];
	thread_num = (uint16_t)atoi(argv[3]);
		
	socks5_bindcmd_client<uint16_t> client(thread_num, proxy_addr, server_addr, channel_factory);
	client.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}