#include <string>
#include <iostream>

#include <fcntl.h>

using namespace std;

bool SetNoneBlock(int fd)
{
	// 获取fd的属性
	int flag = fcntl(fd, F_GETFL);
	if(flag < 0) return false;
	// 设置属性，加上NONBLOCK属性
	int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	if(ret < 0) return false;
	return true;
}

int main()
{
	string s;
	SetNoneBlock(0);
	while(true)
	{
		cin >> s;
		cout << "#" << s << endl;
	}
	return 0;
}
