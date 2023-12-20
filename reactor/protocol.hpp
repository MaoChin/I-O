#pragma once

#include <iterator>
#include <string>
#include <vector>

#define SEP '\3'
#define SEP_LEN (sizeof(SEP))

// 自己定制协议，假设数据格式是 xxxxxx\3xxx\3xxxxxxxxxxxxxx\3
int messageSplit(std::string& inbuffer, std::vector<std::string>* messages)
{
	while(true)
	{
		int pos = inbuffer.find(SEP);
		if(pos == std::string::npos)
			break;
		else
		{
			messages->push_back(inbuffer.substr(0, pos));
			inbuffer.erase(0, pos + SEP_LEN);
		}
	}
	return 0;
}
