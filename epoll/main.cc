#include <memory>
#include <sys/types.h>

#include "epollServer.hpp"

static void Usage(std::string argv)
{
	std::cerr << "\nUsage: " << argv << " port\n" << std::endl;
}

int handlerRead(int fd)
{
	// 读取
	char buffer[1024];
	// 这次读取不会被阻塞
	ssize_t ret = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if(ret > 0)
	{
		buffer[ret] = 0;
		std::cout << buffer << std::endl;
	}
	return ret;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
		exit(-1);
	}

	// 使用只能指针
	std::unique_ptr<EpollServer> epollServer(new EpollServer(atoi(argv[1]), handlerRead));
	epollServer->initServer();
	epollServer->runServer();

	return 0;
}
