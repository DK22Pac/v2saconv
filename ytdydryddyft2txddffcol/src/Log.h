#pragma once

#include <Windows.h>
#include <stdio.h>

class _log
{
	FILE *file;
public:
	_log()
	{
		file = 0;
	}

	void close()
	{
		if(file)
		{
			fputs("Log closed", file);
			fclose(file);
			file = 0;
		}
	}

	void open()
	{
		file = fopen("log.txt", "wt");
		fputs("Log started\n", file);
	}

	void restart()
	{
		if(file)
		{
			fclose(file);
			file = fopen("log.txt", "at");
		}
	}

	void put(char *line, ...)
	{
		char text[1024];
		va_list ap;
		va_start(ap, line);
		vsnprintf(text, 1024, line, ap);
		va_end(ap);
		printf(text);
		if(file)
			fputs(text, file);
	}

	void put_line(char *line, ...)
	{
		if(file)
		{
			char text[1024];
			va_list ap;
			va_start(ap, line);
			vsnprintf(text, 1024, line, ap);
			va_end(ap);
			printf(text);
			printf("\n");
			if(file)
			{
				fputs(text, file);
				fputs("\n", file);
			}
		}
		restart();
	}
};

_log gta_log;

#define LOG gta_log.put
#define LOGL gta_log.put_line
#define LOG_RESTART gta_log.restart
#define LOG_OPEN gta_log.open
#define LOG_CLOSE gta_log.close