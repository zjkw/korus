#include "object_state.h"

noncopyable::noncopyable()
{
}

noncopyable::~noncopyable()
{
}

//////////////////////////
double_state::double_state()
	: _invalid(true)
{
}

double_state::~double_state()
{
}

void	double_state::invalid()
{
	_invalid = false;
}

void	double_state::valid()
{
	_invalid = true;
}

bool	double_state::is_valid()
{
	return _invalid;
}


/////////////
multiform_state::multiform_state() : _obj_state(MSS_NONE)
{
}

multiform_state::~multiform_state()
{
//	assert(is_dead());
}

void	multiform_state::set_prepare()
{
	_obj_state = MSS_PREPARE;
}

void	multiform_state::set_normal()
{
	_obj_state = MSS_NORMAL;
}

void	multiform_state::set_release()
{
	_obj_state = MSS_RELEASE;
}

void	multiform_state::set_dead()
{
	_obj_state = MSS_DEAD;
}

bool	multiform_state::is_none()
{
	return MSS_NONE == _obj_state;
}

bool	multiform_state::is_prepare()
{
	return MSS_PREPARE == _obj_state;
}

bool	multiform_state::is_normal()
{
	return MSS_NORMAL == _obj_state;
}

bool	multiform_state::is_release()
{
	return MSS_RELEASE == _obj_state;
}

bool	multiform_state::is_dead()
{
	return MSS_DEAD == _obj_state;
}
