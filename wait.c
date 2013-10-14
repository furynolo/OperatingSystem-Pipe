#include "type.h"

//PROC *freeList, *readyQueue, *sleepList;

int hasRegularChild(int parentPid)
{
	int i;

	for (i = 0; i < NPROC; i++)	// Cycle through all processes
	{
		// If the process's ppid equals passed in parent proc pid and status is not ZOMBIE
		if ((proc[i].ppid == parentPid) && (proc[i].status != ZOMBIE))
			return 1; // return that a regular child has been found
	}

	return 0;	// no non-ZOMBIE children found
}

// searches all PROCs for a ZOMBIE child of the passed in PROC and returns it, otherwise 0
PROC *findZombieChild(int parentPid)
{
	int i;

	for (i = 0; i < NPROC; i++)	// Cycle through all processes
	{
		// If the process's ppid equals passed in parent proc pid and status is ZOMBIE
		if ((proc[i].ppid == parentPid) && (proc[i].status == ZOMBIE))
			return &(proc[i]); // return the ZOMBIE child PROC
	}

	return 0;	// no ZOMBIE child found
}

// 
int enterWait()
{
	int status, returnValue;

	returnValue = kwait(&status);
	printf("kwait() returned on pid=%d\n", returnValue);

	return 0;
}

// a proc calls pid=wait to wait for A/ANY child to die, where pid is the dead child's pid and status is the child's exit value.
int kwait(int *status)
{
	PROC *child = 0;

	if (0 == hasChild(running->pid))
	{
		printf("PROC %d has no children. Exiting code execution.\n", running->pid);
		return -1; // ERROR
	}
	while (1)
	{
		child = findZombieChild(running->pid);

		if (child != 0)
		{
			printf("ZOMBIE child found! Freeing proc %d.\n", child->pid);
			// copy child's exit value to *status
			*status = child->exitCode;

			// free the ZOMBIE child's proc
			child->status = FREE;
			child->ppid = -1;
			child->parent = 0;
			child->exitCode = -1;
			child->event = -1;
			enqueue(&freeList, child);

			return child->pid;	// Return dead child's pid
		}
		printf("PROC %d did not find any ZOMBIE children, so they are going to sleep.\n", running->pid);
		ksleep(running);	// Sleep at its own &PROC
	}

	return 9001; // Should not reach here, stuck in the while loop
}

// a proc calls sleep to sleep on a manually entered event
int enterSleepEvent()
{
	char input;
	int sleepEvent, returnValue;
	sleepEvent = -1;

	while ((sleepEvent < 0) || (sleepEvent > 10))
	{
		printf("Enter sleep event (0-9): ");
		input = getc();
		printf("%c\n", input);
		sleepEvent = (int)(input-'0');
	}

	returnValue = ksleep(sleepEvent);

	return 0;
}

int enterWakeupEvent()
{
	char input;
	int wakeupEvent, returnValue;
	wakeupEvent = -1;

	while ((wakeupEvent < 0) || (wakeupEvent > 10))
	{
		printf("Enter wakeup event (0-9): ");
		input = getc();
		printf("%c\n", input);
		wakeupEvent = (int)(input-'0');
	}

	returnValue = kwakeup(wakeupEvent);

	printf("%d PROC(s) just woke up!\n", returnValue);

	return 0;
}

// a proc calls sleep to sleep on an event
int ksleep(int event)
{
	running->event = event;		// Record event in PROC
	running->status = SLEEP;	// Fall aSLEEP

	// Add running to FIFO sleeplist (for fairness we want them to wake up in order; don't pick favorites)
	enqueue(&sleepList, running);	//enqueueSleep(&sleepList, running);

	tswitch();					// Not in readyQueue anymore

	return 0;
}

// a proc calls wakeup to wake up ALL procs sleeping on an event, returns number of PROC's awoken
int kwakeup(int event)
{
	int counter;
	PROC *next;
	PROC *tmp = sleepList;
	counter = 0;

	// For every PROC sleeping on this event: remove from sleepQueue, set status to READY, add to readyQueue
	while (tmp != 0)
	{
		if ((tmp->status == SLEEP) && (tmp->event == event))
		{
			counter++;
			next = tmp->next;//next = dequeueSleep(&sleepList, tmp);
			dequeueSleep(&sleepList, tmp);
			tmp->status = READY;
			enqueueReadyQueue(&readyQueue, tmp);
			tmp = next;
		}
		else
		{
			tmp = tmp->next;
		}
	}

	return counter;
}

// checks all PROCs to see if passed in PROC is a parent
int hasChild(int processid)
{
	int i;

	for (i = 0; i < NPROC; i++)
	{
		// process with parent of passed in id (processid) was found (and isn't the parent of itself)
		if ((proc[i].pid != processid) && (proc[i].ppid == processid))
			return 1;
	}

	return 0; // no process has a ppid equal to passed in id (processid)
}

// Generic function to print queue
int printQueue (PROC **queue)
{
	PROC *tmp;
	tmp = *queue;

	printf("Queue:\t");
	while (tmp != 0)
	{
		printf("%d(%d)->", tmp->pid, tmp->priority);
		tmp = tmp->next;
	}
	printf("NULL\n");

	return 0;
}

