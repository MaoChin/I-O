#pragma once

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>

#include "sock.hpp"
#include "epoll.hpp"

// 基于Reactor模式的EpollServer---ET工作模式

// 回调类型
using func_t = std::function<int(Connection*)>;

struct Connection
{
	int sock_;               // 套接字
	std::string inbuffer_;   // 接受缓冲区
	std::string outbuffer_;  // 发送缓冲区

	func_t recver_;          // 读取回调
	func_t sender_;          // 发送回调
	func_t excepter_;        // 异常回调
};


class EpollServer
{
	EpollServer(int port = 8080)
		:listenSock_(-1)
		,epfd(-1)
		,port_(port)
		,revs_num_(64)
		,revs_(new struct epoll_event[revs_num_])
	{}
	~EpollServer()
	{
		if(listenSock_ != -1) close(listenSock_);
		if(epfd_ != -1) close(epfd_);
		if(revs_) delete[] revs_;
	}

	void initServer()
	{
		// 1.创建套接字-》绑定-》监听
		listenSock_ = Sock::Socket();
		Sock::Bind(listenSock_, port_);
		Sock::Listen(listenSock_);

		epfd_ = Epoll::EpollCreate();
		// 2.把listenSock_添加到epoll模型，并设置成ET模式
		bool ret = Epoll::EpollAdd(epfd_, listenSock_, EPOLLIN|EPOLLET);
		if(!ret) exit(7);
		// 3.把listenSoc_和Connection添加到hash里
		connections_.insert({listenSock_, new Connection(listenSock_)});
	}

	// 事件派发
	void dispatch()
	{
		// 1.获取就绪文件描述符
		// 返回值：获取到的事件就绪的文件描述符的个数
		int ret = Epoll::EpollWait(epfd_, revs_, revs_num_);
		for(int i = 0; i < n; ++i)
		{
			int fd = revs_[i].data.fd;
			struct epoll_event revent = revs_[i].events;
			if(revent & EPOLLIN)
			{
			  if(connections_.find(fd) != connections_.end() && \
			  		connections_[fd]->recver_)
			  	connections_[fd]->recver_(connections_[fd]);
			}
			if(revent & EPOLLOUT)
			{
			  if(connections_.find(fd) != connections_.end() && \
			  		connections_[fd]->sender_)
			  	connections_[fd]->sender_(connections_[fd]);
			}
			if(revent & EPOLLERR)
			{
			  if(connections_.find(fd) != connections_.end() && \
			  		connections_[fd]->excepter_)
			  	connections_[fd]->excepter_(connections_[fd]);
			}
		}
	}

	void run()
	{

	}

private:
	int listenSock_;         // 监听套接字
	int epfd_;							 // epoll模型
	uint16_t port_;					 // 端口

	// fd -> Connection 把epoll模型中的fd和底层连接做映射,这里是双向映射，因为Connection类里也有fd
	std::unordered_map<int, Connection*> connections_;
	int revs_num_;
	struct epoll_event *revs_;
};


