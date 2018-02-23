#include <string.h>
#include "korus/inc/korus.h"
#include "korus/src/util/net_serialize.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

//自定义PORT协议：
//c->s:		u16len + u8ver + u32seq + u16Cmd + u8atyp + vdst_addr + u16dst_port			
//						u16len			整包长度，含自身
//						u32seq = 0x00			
//						u8ver = 0x01
//							u16Cmd = 0x0001	请求连接代理服务器   
//								u8atyp:
//									0x01		IPV4,	vdst_addr = u32ip
//									0x03		域名,	vdst_addr = u8len * u8domain
//									0x04		IPV6,	vdst_addr = u128ip
//							u16Cmd = 0x0002	打印文本，stOther2 = u8TextLen * u8TextChar
//
//s->c:		u16len + u8ver + u32seq + u16Cmd + 	stOther	
//							u16Cmd = 0x0001	请求连接代理服务器，stOther = u8Ret + stOther2
//								u8Ret			=0表示成功，其他表示失败：
//									0x00		成功，stOther2 = 空
//									非0x00		失败，stOther2 = u8TextLen * u8TextChar，其中Text为提示语
//							u16Cmd = 0x0002	无需返回

// 1，socks5_bindcmd_client_handler_terminal执行ctrl_connect，进行命令流连接
// 2，命令流on_ctrl_connected事件到，执行data_connect，进行数据流连接（BIND CMD）
// 3，on_data_prepare调用发生了，参数为proxy监听的tcp地址X，然后应用层需要执行ctrl_send，发送类似port命令，通知ftp服务主动连接X
// 4，on_data_connected调用发生了,表示数据流通道建立
// 5，on_ctrl_recv_pkg对port返回的处理

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
		//Step 1
		//char szTest[] = "hello server, i am ctrl client!";
		//int32_t ret = ctrl_send(szTest, strlen(szTest));
		//printf("\nCtrl Connected, then Send %s, ret: %d\n", szTest, ret);
