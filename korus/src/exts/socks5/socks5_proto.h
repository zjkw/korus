#pragma once

#include <stdint.h>

//	---注意: 本程序不支持ipv6，以及不支持除"不需要认证和密码验证"外的认证方式---

#define SOCKS5_V	0x05

enum SOCKS_METHOD_TYPE
{
	SMT_NONE = 0,
	SMT_NOAUTH = 1,
	SMT_GSSAPI = 2,		// 暂不支持
	SMT_USERPSW = 3,
};

//
//	客户端	<--->	代理服务器	<--->	目标服务器
//
//	1,	method_req:	u8ver + u8nmethod * u8method										客户端能支持的认证集合，让代理服务器选一个
//						u8ver = 0x05
//						u8method:
//							0x05		不需要认证
//							0x01		GSSAPI
//							0x02		用户名 / 密码
//							0x03--0x7F	由IANA分配
//							0x80--0xFE	为私人方法所保留的
//							0xFF		没有可以接受的方法
//
//	2,	method_ack:	u8ver + u8method													代理服务器回应，或选择一个，或说都不支持
//							

//	3,	auth_req:	u8ver + u8ulen * u8name + u8plen * u8psw							客户端提交账号密码，让代理服务器认证
//						u8ver = 0x01
//
//	4,	auth_ack:	u8ver + u8status													代理服务器回应是否认证成功
//						u8status:
//							0x00		成功
//							其他		失败		
//

//	5,	tunnel_req:	u8ver + u8cmd + u8rsv + u8atyp + vdst_addr + u16dst_port			客户端请求代理服务打通代理通道
//						u8ver = 0x05
//						u8cmd:
//							0x01		CONNECT模式:
//								vdst_addr	目标服务地址
//								u16dst_port	目标服务端口
//							0x02		BIND模式:
//								vdst_addr	目标服务地址
//								u16dst_port	目标服务端口
//							0x03		UDP ASSOCIATE模式:
//								vdst_addr	填0
//								u16dst_port	要填客户端想发送/接收UDP包的本地端口
//						u8rsv = 0x00
//						u8atyp:
//							0x01		IPV4,	vdst_addr = u32ip
//							0x03		域名,	vdst_addr = u8len * u8domain
//							0x04		IPV6,	vdst_addr = u128ip
// 
//	6,	tunnel_ack:	u8ver + u8rep + u8rsv + u8atyp + vdst_addr + u16dst_port			代理服务返回打通结果，特别对于BIND模式存在一个请求两个回应
//						rep:
//							0x00		成功
//							0x01		普通的SOCKS服务器请求失败
//							0x02		现有的规则不允许的连接
//							0x03		网络不可达
//							0x04		主机不可达
//							0x05		连接被拒
//							0x06		TTL超时
//							0x07		不支持的命令
//							0x08		不支持的地址类型
//							0x09–0xFF	未定义
//
//						CONNECT模式:
//							vdst_addr	目标服务地址
//							u16dst_port	目标服务端口
//						BIND模式第一次回应:
//							vdst_addr	代理服务监听目标服务请求的监听地址
//							u16dst_port	代理服务监听目标服务请求的监听端口
//						BIND模式第二次回应:
//							vdst_addr	目标服务连接到代理服务器的连接地址
//							u16dst_port	目标服务连接到代理服务器的连接端口
//						UDP ASSOCIATE模式下，此时：
//							vdst_addr	代理服务器udp监听地址
//							u16dst_port	代理服务器udp监听端口

//	7,	udp_associate_req:	u16rsv + u8flag + u8atyp + vdst_addr + u16dst_port + vdata	客户端向代理服务器发udp数据包，让其转发/拆解到目标服务器
//						u16rsv = 0x0000
//						u8flag:	当前udp包分段号，无分段填0
//						vdst_addr:	目标服务器地址
//						u16dst_port:目标服务器端口
//
//	8,	udp_associate_ack:	u16rsv + u8flag + u8atyp + vdst_addr + u16dst_port + vdata	代理服务封装/转发来自目标服务器数据包到客户端
//						u16rsv = 0x0000
//						u8flag:	当前udp包分段号，无分段填0		
//						vdst_addr:	目标服务器地址
//						u16dst_port:目标服务器端口


