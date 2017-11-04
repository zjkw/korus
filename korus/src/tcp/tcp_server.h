#pragma once

#include <unistd.h>
#include <sys/sysinfo.h> 
#include "korus/src/thread/thread_object.h"
#include "tcp_listen.h"
#include "tcp_server_channel_creator.h"

#define DEFAULT_LISTEN_BACKLOG	20
#define DEFAULT_DEFER_ACCEPT	3

// 对应用层可见类：tcp_server_channel, tcp_server_handler_base, reactor_loop, tcp_server. 都可运行在多线程环境下，所以都要求用shared_ptr包装起来，解决生命期问题
// tcp_server不提供遍历channel的接口，这样减少内部复杂性，另外channel遍历需求也只是部分应用需求，上层自己搞定
// 如果有多进程实例绑定相同本地ip端口，需要注意当某进程挂掉后，原先和之通信的对端对象将和现存进程通信，当严格要求进程与逻辑对象映射时候，需要做转发或拒绝服务

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
	// 外部使用 dynamic_pointer_cast 将派生类的智能指针转换成 std::shared_ptr<tcp_server_handler_base>
	tcp_server(uint16_t thread_num, const std::string& listen_addr, const tcp_server_channel_factory_t& factory, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
				const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
				: _thread_num(thread_num), _listen_addr(listen_addr), _backlog(backlog), _defer_accept(defer_accept),
				_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#ifndef REUSEPORT_OPTION
				, _listen_thread(nullptr), _is_listen_init(false), _alone_listen(nullptr), _num_worker_ready(0)
#endif
	{
		_tid = std::this_thread::get_id();
		_factory_chain.push_back(factory);
	}

	virtual ~tcp_server()
	{
		//不加锁，因为避免和exit冲突 std::unique_lock <std::mutex> lck(_mutex_pool);
#ifndef REUSEPORT_OPTION
		//由于只有tcp_server引用，将会自行销毁
		delete _listen_thread;		
		_listen_thread = nullptr;
#endif
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
		//atomic::test_and_set检查flag是否被设置，若被设置直接返回true，若没有设置则设置flag为true后再返回false
		if (!_start.test_and_set())
		{
			assert(_factory_chain.size());

			inner_init();

			assert(_thread_num);
			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand();

#ifndef REUSEPORT_OPTION
			// 步骤1，创建listen线程
			_listen_thread = new thread_object(abs(offset % cpu_num));
			offset++;

			_listen_thread->add_init_task(std::bind(&tcp_server::listen_thread_init, this, _listen_thread, _factory_chain, offset));
			_listen_thread->start();	
			if (1 != _thread_num)
			{
				{
					std::unique_lock <std::mutex> lck(_mutex_listen_init);
					while (!_is_listen_init)
					{
						_cond_listen_init.wait(lck);
					}
				}
				{
					std::unique_lock <std::mutex> lck(_mutex_worker_ready);
					_num_worker_ready = _thread_num;				
				}
				tcp_listen* listen = _alone_listen;
#else
				tcp_listen* listen = nullptr;
#endif
				
				// 步骤2，创建工作线程
				for (uint16_t i = 0; i < _thread_num; i++)
				{
					thread_object*	thread_obj = new thread_object(abs((i + offset) % cpu_num));
					_thread_pool[i] = thread_obj;

					thread_obj->add_init_task(std::bind(&tcp_server::common_thread_init, this, thread_obj, _factory_chain, listen));
					thread_obj->start();
				}
#ifndef REUSEPORT_OPTION
			}
#endif
		}
	}

protected:
	tcp_server_channel_factory_chain_t		_factory_chain;

	void inner_init()
	{
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}

private:
	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_listen_addr;
	uint32_t								_backlog;
	uint32_t								_defer_accept;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	// 无其他对象依赖，所以不用shared_ptr，下同
#ifndef REUSEPORT_OPTION				
	thread_object*							_listen_thread;
	tcp_listen*								_alone_listen;				//listen线程创建，主线程不做销毁

	bool									_is_listen_init;
	std::condition_variable					_cond_listen_init;			//调用者阻塞等待监听工作线程init完成
	std::mutex								_mutex_listen_init;

	uint16_t								_num_worker_ready;
	std::condition_variable					_cond_worker_ready;			//调用者阻塞等待工作线程start完成
	std::mutex								_mutex_worker_ready;
#endif

#ifndef REUSEPORT_OPTION
	void listen_thread_init(thread_object*	thread_obj, const tcp_server_channel_factory_chain_t& factory_chain, int32_t offset)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>();
		_alone_listen = new tcp_listen(reactor, _listen_addr, _backlog, _defer_accept);
		tcp_server_channel_creator*		creator = nullptr;
		if(1 == _thread_num)	//就复用同一个线程好了
		{
			creator = new tcp_server_channel_creator(reactor, factory_chain, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_alone_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}

		thread_obj->add_exit_task(std::bind(&tcp_server::listen_thread_exit, this, thread_obj, reactor, _alone_listen, creator));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));

		// 通知主进程：listen线程初始化完成
		{
			std::unique_lock <std::mutex> lck(_mutex_listen_init);
			_is_listen_init = true;
			_cond_listen_init.notify_one();
		}
		
		// 等待工作线程初始化完成
		if(1 != _thread_num)
		{
			std::unique_lock <std::mutex> lck(_mutex_worker_ready);
			while (_num_worker_ready)
			{
				_cond_worker_ready.wait(lck);
			}
		}

		_alone_listen->start();
	}
	void listen_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen, tcp_server_channel_creator* creator)
	{
		assert(_alone_listen == listen);
		delete listen;		
		_alone_listen = nullptr;
		delete creator;     // 可能为空：当 "1 !=_thread_num" 时
		reactor->invalid();	// 可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	}
#endif
	void common_thread_init(thread_object*	thread_obj, const tcp_server_channel_factory_chain_t& factory_chain, tcp_listen*	alone_listen)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>();
		tcp_server_channel_creator*		creator = new tcp_server_channel_creator(reactor, factory_chain, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		tcp_listen*						listen = nullptr;
		if (!alone_listen)
		{
			listen = new tcp_listen(reactor, _listen_addr, _backlog, _defer_accept);
			listen->start();
			listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}
		else
		{
			alone_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}

		thread_obj->add_exit_task(std::bind(&tcp_server::common_thread_exit, this, thread_obj, reactor, listen, creator));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));

