/**************************************************
  bio.o, queue.o loader.o are in mtxlib
**************************************************/
/* #include "bio.c" */
/* #include "queue.c" */
/* #include "loader.c" */

#include "wait.c"
#include "kernel.c"
#include "int.c"
#include "pipe.c"
#include "type.h"

PROC proc[NPROC];
PROC *running;
PROC *freeList;
PROC *readyQueue;
PROC *sleepList;
int procSize = sizeof(PROC);
int nproc = 0;

OFT  oft[NOFT];
PIPE pipe[NPIPE];

char *pname[] = { "Sun", "Mercury", "Venus", "Earth",  "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };

int init()
{
	PROC *p;
	int i, j;
	printf("initializing ...");

	for (i=0; i < NPROC; i++)
	{   // initialize all procs
		p = &proc[i];
		p->pid = i;
		p->status = FREE;
		if (i == 0)
			p->priority = 0;
		else
			p->priority = 1;  
		strcpy(proc[i].name, pname[i]);
		p->ppid = -1;
   
		p->next = &proc[i+1];
		for (j = 0; j < NFD; j++)
			p->fd[j] = 0;
	}
	freeList = &proc[0];        // All procs are in freeList.
	proc[NPROC-1].next = 0;     // Set last PROC's next pointer null.
	readyQueue = sleepList = 0; // readyQueue and sleepList are empty.

	for (i = 0; i < NOFT; i++)
	{
		oft[i].mode = 0;
		oft[i].refCount = 0;
		oft[i].pipe_ptr = 0;
	}
	for (i = 0; i < NPIPE; i++)
	{
		pipe[i].busy = 0;
		pipe[i].buf[0] = 0;
		pipe[i].head = 0;
		pipe[i].tail = 0;
		pipe[i].data = 0;
		pipe[i].room = 0;
		pipe[i].nreader = 0;
		pipe[i].nwriter = 0;
		pipe[i].busy = 0;
	}

	/**** create P0 as running ******/
	p = get_proc(&freeList);
	p->status	= RUNNING;
	p->ppid		= 0;
	p->parent	= p;
	running		= p;
	nproc		= 1;
	printf("done\n");

	return 0;
}

int scheduler()
{
	if (running->status == RUNNING)
	{
		running->status = READY;
		enqueueReadyQueue(&readyQueue, running);
	}
	running = dequeue(&readyQueue);
	running->status = RUNNING;

	return 0;
}

int set_vec(u16 vector, u16 handler)
{
//	put_word(word, segment, offset) in mtxlib
	put_word(handler, 0, (vector<<2));
	put_word(0x1000,  0, (vector<<2) + 2);

	return 0;
}
			
main()
{
	printf("\n\nMTX starts in main()\n");
	init();	// initialize and create P0 as running
	set_vec(80, int80h);

	kfork("/bin/u1");	// P0 kfork() P1

	while(1)
	{
		printf("P0 running\n");
		if ((nproc == 2) && (proc[1].status != READY))
			printf("no runable process, system halts\n");
		while(!readyQueue);	// Halts program (well, stuck in loop) when null
		printf("P0 switch process\n");
		tswitch();	// P0 switch to run P1
	}
}