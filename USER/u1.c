#include "ucode.c"

int color;

main()
{
	char name[64];
	int pid, cmd, segment;

	while(1)
	{
		pid = getpid();
		color = 0x000B + (pid % 5);	// avoid black on black baground
		segment = (pid+1)*0x1000; 
		printf("==============================================\n");
		printf("I am proc %d in U mode: running segment=%x\n", pid, segment);
		show_menu();
		printf("Command ? ");
		gets(name); 
		if (name[0] == 0) 
			continue;

		cmd = find_cmd(name);
//printf("cmd = %d\n", cmd);
		switch(cmd)
		{
			case 0 : getpid();		break;
			case 1 : ps();			break;
			case 2 : chname();		break;
			case 3 : kmode();		break;
			case 4 : kswitch();		break;
			case 5 : wait();		break;
			case 6 : exit();		break;
			case 7 : fork();		break;
			case 8 : exec();		break;
			case 9 : pipe();		break;
			case 10: pfd();			break;
			case 11: read_pipe();	break;
			case 12: write_pipe();	break;
			case 13: close_pipe();	break;

			default: invalid(name);	break;
		}
	}
}