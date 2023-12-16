#include <cstdint>
#include <iostream>
#include <sys/select.h>
#include "sock.hpp"

// 基于select的多路转接

// 由于select的fd_set是输入输出型参数，所以每一次调用都需要重新设置这些参数！！
// 这就需要自己把所有的fd统一保存一份，不然就找不到了
#define FDNUM (sizeof(fd_set))


static void Usage(std::string argv)
{
	std::cerr << "\nUsage: " << argv << " port\n" << std::endl;
}

void handlerEvent(int listenSock, fd_set& readfds /* fd_set& writefds, fd_set& exceptionfds */)
{
	if(FD_ISSET(listenSock, &readfds))
	{
		std::string clientIp;
		uint16_t clientPort;

		// 此时accept一定不会阻塞了，因为等的过程已经由select完成了
		int sock = Sock::Accept(listenSock, &clientIp, &clientPort);
		if(sock > 0)
			std::cout << "get a new connection,clinetIp: " << clientIp << ", clientPort: " << clientPort \
				<< std::endl;

		// 此时不是直接进行read/write，因为这些操作也需要 等+数据拷贝
		// 而是把获取的sockfd给select，让select去等！！
	}
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
		exit(-1);
	}

	// 1.创建套接字
	int listenSock = Sock::Socket();
	// 2.绑定
	Sock::Bind(listenSock, atoi(argv[1]));
	// 3.监听
	Sock::Listen(listenSock);
	// 4.accept，但是accept本质也是进行I/O操作，需要 等+数据拷贝
	//   从这个角度看，accept和read,write没有区别，也需要先丢给select去 等！
	//   只不过accept的套接字是listenSock，这个套接字非常特殊，要永远在readfds里！

	while(true)
	{
		// select使用同一张位图作为输入输出型参数，所以每一次调用select都需要重新设置位图
		// 因为以前的位图数据会被覆盖！
	  int maxFd = listenSock;
	  fd_set readfds;
	  FD_ZERO(&readfds);
	  FD_SET(listenSock, &readfds);
	  // select 在timeout时间内阻塞等待
	  struct timeval timeout{3, 0}; // second, microsecond

		// 所以不能直接accept，要先select
		int ret = select(maxFd + 1, &readfds, nullptr, nullptr, &timeout);
		switch(ret)
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
				 std::cout << "listenSock is ready" << std::endl;
				handlerEvent(listenSock, readfds /* writefds, exceptionfds */);
				break;
		}
	}

	return 0;
}
