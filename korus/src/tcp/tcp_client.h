#pragma once

#include <map>
#include <memory>
#include <thread>
#include "korus/src/thread/thread_object.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_client_channel.h"

// 对应用层可见类：tcp_client_handler_origin, tcp_client_handler_base, reactor_loop, tcp_client. 都可运行在多线程环境下，所以都要求用shared_ptr包装起来，解决生命期问题
// tcp_client不提供遍历channel的接口，这样减少内部复杂性，另外channel遍历需求也只是部分应用需求，上层自己搞定

// 占坑
template<typename T>
class tcp_client
{
public:
	tcp_client(){}
	virtual ~tcp_client(){}
};

// 一个tcp_client拥有多线程，应用场景感觉不多？多线程下载同一文件？
template <>
class tcp_client<uint16_t>
{
public:
	// addr格式ip:port
	// 当thread_num为0表示默认核数
	// 支持亲缘性绑定
	// 外部使用 dynamic_pointer_cas 将派生类的智能指针转换成 std::shared_ptr<tcp_client_handler_base>
	// connect_timeout 表示连接开始多久后多久没收到确定结果，0表示不要超时功能; 
	// connect_retry_wait 表示知道连接失败或超时后还要等多久才开始重连，为0表立即，为-1表示不重连
	tcp_client(uint16_t thread_num, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client()
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
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设tcp_server是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic_flag::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			inner_init();

			assert(_thread_num);
			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand() % cpu_num;

			for (uint16_t i = 0; i < _thread_num; i++)
			{
				thread_object*	thread_obj = new thread_object(abs((i + offset) % cpu_num));
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&tcp_client::thread_init, this, thread_obj));
				thread_obj->start();
			}
		}
	}

protected:
	void inner_init()
	{
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}
	tcp_client_handler_origin*			create_origin_channel(std::shared_ptr<reactor_loop> reactor)
	{
		return new tcp_client_handler_origin(reactor, _server_addr, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
	}
	complex_ptr<tcp_client_handler_base>	create_terminal_channel(std::shared_ptr<reactor_loop> reactor)
	{
		complex_ptr<tcp_client_handler_base>	channel = _factory(reactor);
		return channel;
	}
	virtual complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel	=	create_origin_channel(reactor);
		complex_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)terminal_channel.get());
		origin_channel->connect();

		return terminal_channel;
	}

	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	tcp_client_channel_factory_t			_factory;
	std::string								_server_addr;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;
	
	virtual void thread_init(thread_object*	thread_obj)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		complex_ptr<tcp_client_handler_base>	terminal_channel = build_channel_chain(reactor);
		
		thread_obj->add_exit_task(std::bind(&tcp_client::thread_exit, this, thread_obj, reactor, terminal_channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, complex_ptr<tcp_client_handler_base>	terminal_channel)
	{
		reactor->invalid();
	}
};

// 一个线程拥有多个tcp_server
template <>
class tcp_client<reactor_loop>
{
public:
	// addr格式ip:port
	tcp_client(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _reactor(reactor), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client()
	{

	}
	
	virtual void start()
	{
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设tcp_server是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic_flag::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			// 构造中执行::connect，无需外部手动调用
			init_object(_reactor);
			
		}
	}
	
protected:
	virtual void init_object(std::shared_ptr<reactor_loop> reactor)
	{
		_channels = build_channel_chain(_reactor);
	}

	tcp_client_handler_origin*			create_origin_channel(std::shared_ptr<reactor_loop> reactor)
	{
		return new tcp_client_handler_origin(reactor, _server_addr, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
	}
	complex_ptr<tcp_client_handler_base>	create_terminal_channel(std::shared_ptr<reactor_loop> reactor)
	{
		complex_ptr<tcp_client_handler_base>	channel = _factory(reactor);
		return channel;
	}
	complex_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel = create_origin_channel(reactor);
		complex_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)terminal_channel.get());
		terminal_channel->connect();

		return terminal_channel;
	}

	std::shared_ptr<reactor_loop>			_reactor;
	std::thread::id							_tid;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_server_addr;
	tcp_client_channel_factory_t			_factory;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	complex_ptr<tcp_client_handler_base>		_channels;
};