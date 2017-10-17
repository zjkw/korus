#pragma once

#include <unistd.h>
#include <sys/sysinfo.h> 
#include "tcp_listen.h"

#define DEFAULT_LISTEN_BACKLOG	20
#define DEFAULT_DEFER_ACCEPT	3

// 对应用层可见类：tcp_server_channel, tcp_server_callback, reactor_loop, tcp_server. 都可运行在多线程环境下，所以都要求用shared_ptr包装起来，解决生命期问题
// tcp_server不提供遍历channel的接口，这样减少内部复杂性，另外channel遍历需求也只是部分应用需求，上层自己搞定

// 占坑
template<typename T>
class tcp_server
{
public:
	tcp_server(){}
	virtual ~tcp_server(){}
};

// 一个tcp_server拥有多线程
template <>
class tcp_server<uint16_t>
{
public:
	// addr格式ip:port
	// 当thread_num为0表示默认核数
	// 支持亲缘性绑定
	// 外部使用 dynamic_pointer_cast 将派生类的智能指针转换成 std::shared_ptr<tcp_server_callback>
	tcp_server(	uint16_t thread_num, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT, 
				const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
				: _thread_num(thread_num), _listen_addr(listen_addr), _cb(cb), _backlog(backlog), _defer_accept(defer_accept),
				_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}

	virtual ~tcp_server()
	{
		//不加锁，因为避免和exit冲突 std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, std::shared_ptr<thread_object>>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			it->second->invalid();
		}
		_thread_pool.clear();
	}

	void start()
	{
		//必须同个线程，避免数据管理问题：构造/析构不与start不在一个线程将会使得问题复杂化：假设tcp_server是非引用计数的，生命期结束/析构时候同时又遇到了其他线程调用start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			assert(_cb);

			assert(_thread_num);
			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand() % cpu_num;
			
			for (uint16_t i = 0; i < _thread_num; i++)
			{
				std::shared_ptr<thread_object>	thread_obj = std::make_shared<thread_object>((i + offset) % cpu_num);
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&tcp_server::thread_init, this, thread_obj, _cb));
				thread_obj->start();
			}
			_cb = nullptr;//避免tcp_server_callback和tcp_server互相引用
		}
	}

private:
	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, std::shared_ptr<thread_object>>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::shared_ptr<tcp_server_callback>	_cb;
	std::string								_listen_addr;
	uint32_t								_backlog;
	uint32_t								_defer_accept;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void thread_init(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<tcp_server_callback> cb)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>(thread_obj);
		tcp_listen*						listen = new tcp_listen(reactor);

		thread_obj->add_exit_task(std::bind(&tcp_server::thread_exit, this, thread_obj, reactor, listen));

		listen->add_listen(this, _listen_addr, cb, _backlog, _defer_accept, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen)
	{
		delete listen;		//listen始终在reactor线程，所以删除是没问题的
		reactor->invalid();	//可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	}
};

// 一个线程拥有多个tcp_server
template <>
class tcp_server<reactor_loop>
{
public:
	// addr格式ip:port
	tcp_server(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		_listen = new tcp_listen(reactor);
		_listen->add_listen(this, listen_addr, cb, backlog, defer_accept, self_read_size, self_write_size, sock_read_size, sock_write_size);
	}

	virtual ~tcp_server()
	{
		delete _listen;
		_listen = nullptr;
	}

private:
	tcp_listen*								_listen;
};