#ifndef REUSEPORT_OPTION
		{
			std::unique_lock <std::mutex> lck(_mutex_worker_ready);
			_num_worker_ready--;
			_cond_worker_ready.notify_one();
		}
#endif
	}
	void common_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen, tcp_server_channel_creator*	creator)
	{
		delete listen;		
		delete creator;
		reactor->invalid();	//可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	}
};

// 一个线程拥有多个tcp_server
template <>
class tcp_server<reactor_loop>
{
public:
	// addr格式ip:port
	tcp_server(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, const tcp_server_channel_factory_t& factory, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		_factory_chain.push_back(factory);
	}

	virtual ~tcp_server()
	{
		delete _listen;		
		delete _creator;
	}

	virtual void start()
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
			assert(_factory_chain.size());

			_creator = new tcp_server_channel_creator(_reactor, _factory_chain, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_listen = new tcp_listen(_reactor, _listen_addr, _backlog, _defer_accept);
			_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, _creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
			_listen->start();
		}
	}

protected:
	tcp_server_channel_factory_chain_t		_factory_chain;
	
private:
	std::shared_ptr<reactor_loop>			_reactor;
	std::thread::id							_tid;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_listen_addr;
	uint32_t								_backlog;
	uint32_t								_defer_accept;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	tcp_listen*								_listen;
	tcp_server_channel_creator*				_creator;
};