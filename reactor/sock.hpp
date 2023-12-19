#pragma once

#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>

class Sock
{
	// 监听时全连接队列的长度
	static const int gbacklog = 10;

public:
	static int Socket()
	{
		// 1.创建套接字
	  int listenSock = socket(PF_INET, SOCK_STREAM, 0);
	  if(listenSock < 0)
	  	exit(1);
	  return listenSock;
	}
	static void Bind(int sockfd, uint16_t port)
	{
		// 2.绑定
		struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_port = port;
		local.sin_family = PF_INET;
		local.sin_addr.s_addr = INADDR_ANY;

		if(bind(sockfd, (const struct sockaddr*)&local, sizeof(local)) < 0)
			exit(2);
	}
	static void Listen(int sockfd)
	{
		// 3.监听，第二个参数是全连接队列的长度
		if(listen(sockfd, gbacklog) < 0)
			exit(3);
	}
	static int Accept(int sockfd, std::string* clientIp, uint16_t* clientPort)
	{
		// 4.获取连接
		struct sockaddr_in peer;
		socklen_t len = sizeof peer;
		int serviceSock = accept(sockfd, (struct sockaddr*)&peer, &len);
		if(serviceSock < 0)
			return -1;

		if(clientIp) *clientIp = inet_ntoa(peer.sin_addr);
		if(clientPort) *clientPort = ntohs(peer.sin_port);

		return serviceSock;
	}
};



