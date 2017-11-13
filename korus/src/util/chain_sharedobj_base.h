#pragma once

#include <unistd.h>
#include <memory>
#include <vector>

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

// 可能处于多线程环境下
template<typename T>
class chain_sharedobj_base : public std::enable_shared_from_this<chain_sharedobj_base<T>>
{
public:
	chain_sharedobj_base()
	{
	}	
	virtual ~chain_sharedobj_base()
	{
		// 必须执行chain_final
		assert(!_tunnel_next);
		assert(!_tunnel_prev);
	}

	//override------------------
	virtual void	on_chain_init() = 0;
	virtual void	on_chain_final() = 0;
	virtual void	on_chain_zomby() = 0;
	virtual long	chain_refcount()
	{
		long ref = 0;
		if (_tunnel_next)
		{
			ref++;
		}
		if (_tunnel_prev)
		{
			ref += 1 + _tunnel_prev->chain_refcount();
		}

		return ref + 1 - this->shared_from_this().use_count();
	}
	virtual void	chain_init(std::shared_ptr<T> tunnel_prev, std::shared_ptr<T> tunnel_next)
	{
		_tunnel_prev = tunnel_prev;
		_tunnel_next = tunnel_next;
	}
	virtual void	chain_final()
	{
		on_chain_final();

		// 无需前置is_release判断，相信调用者
		std::shared_ptr<T> tunnel_prev = _tunnel_prev;
		_tunnel_next = nullptr;
		_tunnel_prev = nullptr;

		if (tunnel_prev)
		{
			tunnel_prev->chain_final();
		}
	}
	virtual void	chain_zomby()
	{
		on_chain_zomby();

		if (_tunnel_prev)
		{
			_tunnel_prev->chain_zomby();
		}
	}
	virtual std::shared_ptr<chain_sharedobj_base<T>> chain_terminal()
	{
		if (_tunnel_next)
		{
			return _tunnel_next->chain_terminal();
		}

		return this->shared_from_this();
	}

protected:
	std::shared_ptr<T> _tunnel_prev;
	std::shared_ptr<T> _tunnel_next;
};


///////////////
template<typename T>
bool build_channel_chain_helper_inner(std::vector<T>& objs)
{
	for (size_t i = 0; i < objs.size(); i++)
	{
		T prev = i > 0 ? objs[i - 1] : nullptr;
		T next = i + 1 < objs.size() ? objs[i + 1] : nullptr;
		objs[i]->chain_init(prev, next);
	}

	return true;
}

template<typename T>
bool build_channel_chain_helper(T first, T second)
{
	std::vector<T> objs;
	objs.push_back(first);
	objs.push_back(second);
	return build_channel_chain_helper_inner(objs);
}

template<typename T>
bool build_channel_chain_helper(T first, T second, T third)
{
	std::vector<T> objs;
	objs.push_back(first);
	objs.push_back(second);
	objs.push_back(third);
	return build_channel_chain_helper_inner(objs);
}

template<typename T>
void try_chain_final(T t, long out_refcount)
{
	if (!t)
	{
		return;
	}

	auto terminal = t->chain_terminal();
	terminal->chain_zomby();						//可能上层还保持间接或直接引用，这里使其失效：“只管功能失效化，不管生命期释放”
	if (terminal->chain_refcount() == out_refcount + 1 + 1)	//terminal + auto
	{
		terminal->chain_final();
	}
}