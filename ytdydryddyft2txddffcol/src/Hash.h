#pragma once

class gta_hash
{
public:
	static unsigned int get(char *str)
	{
		char *temp = str;
		unsigned int v = 0;
		if(*str == '"')
			temp = str + 1;
		for(char i = *temp; *temp; i = *temp)
		{
			if(*str == '"' && i == '"')
				break;
			++temp;
			if((unsigned __int8)(i - 65) > 25)
			{
				if(i == '\\')
					i = '/';
			}
			else
				i += 32;
			v = (1025 * (v + i) >> 6) ^ 1025 * (v + i);
		}
		return 32769 * (9 * v ^ (9 * v >> 11));
	}
};

#define HASH gta_hash::get