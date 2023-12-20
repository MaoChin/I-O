#pragma once

#include <cstdint>
#include <iostream>
#include <vector>
#include <cerrno>
#include <cassert>
#include <sys/types.h>
#include <unordered_map>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>

#include "sock.hpp"
#include "epoll.hpp"
#include "util.hpp"
#include "protocol.hpp"

// 基于Reactor模式的EpollServer---ET工作模式
// 单进程，即负责I/O，又负责事件派发; 还负责进行业务处理(这个后续可以使用多线程分出去)

// 回调类型
class EpollServer;
class Connection;
// 事件就绪后的回调处理
using func_t = std::function<int(Connection*)>;
// 读取数据后的反序列化处理
using callback = std::function<void(Connection*, std::string&)>;

struct Connection
{
	int sock_;               // 套接字
	std::string inbuffer_;   // 接受缓冲区
	std::string outbuffer_;  // 发送缓冲区

	func_t recver_;          // 读取回调
	func_t sender_;          // 发送回调
	func_t excepter_;        // 异常回调
	EpollServer* R_;				 // 回指指针

	Connection(int sock, EpollServer* r)
		:sock_(sock)
		,R_(r)
	{}
};


class EpollServer
{
public:
	EpollServer(int port = 8080, callback cb = nullptr)
		:listenSock_(-1)
		,epfd_(-1)
		,port_(port)
		,revs_num_(64)
		,revs_(new struct epoll_event[revs_num_])
		,cb_(cb)
	{}
	~EpollServer()
	{
		if(listenSock_ != -1) close(listenSock_);
		if(epfd_ != -1) close(epfd_);
		if(revs_) delete[] revs_;
	}

  int Accepter(Connection* conn)
  {
		// ET模式下，listenSock_每一次读取也要把底层的所有数据读取完！不然后面就不通知了
		while(true)
		{
  	  std::string clientIp;
  	  uint16_t clientPort = 0;
  	  int newSock = Sock::Accept(conn->sock_, &clientIp, &clientPort);
  	  if(newSock < 0)
  	  {
				// 一样的，accept的返回值也有讲究
				if(errno == EAGAIN || errno == EWOULDBLOCK) break;
				else if(errno == EINTR) continue;
				else
				{
					std::cerr << "Sock::Accept error" << std::endl;
					conn->excepter_(conn);
					return -1;
				}
  	  }
  	  // 把newSock添加到epoll模型中
		  std::cout << "get a new link, clientIp: " << clientIp << ", clientPort: " \
		   	<< clientPort << std::endl;
		  // 针对不同的文件描述符设置不同的回调
  	  conn->R_->addConnection(newSock, EPOLLIN|EPOLLET, std::bind(&EpollServer::Recver, this, \
		  			std::placeholders::_1), std::bind(&EpollServer::Sender, this, std::placeholders::_1),\
						std::bind(&EpollServer::Excepter, this, std::placeholders::_1));
		}
  	return 0;
  }

	int Recver(Connection* conn)
	{
		char buffer[1024];
		// ET模式要死循环，一直读取
		while(true)
		{
		  ssize_t ret = recv(conn->sock_, buffer, sizeof(buffer) - 1, 0);
		  if(ret > 0)
		  {
				buffer[ret] = 0;
				conn->inbuffer_ += buffer;
				std::cout << buffer << std::endl;
		  }
		  else if(ret == 0)
		  {
		  	std::cout << "对端关闭连接" << std::endl;
				conn->excepter_(conn);
				break;
		  }
		  else
		  {
				// 这两个标记意味着把底层的数据全部读取完了！
				if(errno == EAGAIN || errno == EWOULDBLOCK)
				{
					// std::cout << "errno : " << errno << std::endl;
				 	break;
				}
				// 这个标记意思是因为信号而中断了！并没有读完
				else if(errno == EINTR)
				{
					// std::cout << "errno : " << errno << std::endl;
					continue;
				}
				else
				{
					// 出错
					std::cout << "recv error" << std::endl;
					// 调用异常回调
					conn->excepter_(conn);
					break;
				}
		  }
		}
		// 这样才能把底层的数据全部读取完！！并保存在每个conn的inbuffer里
		// 向上交付时还需要进行序列化反序列化！
		// std::cout << "所有数据提取完成" << std::endl;
		std::vector<std::string> messages;
		messageSplit(conn->inbuffer_, &messages);
		// std::cout << "数据split完成" << std::endl;
		for(auto& message : messages)
		{
			cb_(conn, message);
		}

		return 0;
	}
	int Sender(Connection* conn)
	{
		// 这里也要死循环写！

	}
	int Excepter(Connection* conn)
	{
		assert(conn);
		if(connections_.find(conn->sock_) != connections_.end())
		{
		  Epoll::EpollDel(epfd_, conn->sock_);

		  // 先把套接字从epoll模型中去掉，然后关闭套接字
		  close(conn->sock_);
		  delete connections_[conn->sock_];
			connections_.erase(conn->sock_);
		}
	}

	void initServer()
	{
		// 1.创建套接字-》绑定-》监听
		listenSock_ = Sock::Socket();
		// 设置成非阻塞
		Util::setNoneBlock(listenSock_);
		Sock::Bind(listenSock_, port_);
		Sock::Listen(listenSock_);

		epfd_ = Epoll::EpollCreate();
		// 2.添加listenSock_到epoll模型中
		addConnection(listenSock_, EPOLLIN|EPOLLET, std::bind(&EpollServer::Accepter, this, \
					std::placeholders::_1), nullptr, nullptr);
	}

	// 添加套接字
	void addConnection(int newfd, uint32_t event, func_t recver, func_t sender, func_t excepter)
	{
		if(event & EPOLLET)
			Util::setNoneBlock(newfd);

		// 把newfd添加到epoll模型，并设置成ET模式
		bool ret = Epoll::EpollAdd(epfd_, newfd, event);
		if(!ret) exit(7);
		// 把newfd和Connection添加到hash里
		Connection* conn = new Connection(newfd, this);
		conn->recver_ = recver;
		conn->sender_ = sender;
		conn->excepter_ = excepter;
		connections_.insert({newfd, conn});
		std::cout << "add " << newfd << " to Connection success" << std::endl;
	}

	// 事件派发
	void dispatch()
	{
		// 1.获取就绪文件描述符
		// 返回值：获取到的事件就绪的文件描述符的个数
		int ret = Epoll::EpollWait(epfd_, revs_, revs_num_);
		for(int i = 0; i < ret; ++i)
		{
			// 2.进行派发
			int fd = revs_[i].data.fd;
			uint32_t revent = revs_[i].events;
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
			// EPOLLHUB是指对端连接关闭。设置读事件就绪，那么下一次读时就一定会出错
			// 出错时也设置读事件就绪，这样就把所有的异常规整到了读出错那里！！！
			if(revent & EPOLLERR)
				revent |= (EPOLLIN | EPOLLOUT);
			if(revent & EPOLLHUP)
				revent |= (EPOLLIN | EPOLLOUT);
		}
	}

	void run()
	{
		while(true)
			dispatch();
	}

private:
	int listenSock_;         // 监听套接字
	int epfd_;							 // epoll模型
	uint16_t port_;					 // 端口

	// fd -> Connection 把epoll模型中的fd和底层连接做映射,这里是双向映射，因为Connection类里也有fd
	std::unordered_map<int, Connection*> connections_;
	int revs_num_;
	struct epoll_event *revs_;

	// 读取数据处理的回调
	callback cb_;
};


