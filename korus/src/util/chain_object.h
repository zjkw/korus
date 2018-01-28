#pragma once

#include <unistd.h>
#include <assert.h>
#include <memory>
#include <vector>

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

//纯接口
class chain_object_interface
{
public:
	chain_object_interface()
	{
	}
	virtual ~chain_object_interface()
	{
	}

	//override------------------
	virtual void	on_chain_init() = 0;
	virtual void	on_chain_final() = 0;
	virtual void	chain_inref() = 0;
	virtual void	chain_deref() = 0;
	virtual void	chain_final() = 0;
};

//链路实现
template<typename T>
class chain_object_linkbase : public chain_object_interface
{
public:
	chain_object_linkbase() : _tunnel_next(nullptr), _tunnel_prev(nullptr)
	{
	}	
	virtual ~chain_object_linkbase()
	{
		// 必须执行chain_final
		assert(!_tunnel_next);
		assert(!_tunnel_prev);
	}

	//override------------------
	virtual void	on_chain_init()
	{

	}
	virtual void	on_chain_final()
	{

	}
	virtual void	chain_inref()
	{
		if (_tunnel_next)
		{
			return _tunnel_next->chain_inref();
		}

		assert(false);
	}
	virtual void	chain_deref()
	{
		if (_tunnel_next)
		{
			return _tunnel_next->chain_deref();
		}

		assert(false);
	}
	virtual void	chain_init(T* tunnel_prev, T* tunnel_next)
	{
		_tunnel_prev = tunnel_prev;
		_tunnel_next = tunnel_next;
	}
	virtual void	chain_final()
	{
		on_chain_final();

		// 无需前置is_release判断，相信调用者
		T* tunnel_prev = _tunnel_prev;
		_tunnel_next = nullptr;
		_tunnel_prev = nullptr;

		if (tunnel_prev)
		{
			tunnel_prev->chain_final();
		}

		on_release();
	}

protected:
	T* _tunnel_prev;
	T* _tunnel_next;

	virtual void	on_release()
	{
		//默认删除
		delete this;
	}
};

//裸对象引用计数包装
template<typename T>
class chain_object_sharedwrapper
{
public:
	chain_object_sharedwrapper() : _tunnel_obj(nullptr)
	{
	}
	chain_object_sharedwrapper(T* tunnel_obj) : _tunnel_obj(tunnel_obj)
	{
		if (_tunnel_obj)
		{
			_tunnel_obj->chain_inref();
		}
	}
	chain_object_sharedwrapper(const chain_object_sharedwrapper& rhs)
	{
		_tunnel_obj = rhs._tunnel_obj;
		if (_tunnel_obj)
		{
			_tunnel_obj->chain_inref();
		}
	}
	chain_object_sharedwrapper& operator = (const chain_object_sharedwrapper& rhs)
	{
		if (this != &rhs) 
		{
			if (_tunnel_obj)
			{
				_tunnel_obj->chain_deref();
			}
			_tunnel_obj = rhs._tunnel_obj;
			if (_tunnel_obj)
			{
				_tunnel_obj->chain_inref();
			}
		}
		return *this;
	}
	virtual ~chain_object_sharedwrapper()
	{
		if (_tunnel_obj)
		{
			_tunnel_obj->chain_deref();
		}
	}

	T*	operator->()
	{
		return _tunnel_obj;
	}

	T* get()
	{
		return _tunnel_obj;
	}

private:
	T* _tunnel_obj;
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