PROC *previousProc(PROC **queue, PROC *p) // Return the NODE that is in front (opposite direction of the next pointer) of this node
{
	PROC *tmp;
	tmp = *queue;

	if (tmp == 0)
		return 0;
	while (tmp->next != 0)
	{
		if (tmp->next == p)
			return tmp;
		tmp = tmp->next;
	}

	return *queue; // proc wasnt found (or front is proc being searched for), return front of queue
}

// enqueue PROC *added to the readyQueue based on priority (higher priority is in front of lower priorities)
int enqueueReadyQueue (PROC **queue, PROC *added)
{
	PROC *tmp;
	
	added->next = 0;
	tmp = *queue;
	printf("Enqueuing proc: %d(%d)\n", added->pid, added->priority);

	if (tmp == 0)
	{
		*queue = added;
		return 0;
	}

	// iterate through until the last proc w/ the same priority or the last NODE in the list
	while ((tmp->next != 0) && (tmp->next->priority >= added->priority))
		tmp = tmp->next;
	if (added->priority > tmp->priority)
	{
		tmp = previousProc(queue, tmp);
		if (tmp == *queue)
		{
			added->next = tmp;
			*queue = added;
			return 0;
		}
	}
	added->next = tmp->next;
	tmp->next = added;

	return 0;
}

// Enqueue a PROC to end of queue
int enqueue(PROC **queue, PROC *added)
{
	PROC *tmp = *queue;
	printf("Enqueuing %d(%d)\n", added->pid, added->priority);
	if (tmp == 0) // queue is empty, added PROC is only in list
	{
		added->next = 0;
		*queue = added;
		return 1;
	}
	while (tmp->next != 0) // find last item in queue
		tmp = tmp->next;

	tmp->next = added; // add new process to end of queue
	added->next = 0;

	return 1;
}

PROC *dequeue (PROC **queue)	// removes first node from the queue
{
	PROC *tmp;
	tmp = *queue;

	if (!tmp)
	{
		printf("Nothing to dequeue.\n");
		return 0;
	}

	*queue = tmp->next;
	tmp->next = 0;	// set next pointer to null
	printf("Dequeuing proc: %d(%d)\n", tmp->pid, tmp->priority);

	return tmp;
}

// Dequeue a specific PROC from the sleepQueue
int dequeueSleep(PROC **queue, PROC *sleepyProc)
{
	PROC *prev;
	PROC *tmp;
	PROC *next;

	tmp = *queue;

	if (!tmp)
	{
		printf("dequeueSleep(): Empty queue was passed in. Unable to dequeue proc %d(%d).\n", sleepyProc->pid, sleepyProc->priority);
		return -1;
	}

	if (tmp == sleepyProc)
	{
		*queue = sleepyProc->next;
		sleepyProc->next = 0;
		return 0;
	}

	while ((tmp != 0) && (tmp->next != 0))
	{
		if (tmp->next == sleepyProc)
		{
			tmp = sleepyProc->next;
			sleepyProc->next = 0;
			return 0;
		}
		tmp = tmp->next;
	}

	printf("dequeueSleep(): Unable to locate PROC %d(%d) in the queue.\n", sleepyProc->pid, sleepyProc->priority);
	return -1;
}

int enterExit()
{
	char input;
	int exitCode, returnValue;
	exitCode = -1;

	while ((exitCode < 0) || (exitCode > 10))
	{
		printf("Enter exit value (0-9): ");
		input = getc();
		printf("%c\n", input);
		exitCode = (int)(input-'0');
	}
	returnValue = kexit(exitCode);

	return 0;
}


int kexit(int value)
{
// NOTE:	When a PROC dies it is marked as ZOMBIE, contains an exit value, but the PROC is not free yet.
//		It will be freed by the parent PROC with the wait() command. 
	if ((running->pid == 1) && (hasChild(running->pid))) //if ((running == &proc[1]) && (hasChild(running->pid)))
	{
		printf("Cannot kill P1 while it has children, try again later!\n");
		return -1;
	}
	// Mark itself as a ZOMBIE.
	printf("PROC %d is now a ZOMBIE!!\n", running->pid);
	running->status = ZOMBIE;
	// Record an exit value in its PROC.exitCode.
	running->exitCode = value;
	// Give away children (dead or alive) to P1. Make sure P1 is alive while other PROCs still exist.
	giveChildrenAway(running->pid);
	// Issue wakeup(&parentPROC) to wakeup its parent.
	kwakeup(running->parent);

	// The dying task must close its opened file descriptors (which 
	//	may free the OFTEs and/or pipes .... if it's the last task).
// TODO: 

	// Call tswitch() to free up the CUP (for the last time).
	tswitch();

	return 0;
}


int giveChildrenAway(int parentPid)
{
	int i;

	for (i = 0; i < NPROC; i++)	// Cycle through all processes
	{
		if (proc[i].ppid == parentPid)	// If the process's ppid equals passed in parent proc pid then set it to PROC 1
		{
			proc[i].parent = &proc[1];
			proc[i].ppid = proc[1].pid;
		}
	}

	return 0;	// no ZOMBIE child found
}