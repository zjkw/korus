#include "ares.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <sys/epoll.h>

static void callback(void *arg, int status, int timeouts, struct hostent *host)
{
	if (!host || status != ARES_SUCCESS) {
		printf("Failed to lookup %s\n", ares_strerror(status));
		return;
	}

	printf("Found address name %s\n", host->h_name);
	char ip[INET6_ADDRSTRLEN];
	int i = 0;

	for (i = 0; host->h_addr_list[i]; ++i) {
		inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
		printf("%s\n", ip);
	}
}

static void wait_ares(ares_channel channel)
{
	int 	_fd = epoll_create1(EPOLL_CLOEXEC);	//tbd 异常处理
	struct epoll_event	ev[ARES_GETSOCK_MAXNUM];

	int fds[ARES_GETSOCK_MAXNUM];
	int fd_num = ares_getsock(channel, fds, ARES_GETSOCK_MAXNUM);
	if(fd_num < 0)
	{
		printf("ares_getsock failed\n");
		return ;
	}
	for(int32_t j = 0; j < fd_num; ++j)
	{
		struct epoll_event ev_x;
		memset(&ev_x, 0, sizeof(ev_x));
		ev_x.events = EPOLLIN | EPOLLOUT;
		ev_x.data.fd = fds[j];
		int32_t ret = epoll_ctl(_fd, EPOLL_CTL_ADD, fds[j], &ev_x);
		if(ret < 0)
		{
			 printf("epoll_ctl failed\n");
		}
	}
	
	int count = 0;
	for (;;) 
	{
		int fds2[ARES_GETSOCK_MAXNUM];
	        int fd_num2 = ares_getsock(channel, fds2, ARES_GETSOCK_MAXNUM);
	        if(fd_num2 < 0)
	        {
	                printf("ares_getsock failed\n");
	                return ;
	        }
		else if(fd_num2 == 0)
		{
			printf("ares_getsock zero\n");
	                break;
		}
		else 
		{
			for (int32_t i = 0; i < fd_num; ++i)
			{
				int32_t j = 0;
				for (; j < fd_num2; ++j)	
				{
					if(fds[i] == fds2[j])
					{
						break;
					}
				}
				if(j >= fd_num2)
				{
					struct epoll_event ev_x;
			        memset(&ev_x, 0, sizeof(ev_x));
			        ev_x.events = EPOLLIN | EPOLLOUT;
			        ev_x.data.fd = fds2[j];
			        int32_t ret = epoll_ctl(_fd, EPOLL_CTL_ADD, fds2[j], &ev_x);
			        if(ret < 0)
			        {
			                printf("epoll_ctl failed\n");
	 		        }
				}
			}
			for (int32_t j = 0; j < fd_num2; ++j)
			{
				int32_t i = 0;
				for (; i < fd_num; ++i)             
				{
					if(fds[i] == fds2[j])
					{
						break;
					}
				}
				if(i >= fd_num)
				{
					struct epoll_event ev_x;
					memset(&ev_x, 0, sizeof(ev_x));
					ev_x.events = EPOLLIN | EPOLLOUT;
					ev_x.data.fd = fds[i]; 
					int32_t ret = epoll_ctl(_fd, EPOLL_CTL_DEL, fds[i], &ev_x); 
					if(ret < 0)
					{       
						printf("epoll_ctl failed\n");
					}
				}
			}
		}
		// tbd fd clean up 

		struct timeval *tvp, tv;
		tvp = ares_timeout(channel, NULL, &tv);

		int32_t ret = epoll_wait(_fd, &ev[0], ARES_GETSOCK_MAXNUM, tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : 0);
		for (int32_t i = 0; i < ret; ++i)
		{
			for (int32_t j = 0; j < fd_num; ++j)
			{
				if (fds[j] == ev[i].data.fd)
				{
					if (ev[i].events & (EPOLLERR | EPOLLHUP))
					{
						ev[i].events |= EPOLLIN | EPOLLOUT;
					}
					
					int w = ARES_SOCKET_BAD, r = ARES_SOCKET_BAD;
					if (ev[i].events & (EPOLLERR | EPOLLIN))
					{
						r = fds[j];
					}
					if (ev[i].events &  EPOLLOUT)
					{
						w = fds[j];
					}

					ares_process_fd(channel, r, w);
				}
			}
		}
	}
}

int main(int argc, char **argv)
{
	ares_channel channel;
	int status;
	struct ares_options options;
	int optmask = 0;

	if (argc < 2) {
		printf("usage: %s ip.address\n", argv[0]);
		return 1;
	}

	status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status != ARES_SUCCESS) {
		printf("ares_library_init: %s\n", ares_strerror(status));
		return 1;
	}

	if ((status = ares_init(&channel)) != ARES_SUCCESS) {     // ares 对channel 进行初始化
		printf("ares_library_init: %s\n", ares_strerror(status));
		return 1;
	}

	ares_gethostbyname(channel, argv[1], AF_INET, callback, NULL);
	//ares_gethostbyname(channel, "google.com", AF_INET6, callback, NULL);
	wait_ares(channel);
	ares_destroy(channel);
	ares_library_cleanup();
	printf("fin\n");
	return 0;
}
