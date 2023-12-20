#pragma once

#include <cstdint>
#include <sys/epoll.h>

class Epoll
{
public:
	static int EpollCreate()
	{
		// epoll_create的参数只要大于0就可以！这个128是随便给的
		int epfd = epoll_create(128);
		if(epfd < 0)
		{
			std::cout << "epoll_create error: " << strerror(errno) << std::endl;
			exit(5);
		}
		return epfd;
	}
	static bool EpollAdd(int epfd, int fd, uint32_t event)
	{
		struct epoll_event ev;
		ev.data.fd = fd;
		ev.events = event;
		int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
		if(ret != 0)
		{
			std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
			return false;
		}
		return true;
	}
	static bool EpollMod(int epfd, int fd, uint32_t event)
	{
		struct epoll_event ev;
		ev.data.fd = fd;
		ev.events = event;
		int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
		if(ret != 0)
		{
			std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
			return false;
		}
		return true;
	}
	static bool EpollDel(int epfd, int fd)
	{
		int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
		if(ret != 0)
		{
			std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
			return false;
		}
		return true;
	}
	static int EpollWait(int epfd, struct epoll_event* revs, int revs_num)
	{
		int timeout = -1;
		int ret = epoll_wait(epfd, revs, revs_num, timeout);
		if(ret == -1)
		{
			std::cout << "error:" << errno << " : " << strerror(errno) << std::endl;
			exit(8);
		}
		return ret;
	}
};
