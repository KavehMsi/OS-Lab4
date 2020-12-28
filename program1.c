#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{

	int pid , i = 0;
	read_write_initialize();

	pid = fork();

	if (pid > 0)
	{
		pid = fork();

		if(pid > 0)
		{
			pid = fork();
			if(pid > 0)
			{
				pid = fork();
				if(pid > 0)
				{		
					for(i = 0 ; i < 4 ; i += 1)
						wait();
				}
				else
				{
					writer();
				}
			}
			else
			{
				reader();
			}
		}
		else
		{
			writer();
		}
	}
	else
	{
		reader();
	}
	exit();
}