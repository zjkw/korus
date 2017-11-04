#pragma once

#include <memory>
#include <unistd.h>
#include <sys/sysinfo.h> 
#include "korus/src/thread/thread_object.h"
#include "udp_client_channel.h"

// 对应用层可见类：udp_client_channel, udp_client_handler_base, reactor_loop, udp_client. 都可运行在多线程环境下，所以都要求用shared_ptr包装起来，解决生命期问题
// udp_client不提供遍历channel的接口，这样减少内部复杂性，另外channel遍历需求也只是部分应用需求，上层自己搞定
// 如果有多进程实例绑定相同本地ip端口，需要注意当某进程挂掉后，原先和之通信的对端对象将和现存某进程通信，当严格要求进程与逻辑对象映射时候，需要做转发或拒绝服务

// 占坑
template<typename T>
class udp_client
{
public:
	udp_client(){}
	virtual ~udp_client(){}
};

// 一个udp_client拥有多线程，应用场景感觉不多？多线程下载同一文件？
template <>
class udp_client<uint16_t>
{
public:
	// addr格式ip:port
	// 支持reuseport时候可用thread_num，其为0表示默认核数，自动亲缘性绑定
	// 外部使用 dynamic_pointer_cast 将派生类的智能指针转换成 std::shared_ptr<udp_client_handler_base>
	// sock_write_size仅仅是写到套接口的UDP数据报的大小上限，无类似tcp含义	
	// 在使用reuseport情况下，注意服务器是否也开启了这个选项，因为5元组才能确定一条"连接"
#ifndef REUSEPORT_OPTION
	udp_client(const udp_client_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#else
	udp_client(uint16_t thread_num, const udp_client_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _factory_chain(factory_chain), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#endif
	{
		_tid = std::this_thread::get_id();
		_factory_chain.push_back(factory);
	}

	virtual ~udp_client()
	{
		//不加锁，因为避免和exit冲突 std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, thread_object*>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			delete it->second;
		}
		_thread_pool.clear();
	}

	virtual void start()
	{
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设udp_client是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			inner_init();
			assert(_factory_chain.size());

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

				thread_obj->add_init_task(std::bind(&udp_client::common_thread_init, this, thread_obj, _factory_chain));
				thread_obj->start();
			}
		}
	}

protected:
	udp_client_channel_factory_chain_t		_factory_chain;
	void inner_init()
	{
#ifdef REUSEPORT_OPTION
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
#endif
	}

private:
	std::thread::id							_tid;
#ifdef REUSEPORT_OPTION
	uint16_t								_thread_num;
#endif
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void common_thread_init(thread_object*	thread_obj, const udp_client_channel_factory_chain_t& factory_chain)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		std::shared_ptr<udp_client_channel>	channel = std::make_shared<udp_client_channel>(_self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		build_chain(reactor, std::dynamic_pointer_cast<udp_client_handler_base>(channel), factory_chain);

		thread_obj->add_exit_task(std::bind(&udp_client::common_thread_exit, this, thread_obj, reactor, channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));

		channel->start();
	}
	void common_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_channel>	channel)
	{
		channel->set_release();
		if (!channel->can_delete(true, 1))
		{
			channel->inner_final();
		}
		reactor->invalid();	//可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	}
};

// 一个线程拥有多个udp_client
template <>
class udp_client<reactor_loop>
{
public:
	// addr格式ip:port
	udp_client(std::shared_ptr<reactor_loop> reactor, const udp_client_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE,
		const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _reactor(reactor), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
		_factory_chain.push_back(factory);		
	}

	virtual ~udp_client()
	{
		_channel->set_release();
		if (!_channel->can_delete(true, 1))
		{
			_channel->inner_final();
		}
	}
	virtual void start()
	{
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设udp_client是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			assert(_factory_chain.size());

			_channel = std::make_shared<udp_client_channel>(_self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			build_chain(_reactor, std::dynamic_pointer_cast<udp_client_handler_base>(_channel), _factory_chain);
			_channel->start();
		}
	}

protected:
	udp_client_channel_factory_chain_t		_factory_chain;

private:
	std::thread::id							_tid;
	std::shared_ptr<reactor_loop>			_reactor;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	std::shared_ptr<udp_client_channel>		_channel;
};