#pragma once

#include <atomic>

class noncopyable
{
public:
	noncopyable();
	virtual ~noncopyable();
	
private:
	noncopyable(const noncopyable& o) = delete;
	noncopyable& operator=(const noncopyable& o) = delete;
};

class double_state : public noncopyable
{
public:
	double_state();
	virtual ~double_state();

	virtual void	valid();
	virtual void	invalid();
	virtual bool	is_valid();

protected:
	std::atomic<bool>	_invalid;
};

class multiform_state : public noncopyable
{
public:
	multiform_state();
	virtual ~multiform_state();

	virtual void	set_prepare();
	virtual void	set_normal();
	virtual void	set_release();
	virtual void	set_dead();

	virtual bool	is_none();
	virtual bool	is_prepare();
	virtual bool	is_normal();
	virtual bool	is_release();
	virtual bool	is_dead();

protected:
	enum life_state
	{
		MSS_NONE = 0,
		MSS_PREPARE = 1,	//“正在”准备资源
		MSS_NORMAL = 2,		
		MSS_RELEASE = 3,	//“正在”清理资源
		MSS_DEAD = 4,		//不可用
	};
	std::atomic<life_state>	_obj_state;
};

