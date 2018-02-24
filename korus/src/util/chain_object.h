#pragma once

#include <unistd.h>
#include <assert.h>
#include <memory>
#include <vector>
#include "basic_defines.h"

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
	virtual void	chain_insert(T* tunnel_new)
	{
		if (_tunnel_next)
		{
			_tunnel_next->chain_prev_assign(tunnel_new);
		}
		tunnel_new->chain_next_assign(_tunnel_next);
		if (_tunnel_prev)
		{
			_tunnel_prev->chain_next_assign(tunnel_new);
		}
		tunnel_new->chain_prev_assign(_tunnel_prev);
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

	void chain_next_assign(T* tunnel_obj)
	{
		_tunnel_next = tunnel_obj;
	}
	void chain_prev_assign(T* tunnel_obj)
	{
		_tunnel_prev = tunnel_obj;
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

template<typename T>
class obj_refbase
{
public:
	obj_refbase() : _ref_counter(0)
	{
	}
	virtual ~obj_refbase()
	{
		assert(0 == _ref_counter);
	}

	int32_t	incr_ref()
	{
		return ++_ref_counter;
	}
	int32_t	decr_ref()	//外部处理=0情况时候的delete
	{
		return	--_ref_counter;
	}
	int32_t	get_ref()
	{
		return _ref_counter;
	}

protected:
	int32_t	_ref_counter;
};

template<typename T>
class obj_smartptr
{
public:
	obj_smartptr(T* obj = nullptr) : _obj(nullptr)
	{
		if (obj)
		{
			obj->incr_ref();
		}
		_obj = obj;
	}
	obj_smartptr(const obj_smartptr<T>& rhs) : _obj(nullptr)
	{
		assign(rhs);
	}
	obj_smartptr<T>& operator= (const obj_smartptr<T> rhs)
	{
		assign(rhs);
		return *this;
	}
	virtual ~obj_smartptr()
	{
		if (_obj)
		{
			if (!_obj->decr_ref())
			{
				delete _obj;
			}
		}
	}
	T* get()
	{
		return _obj;
	}
	bool operator!= (const obj_smartptr& l) const
	{
		return  _obj != l._obj;
	}
	explicit operator bool() const
	{	// test if shared_ptr object owns no resource
		return _obj != nullptr;
	}

protected:
	T*	_obj;

	void assign(const obj_smartptr<T>& rhs)
	{
		if (rhs && rhs._obj)
		{
			rhs._obj->incr_ref();
		}
		if (_obj)
		{
			if (!_obj->decr_ref())
			{
				delete _obj;
			}
		}
		_obj = rhs ? rhs._obj : nullptr;
	}
};

template<typename T>
class complex_ptr
{
public:
	complex_ptr(T* obj = nullptr)
	{
		_unknown = obj;
		_shared = nullptr;
	}
	complex_ptr(const std::shared_ptr<T> shared)
	{
		_unknown = nullptr;
		_shared = shared;
	}
	complex_ptr(const obj_smartptr<T> unknown)
	{
		_shared = nullptr;
		_unknown = unknown;
	}
	complex_ptr(const complex_ptr<T>& rhs)
	{
		_shared = rhs._shared;
		_unknown = rhs._unknown;
	}
	virtual ~complex_ptr()
	{
	}
	T* get()
	{
		if (_unknown)
		{
			return _unknown.get();
		}
		else if (_shared)
		{
			return _shared.get();
		}
		return nullptr;
	}
	complex_ptr<T>& operator= (T* obj)
	{
		_unknown = obj;
		_shared = nullptr;
		return *this;
	}
	complex_ptr<T>& operator= (const std::shared_ptr<T>& shared)
	{
		_unknown = nullptr;
		_shared = shared;
		return *this;
	}
	complex_ptr<T>& operator= (const obj_smartptr<T>& unknown)
	{
		_shared = nullptr;
		_unknown = unknown;
		return *this;
	}
	complex_ptr<T>& operator= (const complex_ptr<T>& rhs)
	{
		_shared = rhs._shared;
		_unknown = rhs._unknown;
		return *this;
	}
	bool operator!= (const complex_ptr& l) const
	{
		if (_unknown || l._unknown)
		{
			return _unknown != l._unknown;
		}
		else if (_shared || l._shared)
		{
			return _shared != l._shared;
		}
		return true;
	}
	explicit operator bool() const
	{	// test if shared_ptr object owns no resource
		return _unknown != nullptr || _shared != nullptr;
	}
	T*	operator->()
	{
		T* obj = get();
		return obj;
	}

protected:
	std::shared_ptr<T>		_shared;
	obj_smartptr<T>			_unknown;
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
