#include <string.h>
#include "korus/inc/korus.h"
#include "korus/src/util/net_serialize.h"
#include "korus/src//exts//domain/tcp_client_channel_domain.h"

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

class tcp_connectcmd_server_handler;
void close_helper_for_compile(std::shared_ptr<tcp_connectcmd_server_handler> channel);

//bindcmd
class tcp_bindcmd_client_handler : public tcp_client_handler_terminal
{
public:
	tcp_bindcmd_client_handler(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_connectcmd_server_handler> ctrl_channel) : _ctrl_channel(ctrl_channel), tcp_client_handler_terminal(reactor)
	{
	}
	virtual ~tcp_bindcmd_client_handler()
	{
		printf("\n	client exit: 0x%p\n", this);
	}

	//override------------------
	virtual void	on_connected()	//连接已经建立
	{
		char szTest[] = "hello client, i am server!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\n	client	Connected, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed()
	{
		printf("\nClosed\n");
		std::shared_ptr<tcp_connectcmd_server_handler> channel = _ctrl_channel.lock();
		close_helper_for_compile(channel);		
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
		return len;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len)
	{
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}

private:
	std::weak_ptr<tcp_connectcmd_server_handler> _ctrl_channel;
};

//connectcmd
class tcp_connectcmd_server_handler : public tcp_server_handler_terminal
{
public:
	tcp_connectcmd_server_handler(std::shared_ptr<reactor_loop> reactor) : tcp_server_handler_terminal(reactor){}
	virtual ~tcp_connectcmd_server_handler()
	{
		printf("\nserver exit: 0x%p\n", this);
	}

	//override------------------
	virtual void	on_accept()	//连接已经建立
	{
		char szTest2[] = "hello client, i am server!";
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
		
		int32_t ret = send(codec.data(), len);
		printf("\n	server Connected/accepted, then Send %s, ret: %d\n", szTest2, ret);
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
		uint16_t pkglen = 0;
		if (len >= 2)
		{
			pkglen = ntohs(*(uint16_t*)(buf));
			if (len >= pkglen)
			{
				return pkglen;
			}
		}
		printf("\non_recv_split len: %u, pkglen: %u\n", len, pkglen);
		return (int32_t)pkglen;
	}

	//这是一个待处理的完整包
	virtual void	on_recv_pkg(const void* buf, const size_t len)
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
		case 0x0001:
			{
				uint8_t	u8atyp = 0;
				decodec >> u8atyp;
				assert(decodec);

				switch (u8atyp)
				{
				case 0x01:
					{
						uint32_t	u32ip = 0;
						uint16_t	u16port = 0;
						decodec >> u32ip >> u16port;
						if (!decodec)
						{
							close();
							return;
						}
						std::string addr;
						if (!string_from_ipport(addr, u32ip, u16port))
						{
							close();
							return;
						}

						printf("	cmd	port(0x0001),	create_data_channel ipv4: %s\n", addr.c_str());
						create_data_channel(addr);
					}
					break;
				case 0x03:
					{
						uint8_t	u8domainlen = 0;
						decodec >> u8domainlen;
						char    szdomain[257];
						decodec.read(szdomain, u8domainlen);
						uint16_t port = 0;
						decodec >> port;
						if (!decodec)
						{
							close();
							return;
						}
						szdomain[u8domainlen] = 0;

						std::string addr;
						if (!string_from_ipport(addr, szdomain, port))
						{
							close();
							return;
						}
						printf("	cmd	port(0x0001),	create_data_channel ipv4: %s\n", addr.c_str());
						create_data_channel(addr);
					}
					break;
				case 0x04:
					{
						close();
						return;
					}
					break;
				default:
					break;
				}
			}
			break;
		case 0x0002:
			{
				uint8_t	u8TextLen = 0;
				decodec >> u8TextLen;
				char    sztext[257];
				decodec.read(sztext, u8TextLen);
				if (!decodec)
				{
					close();
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

private:
	std::shared_ptr<tcp_bindcmd_client_handler> _data_channel;

	//不要在构造函数中调用此函数，因为shared_from_this无法创建
	bool	create_data_channel(const std::string& server_addr)
	{
		//ctrl
		if (!_data_channel)
		{
			tcp_client_channel_domain*		data_origin_channel = new tcp_client_channel_domain(reactor(), server_addr);
			_data_channel = std::make_shared<tcp_bindcmd_client_handler>(reactor(), std::dynamic_pointer_cast<tcp_connectcmd_server_handler>(shared_from_this()));
			build_channel_chain_helper((tcp_client_handler_base*)data_origin_channel, (tcp_client_handler_base*)_data_channel.get());
			data_origin_channel->connect();
		}
		return true;
	}
};

void close_helper_for_compile(std::shared_ptr<tcp_connectcmd_server_handler> channel)
{
	if (channel)
	{
		channel->close();
	}
}

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