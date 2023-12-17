#include <iostream>
#include <unistd.h>
#include "sock.hpp"

// 基于select的多路转接，使用单进程处理多个连接

// 由于select的fd_set是输入输出型参数，所以每一次调用都需要重新设置这些参数！！
// 这就需要自己把所有的fd统一保存一份，不然就找不到了
#define DEFAULT_FD -1
#define FDNUM (sizeof(fd_set) * 8)
int fdsArray[FDNUM];

static void Usage(std::string argv)
{
	std::cerr << "\nUsage: " << argv << " port\n" << std::endl;
}

static void showFds()
{
	for(int i = 0; i < FDNUM; ++i)
	{
		if(fdsArray[i] != DEFAULT_FD) std::cout << fdsArray[i] << " ";
	}
	std::cout << std::endl;
}

void handlerEvent(int listenSock, fd_set& readfds /* fd_set& writefds, fd_set& exceptionfds */)
{
	for(int index = 0; index < FDNUM; ++index)
	{
		if(fdsArray[index] == DEFAULT_FD) continue;
		else if(index == 0 && fdsArray[index] == listenSock)
		{
			// 监听套接字事件就绪
	    if(FD_ISSET(listenSock, &readfds))
	    {
	    	std::string clientIp;
	    	uint16_t clientPorti = 0;

	    	// 此时accept一定不会阻塞了，因为等的过程已经由select完成了
	    	int sock = Sock::Accept(listenSock, &clientIp, &clientPort);
	    	if(sock > 0)
	    		std::cout << "get a new connection,clinetIp: " << clientIp << ", clientPort: " << \
						clientPort << std::endl;

	    	// 此时不是直接进行read/write，因为这些操作也需要 等+数据拷贝
	    	// 而是把获取的sockfd给select，让select去等！！
	    	int i = 0;
	    	for(; i < FDNUM; ++i)
	    	{
	    		if(fdsArray[i] == DEFAULT_FD)
	    		{
	    			fdsArray[i] = sock;
						showFds();
	    			break;
	    		}
	    	}
	    	if(i == FDNUM)
				{
				 	std::cout << "fd已经达到上限！" << std::endl;
					close(sock);
				}
	    }
		}
		else
		{
			// 其他套接字事件就绪
			if(FD_ISSET(fdsArray[index], &readfds))
			{
				// 确定这个套接字的事件就绪了，进行相应事件的处理，这里为了简单，只进行读操作
				char buffer[1024];
				// 这里读取有问题：无法保证数据一定被读取完了！而下一次这些数据就没有了
				ssize_t ret = recv(fdsArray[index], buffer, sizeof(buffer) - 1, 0); // 不会阻塞了！
				if(ret > 0)
				{
					buffer[ret] = 0;
					std::cout << buffer << std::endl;
				}
				else if(ret == 0)
				{
					// 对端关闭连接
					std::cout << "对端关闭连接" << std::endl;
					close(fdsArray[index]);
					fdsArray[index] = DEFAULT_FD;
				}
				else
				{
					// 出错
					std::cout << "recv error" << std::endl;
					close(fdsArray[index]);
					fdsArray[index] = DEFAULT_FD;
				}
			}
		}
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

	memset(&fdsArray, -1, sizeof(fdsArray));
	fdsArray[0] = listenSock;

	while(true)
	{
		// select使用同一张位图作为输入输出型参数，所以每一次调用select都需要重新设置位图
	  int maxFd = DEFAULT_FD;
	  fd_set readfds;
	  FD_ZERO(&readfds);
		for(int i = 0; i < FDNUM; ++i)
		{
			if(fdsArray[i] != DEFAULT_FD)
			{
				FD_SET(fdsArray[i], &readfds);
				maxFd = std::max(maxFd, fdsArray[i]);
			}
		}
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
				// std::cout << "listenSock is ready" << std::endl;
				handlerEvent(listenSock, readfds /* writefds, exceptionfds */);
				break;
		}
	}

	return 0;
}
