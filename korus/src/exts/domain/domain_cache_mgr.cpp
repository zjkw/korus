#include <mutex>
#include <time.h>
#include "korus/src/util/singleton.h"
#include "domain_cache_mgr.h"

const std::chrono::hours	RECORD_TIMEOUT = std::chrono::hours(2);			//2Сʱ

std::mutex	_mutex;

domain_cache_mgr::domain_cache_mgr()
{
	srand(time(NULL));
}

domain_cache_mgr::~domain_cache_mgr()
{
}

bool domain_cache_mgr::ip_by_domain(const std::string& domain, std::string& ip)
{
	std::map<std::string, domain_data, strcompr>::iterator it = _domain_list.find(domain);
	if (it == _domain_list.end())
	{
		return false;
	}

	if (!it->second.iplist.size())
	{
		return false;
	}

	if (it->second.update_time + RECORD_TIMEOUT < std::chrono::system_clock::now())
	{
		_domain_list.erase(it);
		return false;
	}

	ip = it->second.iplist[(++it->second.last_pos) % it->second.iplist.size()];

	return true;
}

void domain_cache_mgr::update_cache(const std::string& domain, const std::vector<std::string>& iplist)
{
	if (domain.empty())
	{
		return;
	}

	domain_data	data;
	data.last_pos = rand();
	data.iplist = iplist;
	data.update_time = std::chrono::system_clock::now();

	_domain_list[domain] = data;
}

/////////////////////////////////////////////
bool query_ip_by_domain(const std::string& domain, std::string& ip)
{
	std::unique_lock<std::mutex> lck(_mutex);
	return singleton<domain_cache_mgr>::instance().ip_by_domain(domain, ip);
}

void update_domain_cache(const std::string& domain, const std::vector<std::string>& iplist)
{
	std::unique_lock<std::mutex> lck(_mutex);
	singleton<domain_cache_mgr>::instance().update_cache(domain, iplist);
}
