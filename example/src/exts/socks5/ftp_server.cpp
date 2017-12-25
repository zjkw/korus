#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class tcp_connectcmd_server_handler;

//bindcmd
class tcp_bindcmd_client_handler : public tcp_client_handler_base
{
public:
	tcp_bindcmd_client_handler(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_connectcmd_server_handler> ctrl_channel) : _ctrl_channel(ctrl_channel), tcp_client_handler_base(reactor)
	{
	}
	virtual ~tcp_bindcmd_client_handler()
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
	virtual void	on_chain_zomby()
	{
		// 因为没有被其他对象引用，本对象可在框架要求下退出，可以主动与消去外界引用
		
	}
	virtual long	chain_refcount()
	{
		long ref = 0;
		if (_ctrl_channel)
		{
			ref += 1;
		}
		return ref + tcp_client_handler_base::chain_refcount();
	}
	virtual void	on_connected()	//连接已经建立
	{
		char szTest[] = "hello server, i am client!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\nConnected, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed()
	{
		printf("\nClosed\n");
		if (_ctrl_channel)
		{
			_ctrl_channel->close();
		}
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

		//socks5_connectcmd_client<uint16_t> bindcmd_client(thread_num, addr, addr, bindcmd_channel_factory);
		//bindcmd_client.start();
	}

private:
	std::shared_ptr<tcp_connectcmd_server_handler> _ctrl_channel;
};

//connectcmd
class tcp_connectcmd_server_handler : public tcp_server_handler_base
{
public:
	tcp_connectcmd_server_handler(std::shared_ptr<reactor_loop> reactor) : tcp_server_handler_base(reactor){}
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
	virtual void	on_chain_zomby()
	{
		// 因为没有被其他对象引用，本对象可在框架要求下退出，可以主动与消去外界引用
	}
	virtual long	chain_refcount()
	{
		long ref = 0;
		if (_data_channel)
		{
			ref += 1;
		}
		return ref + tcp_server_handler_base::chain_refcount();
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
		if (_data_channel)
		{
			_data_channel->close();
		}
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

private:
	std::shared_ptr<tcp_bindcmd_client_handler> _data_channel;

	//不要在构造函数中调用此函数，因为shared_from_this无法创建
	bool	create_data_channel(const std::string& server_addr)
	{
		//ctrl
		if (!_data_channel)
		{
			std::shared_ptr<tcp_client_channel>		data_origin_channel = std::make_shared<tcp_client_channel>(reactor(), server_addr);
			_data_channel = std::make_shared<tcp_bindcmd_client_handler>(reactor(), std::dynamic_pointer_cast<tcp_connectcmd_server_handler>(shared_from_this()));
			build_channel_chain_helper(std::dynamic_pointer_cast<tcp_client_handler_base>(_data_channel), std::dynamic_pointer_cast<tcp_client_handler_base>(data_origin_channel));
		}
		return true;
	}
};

std::shared_ptr<tcp_server_handler_base> connectcmd_channel_factory(std::shared_ptr<reactor_loop> reactor)
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