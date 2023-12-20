#include <memory>
#include <sys/epoll.h>

#include "epollServer.hpp"

static void Usage(std::string argv)
{
	std::cerr << "\nUsage: " << argv << " port\n" << std::endl;
}
// 读取数据后的处理回调
void handlerRequest(Connection* conn, std::string& message)
{
	std::cout << "fd: " << conn->sock_ << " recv: " << message << std::endl;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		Usage(argv[0]);
		exit(-1);
	}

	// 使用只能指针
	std::unique_ptr<EpollServer> epollServer(new EpollServer(atoi(argv[1]), handlerRequest));
	epollServer->initServer();
	epollServer->run();

	return 0;
}
