#include <assert.h>
#include <functional>
#include "tcp_helper.h"
#include "tcp_client_channel.h"

/////////////////base
tcp_client_handler_base::tcp_client_handler_base(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor)
{
}

tcp_client_handler_base::~tcp_client_handler_base()
{ 
}

void	tcp_client_handler_base::on_chain_init()
{
}

void	tcp_client_handler_base::on_chain_final()
{
}

void	tcp_client_handler_base::on_chain_zomby()
{

}

void	tcp_client_handler_base::on_connected()	
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_connected();
}

void	tcp_client_handler_base::on_closed()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_closed();
}

//参考CHANNEL_ERROR_CODE定义
CLOSE_MODE_STRATEGY	tcp_client_handler_base::on_error(CHANNEL_ERROR_CODE code)
{
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_next->on_error(code);
}

//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
int32_t tcp_client_handler_base::on_recv_split(const void* buf, const size_t len)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return CEC_SPLIT_FAILED;
	}
	
	return _tunnel_next->on_recv_split(buf, len);
}

//这是一个待处理的完整包
void	tcp_client_handler_base::on_recv_pkg(const void* buf, const size_t len)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	
	return _tunnel_next->on_recv_pkg(buf, len);
}

int32_t	tcp_client_handler_base::send(const void* buf, const size_t len)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	return _tunnel_prev->send(buf, len);
}

void	tcp_client_handler_base::close()
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->close();
}

void	tcp_client_handler_base::shutdown(int32_t howto)	
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->shutdown(howto); 
}

void	tcp_client_handler_base::connect()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->connect(); 
}

TCP_CLTCONN_STATE	tcp_client_handler_base::state()	
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CNS_CLOSED;
	}

	return _tunnel_prev->state(); 
}

std::shared_ptr<reactor_loop>	tcp_client_handler_base::reactor()
{
	return _reactor;
}

bool	tcp_client_handler_base::peer_addr(std::string& addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}

	_tunnel_prev->peer_addr(addr);
}

bool	tcp_client_handler_base::local_addr(std::string& addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}

	_tunnel_prev->local_addr(addr);
}
////////////////channel

tcp_client_channel::tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
	const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: tcp_client_handler_base(reactor), 
	tcp_channel_base(INVALID_SOCKET, self_read_size, self_write_size, sock_read_size, sock_write_size),
	_conn_fd(INVALID_SOCKET), _server_addr(server_addr), _conn_state(CNS_CLOSED), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait)
{
	_timer_connect_timeout.bind(std::bind(&tcp_client_channel::on_timer_connect_timeout, this, std::placeholders::_1));
	_timer_connect_retry_wait.bind(std::bind(&tcp_client_channel::on_timer_connect_retry_wait, this, std::placeholders::_1));

	_sockio_helper_connect.bind(nullptr, std::bind(&tcp_client_channel::on_sockio_write_connect, this, std::placeholders::_1));
	_sockio_helper.bind(std::bind(&tcp_client_channel::on_sockio_read, this, std::placeholders::_1), std::bind(&tcp_client_channel::on_sockio_write, this, std::placeholders::_1));

	set_prepare();
}

tcp_client_channel::~tcp_client_channel()
{
}

// 保证原子，考虑多线程环境下，buf最好是一个或若干完整包；可能触发错误/异常 on_error
int32_t	tcp_client_channel::send(const void* buf, const size_t len)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	if (CNS_CONNECTED != _conn_state)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return tcp_channel_base::send(buf, len);
}

void	tcp_client_channel::close()
{
	if (is_dead())
	{
		return;
	}
	if (CNS_CLOSED == _conn_state)
	{
		return;
	}
	//线程调度
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_client_channel::close, this), this);
		return;
	}

	printf("stop_sockio, %d\n", __LINE__);
	_sockio_helper_connect.stop();
	_sockio_helper_connect.set(INVALID_SOCKET);
	_sockio_helper.stop();
	_sockio_helper.set(INVALID_SOCKET);
	printf("close fd, %d, Line: %d\n", _fd, __LINE__);
	tcp_channel_base::close();
	if (INVALID_SOCKET != _conn_fd)
	{
		printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
		::close(_conn_fd);
		_conn_fd = INVALID_SOCKET;
	}
	_conn_state = CNS_CLOSED;
	set_prepare();

	on_closed();
}

// 参数参考全局函数 ::shutdown
void	tcp_client_channel::shutdown(int32_t howto)
{
	if (!is_prepare() && !is_normal())
	{
		assert(false);
		return;
	}
	if (CNS_CONNECTED != _conn_state)
	{
		return;
	}
	//线程调度
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_client_channel::shutdown, this, howto), this);
		return;
	}

	tcp_channel_base::shutdown(howto);
}

void	tcp_client_channel::server_addr(const std::string& server_addr)
{
	if (CNS_CLOSED != _conn_state)
	{
		return;
	}
	//线程调度
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_client_channel::server_addr, this, server_addr), this);
		return;
	}

	_server_addr = server_addr;
}

