#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
	// solution 2 struct condvar * cv = cv_ret();
	int pid = fork();
	if (pid < 0)
		printf(1, "Error forking first child.\n");

	else if (pid == 0)
	{
		sleep(20);
		printf(1, "Child 1 Executing\n");
		do_cv(0); //Solution 2 cv_signal(cv); //
	}

	else
	{
		pid = fork();
		if (pid < 0)
			printf(1, "Error forking second child.\n");

		else if (pid == 0)
		{
			do_cv(1); //Solution 2 cv_wait(cv); //
			printf(1, "Child 2 Executing\n");

		}

		else
		{
			int i ;
			for (i = 0 ; i < 2 ; i++)
				wait();
		}
	}
	exit();
}