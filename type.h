#ifndef TYPE_H
#define TYPE_H

#define NPROC		9
#define SSIZE	 1024
#define NULL		0
#define NFD		   10

#define NOFT	   20
#define READ_PIPE	4
#define WRITE_PIPE	5
#define PSIZE	   10
#define NPIPE	   10

//       PROC status         //
#define FREE		0
#define READY		1
#define RUNNING		2
#define STOPPED		3
#define SLEEP		4
#define ZOMBIE		5


typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned long   u32;

typedef struct pipe
{
	char	buf[PSIZE];
	int		head, tail, data, room;
	int		nreader, nwriter;	// number of readers and writers
	int		busy;
}PIPE;

typedef struct oft
{
	int		mode;		// READ, WRITE, READ_PIPE, WRITE_PIPE, etc
	int		refCount;
	//struct pipe *pipe_ptr;
	PIPE	*pipe_ptr;
	//INODE *inodePtr;
	//long offset;		// for ordinary files
}OFT;

//extern typedef struct proc;
typedef struct proc
{
	struct	proc *next;
	int		*ksp;               // at offset 2
	int		uss, usp;           // at offsets 4,6
	int		pid;                // add pid for identify the proc
	int		status;             // status = FREE|READY|RUNNING|SLEEP|ZOMBIE    

	int		ppid;               // parent pid
	struct	proc *parent;
	int		priority;
	int		event;
	int		exitCode;
	
	OFT		*fd[NFD];
	char	name[32];

	int		kstack[SSIZE];      // per proc stack area
}PROC;

extern PROC proc[NPROC];
extern PROC *running;
extern PROC *freeList;
extern PROC *readyQueue;
extern PROC *sleepList;

/****************** int.c ******************/
int kcinth();

/**************** kernel.c *****************/
PROC *getproc();
int copy_image(u16 child_segment);
int do_fork();
int do_exec(int b);
int kfork(char *filename);
int body();
int do_tswitch();
int do_kfork();
int do_wait();
int do_exit();
int do_ps();
int kmode();
int chname();

/******************* t.c *******************/
int init();
int scheduler();
int int80h();
int set_vec(u16 vector, u16 handler);
main();

/****************** wait.c *****************/
PROC *findZombieChild(int processid);
int enterWait();
int kwait(int *status);
int enterSleepEvent();
int enterWakeupEvent();
int ksleep(int event);
int kwakeup(int event);
int hasChild(int processid);
int printQueue (PROC **queue);
int enqueue(PROC **queue, PROC *added);
PROC *dequeue (PROC **queue);
int dequeueSleep(PROC **queue, PROC *sleepyProc);
int enterExit();
int kexit(int value);
int giveChildrenAway(int parent);

/****************** pipe.c *****************/
int show_pipe(PIPE *p);
int pfd();
int read_pipe(int fd, char *buf, int n);
int write_pipe(int fd, char *buf, int n);
int numProcEmptyFds (PROC *p);
PIPE *getFreePipe();
OFT *getFreeOft(int **arrayIndex);
int addFD(PROC *p, OFT *o);
int kpipe(int pd[2]);
int close_pipe(int fd);

//	put_word(u16 word, u16 segment, u16 offset) in mtxlib


#endif