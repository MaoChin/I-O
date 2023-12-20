#pragma once

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
class Util
{
public:
	static void setNoneBlock(int fd)
	{
		int fl = fcntl(fd, F_GETFL);
		if(fl < 0)
		{
			std::cerr << "fcntl error" << std::endl;
			exit(10);
		}
		fcntl(fd, F_SETFL, fl | O_NONBLOCK);
	}
};
