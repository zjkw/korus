#pragma once

#include <memory>
#include <unistd.h>
#include <sys/sysinfo.h> 
#include "korus/src/thread/thread_object.h"
#include "udp_server_channel.h"

// 对应用层可见类：udp_server_channel, udp_server_handler_base, reactor_loop, udp_server. 都可运行在多线程环境下，所以都要求用shared_ptr包装起来，解决生命期问题
// udp_server不提供遍历channel的接口，这样减少内部复杂性，另外channel遍历需求也只是部分应用需求，上层自己搞定
// 如果有多进程实例绑定相同本地ip端口，需要注意当某进程挂掉后，原先和之通信的对端对象将和现存某进程通信，当严格要求进程与逻辑对象映射时候，需要做转发或拒绝服务

// 占坑
template<typename T>
class udp_server
{
public:
	udp_server(){}
	virtual ~udp_server(){}
};

using udp_server_channel_factory_t = std::function<std::shared_ptr<udp_server_handler_base>()>;

// 一个udp_server拥有多线程
template <>
class udp_server<uint16_t>
{
public:
	// addr格式ip:port
	// 支持reuseport时候可用thread_num，其为0表示默认核数，自动亲缘性绑定
	// 外部使用 dynamic_pointer_cast 将派生类的智能指针转换成 std::shared_ptr<udp_server_handler_base>
	// sock_write_size仅仅是写到套接口的UDP数据报的大小上限，无类似tcp含义	
#ifndef REUSEPORT_OPTION
	udp_server(const std::string& local_addr, const udp_server_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		:	_local_addr(local_addr), _factory(factory), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#else
	udp_server(uint16_t thread_num, const std::string& local_addr, const udp_server_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _local_addr(local_addr), _factory(factory), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#endif
	{
		_tid = std::this_thread::get_id();
#ifdef REUSEPORT_OPTION
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
#endif
	}

	virtual ~udp_server()
	{
		//不加锁，因为避免和exit冲突 std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, thread_object*>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			delete it->second;
		}
		_thread_pool.clear();
	}

	void start()
	{
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设udp_server是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			assert(_factory);

			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand();

#ifndef REUSEPORT_OPTION
			// 步骤1，创建listen线程
			for (uint16_t i = 0; i < 1; i++)
#else
			assert(_thread_num);
			for (uint16_t i = 0; i < _thread_num; i++)
#endif
			{
				thread_object*	thread_obj = new thread_object(abs((i + offset) % cpu_num));
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&udp_server::common_thread_init, this, thread_obj, _factory));
				thread_obj->start();
			}
		}
	}

private:
	std::thread::id							_tid;
#ifdef REUSEPORT_OPTION
	uint16_t								_thread_num;
#endif
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	udp_server_channel_factory_t			_factory;
	std::string								_local_addr;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void common_thread_init(thread_object*	thread_obj, const udp_server_channel_factory_t& factory)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		std::shared_ptr<udp_server_handler_base> cb = factory();
		std::shared_ptr<udp_server_channel>	channel = std::make_shared<udp_server_channel>(reactor, _local_addr, cb, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		cb->inner_init(reactor, channel);
		channel->start();
		
		thread_obj->add_exit_task(std::bind(&udp_server::common_thread_exit, this, thread_obj, reactor, channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void common_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_channel>	channel)
	{
		channel->invalid();
		channel->check_detach_relation(1);
		reactor->invalid();	//可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	}
};

// 一个线程拥有多个udp_server
template <>
class udp_server<reactor_loop>
{
public:
	// addr格式ip:port
	udp_server(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, const udp_server_channel_factory_t& factory, 
				const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		std::shared_ptr<udp_server_handler_base> cb = factory();
		_channel = std::make_shared<udp_server_channel>(reactor, local_addr, cb, self_read_size, self_write_size, sock_read_size, sock_write_size);
		cb->inner_init(reactor, _channel);
		_channel->start();
	}

	virtual ~udp_server()
	{
		_channel->invalid();
		_channel->check_detach_relation(1);
	}
	
private:
	std::shared_ptr<udp_server_channel>	_channel;
};