#include "korus/src/util/singleton.h"
#include "socks5_server_data_mgr.h"

std::mutex	_mutex;

socks5_server_data_mgr::socks5_server_data_mgr()
{
}

socks5_server_data_mgr::~socks5_server_data_mgr()
{
}

void	socks5_server_data_mgr::set_bindcmd_line(const std::string& addr, std::shared_ptr<socks5_server_channel> channel)
{
	_forward[addr] = channel;
	_inverte[channel] = addr;
}

std::shared_ptr<socks5_server_channel>	socks5_server_data_mgr::gac_bindcmd_line(const std::string& addr)	//获取并清除
{
	std::map<std::string, std::shared_ptr<socks5_server_channel>, str_comp>::iterator it = _forward.find(addr);
	if (it != _forward.end())
	{
		std::shared_ptr<socks5_server_channel> channel = it->second;
		_forward.erase(it);
		return channel;
	}

	return nullptr;
}

void	socks5_server_data_mgr::clr_bindcmd_line(const std::string& addr)
{
	std::map<std::string, std::shared_ptr<socks5_server_channel>, str_comp>::iterator it = _forward.find(addr);
	if (it != _forward.end())
	{
		_inverte.erase(it->second);
		_forward.erase(it);
	}
}

void	socks5_server_data_mgr::clr_bindcmd_line(std::shared_ptr<socks5_server_channel> channel)
{
	std::map<std::shared_ptr<socks5_server_channel>, std::string, channel_ptr_cmp>::iterator it =	_inverte.find(channel);
	if (it != _inverte.end())
	{
		_forward.erase(it->second);
		_inverte.erase(it);
	}
}

//interface
void	set_bindcmd_line(const std::string& addr, std::shared_ptr<socks5_server_channel> channel)
{
	std::unique_lock<std::mutex> lck(_mutex);
	singleton<socks5_server_data_mgr>::instance().set_bindcmd_line(addr, channel);
}

std::shared_ptr<socks5_server_channel>	gac_bindcmd_line(const std::string& addr)	//获取并清除
{
	std::unique_lock<std::mutex> lck(_mutex);
	return singleton<socks5_server_data_mgr>::instance().gac_bindcmd_line(addr);
}

void	clr_bindcmd_line(const std::string& addr)
{
	std::unique_lock<std::mutex> lck(_mutex);
	singleton<socks5_server_data_mgr>::instance().clr_bindcmd_line(addr);
}

void	clr_bindcmd_line(std::shared_ptr<socks5_server_channel> channel)
{
	std::unique_lock<std::mutex> lck(_mutex);
	singleton<socks5_server_data_mgr>::instance().clr_bindcmd_line(channel);
}
