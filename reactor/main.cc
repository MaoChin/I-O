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
	// 这里就拿到了一个个完整的报文，完善的话需要进行反序列化获取具体的数据
	// 然后进行业务处理，再将结果序列化发送给客户端。

	// ...
	// 业务处理完成后将result直接放到发送缓冲区里。
	// 这里可不能直接调用回调sender_!!需要epoll模型中事件就绪才能调用！！
	// conn->outbuffer_ += result;

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
