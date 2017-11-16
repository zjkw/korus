#include "korus.h"

void	on_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
{
	printf("2 result: %d, domain: %s, ip: %s\n", (int)result, domain.c_str(), ip.c_str());
	
	std::string ip2;
	if (query_ip_by_domain(domain, ip2))
	{
		printf("3 result: %d, domain: %s, ip: %s\n", (int)result, domain.c_str(), ip2.c_str());
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) 
	{
		printf("usage: %s domain\n", argv[0]);
		return 1;
	}
	std::string ip;

	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();
	domain_async_resolve_helper	helper(reactor.get());
	helper.bind(std::bind(&on_result, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	DOMAIN_RESOLVE_STATE state = helper.start(argv[1], ip);
	if (DRS_SUCCESS == state)
	{
		printf("1 result: %d, domain: %s, ip: %s\n", (int)DRS_SUCCESS, argv[1], ip.c_str());		
	}
	else
	{
		reactor->run();
	}
	
	printf("fin\n");

	return 0;
}