void	tcp_client_channel::connect()
{
	if (!is_prepare())
	{
		return;
	}
	if (!reactor())
	{
		assert(false);
		return;
	}
	if (CNS_CLOSED != _conn_state)
	{
		return;
	}
	//线程调度
	if (!reactor()->is_current_thread())
	{
		// tcp_client_channel生命期一般比reactor短，所以加上引用计数
		reactor()->start_async_task(std::bind(&tcp_client_channel::connect, this), this);
		return;
	}
	_sockio_helper_connect.reactor(reactor().get());
	_sockio_helper.reactor(reactor().get());
	_timer_connect_timeout.reactor(reactor().get());
	_timer_connect_retry_wait.reactor(reactor().get());

	_timer_connect_retry_wait.stop();

	struct sockaddr_in	si;
	if (!sockaddr_from_string(_server_addr, si))
	{
		return;
	}
	assert(INVALID_SOCKET == _conn_fd);
	_conn_fd = create_tcp_socket(si);
	if (INVALID_SOCKET == _conn_fd)
	{
		_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
		return;
	}

	//开始连接
	int32_t ret = ::connect(_conn_fd, (struct sockaddr *)&si, sizeof(struct sockaddr));
	if (!ret)
	{
		set_fd(_conn_fd);
		_sockio_helper.set(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CONNECTED;
		printf("start_sockio, %d\n", __LINE__);
		_sockio_helper.start(SIT_READWRITE);
		set_normal();
		on_connected();
	}
	else
	{
		if (errno == EINPROGRESS)
		{
			_conn_state = CNS_CONNECTING;	//start_sockio将会获取fd
			printf("start_sockio, %d\n", __LINE__);
			_sockio_helper_connect.set(_conn_fd);
			_sockio_helper_connect.start(SIT_WRITE);
			if (_connect_timeout.count())
			{
				_timer_connect_timeout.start(_connect_timeout, _connect_timeout);
			}
		}
		else
		{
			printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
			::close(_conn_fd);
			_conn_fd = INVALID_SOCKET;

			if (_connect_retry_wait != std::chrono::seconds(-1))
			{
				_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
			}
		}
	}
}

//触发时机：执行connect且等待状态下；当connect结果在超时前出来，将关掉定时器；触发动作：强行切换到CLOSED
void	tcp_client_channel::on_timer_connect_timeout(timer_helper* timer_id)
{
	printf("stop_sockio, %d\n", __LINE__);
	_sockio_helper_connect.stop();
	_timer_connect_timeout.stop();

	printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
	::close(_conn_fd);
	_conn_fd = INVALID_SOCKET;
	_sockio_helper_connect.set(INVALID_SOCKET);
	_conn_state = CNS_CLOSED;

	_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
}

//触发时机：已经明确connect失败/超时情况下启动；在connect时候关掉定时器；触发动作：自动执行connect
void	tcp_client_channel::on_timer_connect_retry_wait(timer_helper* timer_id)
{
	_timer_connect_retry_wait.stop();

	connect();
}

void tcp_client_channel::on_sockio_write_connect(sockio_helper* sockio_id)
{
	if (!is_prepare())
	{
		assert(false);
		return;
	}
	printf("on_sockio_write _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTING != _conn_state)
	{
		assert(false);
		return;
	}

	printf("stopt_sockio, %d\n", __LINE__);
	_sockio_helper_connect.stop();
	_sockio_helper_connect.set(INVALID_SOCKET);
	_timer_connect_timeout.stop();

	int32_t err = 0;
	socklen_t len = sizeof(int32_t);
	if (getsockopt(_conn_fd, SOL_SOCKET, SO_ERROR, (void *)&err, &len) < 0 || err != 0)
	{
		printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
		::close(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CLOSED;
	}
	else
	{
		set_fd(_conn_fd);
		_sockio_helper.set(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CONNECTED;
		printf("start_sockio, %d\n", __LINE__);

		_sockio_helper.start(SIT_READWRITE);
		set_normal();
		on_connected();
	}
}

void	tcp_client_channel::on_sockio_write(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}
	printf("on_sockio_write _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTED != _conn_state)
	{
		assert(false);
		return;
	}

	int32_t ret = tcp_channel_base::send_alone();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	tcp_client_channel::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}
	printf("on_sockio_read _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTED != _conn_state)
	{
		return;
	}
	int32_t ret = tcp_channel_base::do_recv();
	if (ret < 0)
	{
		if (is_normal())
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
		}
	}
}

void	tcp_client_channel::set_release()
{
	if (is_release() || is_dead())
	{
		assert(false);
		return;
	}

	multiform_state::set_release();

	_timer_connect_timeout.stop();
	_timer_connect_retry_wait.stop();
	printf("stopt_sockio, %d\n", __LINE__);
	_sockio_helper_connect.clear();
	_sockio_helper.clear();
	reactor()->stop_async_task(this);

	close();
}

int32_t	tcp_client_channel::on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg)
{
	if (!is_normal())
	{
		return CEC_INVALID_SOCKET;
	}
	if (CNS_CONNECTED != _conn_state)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	left_partial_pkg = false;
	int32_t size = 0;
	while (len > size)
	{
		int32_t ret = on_recv_split((uint8_t*)buf + size, len - size);
		if (!is_normal())
		{
			return CEC_INVALID_SOCKET;
		}
		if (ret == 0)
		{
			left_partial_pkg = true;	//剩下的不是一个完整的包
			break;
		}
		else if (ret < 0)
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
			break;
		}
		else if (ret + size > len)
		{
			CLOSE_MODE_STRATEGY cms = on_error(CEC_RECVBUF_SHORT);
			handle_close_strategy(cms);
			break;
		}

		on_recv_pkg((uint8_t*)buf + size, ret);
		size += ret;

		if (!is_normal())
		{
			return CEC_INVALID_SOCKET;
		}
	}

	return size;
}

void tcp_client_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//内部会自动检查有效性
	}
}

bool	tcp_client_channel::peer_addr(std::string& addr)
{
	if (!is_normal())
	{
		return false;
	}
	if (!reactor()->is_current_thread())
	{
		return false;
	}
	return tcp_channel_base::peer_addr(addr);
}

bool	tcp_client_channel::local_addr(std::string& addr)
{
	if (!is_normal())
	{
		return false;
	}
	if (!reactor()->is_current_thread())
	{
		return false;
	}
	return tcp_channel_base::local_addr(addr);
}