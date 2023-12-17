#pragma once

#include <cstdint>
#include <ctime>
#include <future>
#include <iostream>
#include <functional>
#include <string>
#include <cerrno>
#include <cstdlib>
#include <type_traits>
#include <unistd.h>

#include <sys/epoll.h>

#include "sock.hpp"

class EpollServer
{
	static const int revent_num = 128;
	// 仿函数             返回值  参数
	using func_t = std::function<int(int)>;
public:
	EpollServer(uint16_t port, func_t func)
		:listenSock_(-1)
		,epfd_(-1)
		,port_(port)
		,func_(func)
	{}
	~EpollServer()
	{
		if(listenSock_ != -1) close(listenSock_);
		if(epfd_ != -1) close(epfd_);
	}
	void initServer()
	{
		// 创建套接字-》绑定-》监听
		listenSock_ = Sock::Socket();
		Sock::Bind(listenSock_, port_);
		Sock::Listen(listenSock_);

		// epoll
		// epoll_create的参数只要大于0就可以！这个128是随便给的
		epfd_ = epoll_create(128);
		if(epfd_ < 0)
		{
			std::cout << "epoll_create error: " << strerror(errno) << std::endl;
			exit(5);
		}
	}
	void runServer()
	{
		// 1.把listenSock_添加到epoll模型
		struct epoll_event event;
		event.data.fd = listenSock_;
		event.events = EPOLLIN;
		int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, listenSock_, &event);
		if(ret < 0)
		{
			std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
			exit(6);
		}
		// 2.开始epoll_wait
		struct epoll_event revents[revent_num]; // 需要用户开空间
		int timeout = 2000;
		while(true)
		{
			int n = epoll_wait(epfd_, revents, revent_num, timeout);
		  switch(n)
		  {
		  	// timeout
		  	case 0:
		  		std::cout << "time out" << std::endl;
		  		break;
		  	// 失败
		  	case -1:
		  		std::cout << "error:" << errno << " : " << strerror(errno) << std::endl;
		  		break;
		  	// 返回事件就绪的文件描述符的个数
		  	default:
		  		// std::cout << "listenSock is ready" << std::endl;
					// n>0表示就绪的文件描述符个数，就是revents的前n个！！
					// 就不用像select和poll那样遍历整个文件描述符表！！
		  		handlerEvent(revents, n);
		  		break;
		  }
		}
	}
private:
	void handlerEvent(struct epoll_event* revents, int num)
	{
		for(int i = 0; i < num; ++i)
		{
			int fd = revents[i].data.fd;
			uint32_t revent = revents[i].events;
			if(revent & EPOLLIN)
			{
				// 读事件就绪
				if(fd == listenSock_)
				{
					// 是监听套接字
					std::string clientIp;
					uint16_t clientPort = 0;
					int sock = Sock::Accept(fd, &clientIp, &clientPort);
					// 把sock托管给epfd_，让epoll模型处理等的事情
		      struct epoll_event event;
		      event.data.fd = sock;
		      event.events = EPOLLIN;
		      int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, sock, &event);
		      if(ret < 0)
		      {
		      	std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
		      	exit(6);
		      }
				}
				else
				{
					// 普通套接字
					int ret = func_(fd);    // 回调
					if(ret == 0)
					{
						std::cout << "对端关闭连接" << std::endl;
						// 把这个套接字从epoll模型中删掉，删除的话最后一个参数给nullptr就可以
						int n = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
		        if(n < 0)
		        {
		        	std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
		        	exit(6);
		        }
						// 先从epoll中移除，再关闭fd！！
						close(fd);
					}
					else if(ret < 0)
					{
						std::cout << "recv error" << std::endl;
						// 把这个套接字从epoll模型中删掉，删除的话最后一个参数给nullptr就可以
						int n = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
		        if(n < 0)
		        {
		        	std::cout << "epoll_ctl error: " << strerror(errno) << std::endl;
		        	exit(6);
		        }
						close(fd);
					}
				}
			}
			else if(revent & EPOLLOUT)
			{
				// 写事件就绪
			}
			// else
		}
	}

private:
	int listenSock_;
	int epfd_;				// epoll模型
	uint16_t port_;		// 端口
	func_t func_;     // 回调,处理读事件就绪的方法
};