//		printf("\n	Step 1: Ctrl Connected, then data_connect\n");

		char szTest2[] = "hello server, i am test from ctrl client!";
		uint8_t buf[128];
		net_serialize	codec(buf, sizeof(buf));
		codec
			<< static_cast<uint16_t>(0x00)
			<< static_cast<uint8_t>(0x01)
			<< static_cast<uint32_t>(0x00)
			<< static_cast<uint16_t>(0x02);

			codec << static_cast<uint8_t>(strlen(szTest2));
			codec.write(szTest2, strlen(szTest2));

		uint16_t	len = (uint16_t)codec.wpos();
		codec.wpos(0);
		codec << len;

		int32_t ret = ctrl_send(codec.data(), len);
		printf("\n	Step 1: Ctrl Connected, then Send to Ctrl, text:  %s, ret: %d, and do data_connect\n", szTest2, ret);

		data_connect();
	}
	virtual void	on_ctrl_closed()
	{
		printf("\nctrl closed\n");
	}
	virtual CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code)				//参考CHANNEL_ERROR_CODE定义	
	{
		printf("\nctrl error code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}
	virtual int32_t	on_ctrl_recv_split(const void* buf, const size_t len)	//这是一个待处理的完整包
	{
		uint16_t pkglen = 0;
		if (len >= 2)
		{
			pkglen = ntohs(*(uint16_t*)(buf));
			if (len >= pkglen)
			{
				return pkglen;
			}
		}
		printf("\non_ctrl_recv_split len: %u, pkglen: %u\n", len, pkglen);
		return pkglen;
	}
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t len)		//这是一个待处理的完整包
	{
		uint16_t	u16len = 0;
		uint8_t		u8ver = 0;
		uint32_t	u32seq = 0; 
		uint16_t	u16Cmd = 0;

		net_serialize	decodec(buf, len);
		decodec >> u16len >> u8ver >> u32seq >> u16Cmd;
		assert(decodec);

		printf("\non_ctrl_recv_pkg: len: %u, u16len: %u, u8ver: %u, u32seq: %u, u16Cmd: %u\n", len, u16len, u8ver, u32seq, u16Cmd);

		switch (u16Cmd)
		{
		case 0x0002:
			{
				uint8_t	u8TextLen = 0;
				decodec >> u8TextLen;
				char    sztext[257];
				decodec.read(sztext, u8TextLen);
				if (!decodec)
				{
					ctrl_close();
					return;
				}
				sztext[u8TextLen] = 0;

				printf("	cmd	print(0x0002),	text: %s\n", sztext);
			}
			break;
		default:
			{
				char szTest[1024] = { 0 };
				memcpy(szTest, buf, min(len, 1023));
				printf("\n  cmd unknown %s, len: %u\n", szTest, len);
			}
			break;
		}
	}

	//data channel--------------
	virtual void	on_data_prepare(const std::string& proxy_listen_target_addr)		//代理服务器用于监听“目标服务器过来的连接”地址
	{		
		//Step 2
		uint8_t buf[128];
		net_serialize	codec(buf, sizeof(buf));
		codec
			<< static_cast<uint16_t>(0x00)
			<< static_cast<uint8_t>(0x01)
			<< static_cast<uint32_t>(0x00)
			<< static_cast<uint16_t>(0x01);
		
		std::string host, port;
		if (!sockaddr_from_string(proxy_listen_target_addr, host, port))
		{
			assert(false);
			return;
		}
		SOCK_ADDR_TYPE	sat = addrtype_from_string(proxy_listen_target_addr);
		if (sat == SAT_DOMAIN)
		{
			codec << static_cast<uint8_t>(0x03);
			codec << static_cast<uint8_t>(host.size());
			codec.write(host.data(), host.size());
		}
		else if (sat == SAT_IPV4)
		{
			struct in_addr ia;
			inet_aton(host.c_str(), &ia);

			codec << static_cast<uint8_t>(0x01);
			codec << static_cast<uint32_t>(ntohl(ia.s_addr));
			codec << static_cast<uint16_t>(strtoul(port.c_str(), NULL, 10));
		}
		else
		{
			assert(false);
			return;
		}
		uint16_t	len = (uint16_t)codec.wpos();
		codec.wpos(0);
		codec << len;

		int32_t ret = ctrl_send(codec.data(), len);
		printf("\n	Step 2: data prepare, then Send to Ctrl  POST cmd, ret: %d\n", ret);
	}
	virtual void	on_data_connected()
	{
		//Step 3
		//to ctrl
		char szTest1[] = "hello ftp server, i am from ctrl client!";
		uint8_t buf[128];
		net_serialize	codec(buf, sizeof(buf));
		codec
			<< static_cast<uint16_t>(0x00)
			<< static_cast<uint8_t>(0x01)
			<< static_cast<uint32_t>(0x00)
			<< static_cast<uint16_t>(0x02)
			<< static_cast<uint8_t>(strlen(szTest1));
		codec.write(szTest1, strlen(szTest1));
		uint16_t	len = (uint16_t)codec.wpos();
		codec.wpos(0);
		codec << len;
		int32_t ret1 = ctrl_send(codec.data(), len);

		//to data
		char szTest2[] = "hello server, i am from data client!";
		int32_t ret2 = data_send(szTest2, strlen(szTest2));

		printf("\n	Step 3: data Connected, first Send to Ctrl %s, ret: %d; second Send to Data %s, ret: %d\n", szTest1, ret1, szTest2, ret2);
	}
	virtual void	on_data_closed()
	{
		printf("\ndata Closed\n");
	}
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code)		//参考CHANNEL_ERROR_CODE定义	
	{
		printf("\ndata error code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}
	virtual int32_t	on_data_recv_split(const void* buf, const size_t len)	//这是一个待处理的完整包
	{
		printf("\non_data_recv_split len: %u\n", len);
		return (int32_t)len;
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
		
	socks5_bindcmd_client<uint16_t> client(thread_num, proxy_addr, server_addr, channel_factory, "test", "123");
	client.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}