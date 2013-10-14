#include "type.h"

char *MODE[ ] = {"READ_PIPE","WRITE_PIPE"};

extern OFT  oft[NOFT];
extern PIPE pipe[NPIPE];

int show_pipe(PIPE *p)
{
	int i, j;
	
	printf("------------ PIPE CONTENETS ------------\n");     
	printf("head:  %d, tail:  %d, data:  %d, room:  %d\nnreader:  %d, nwriter:  %d, busy:  %d\nbuf = ",
		p->head, p->tail, p->data, p->room, p->nreader, p->nwriter, p->busy);
	// Set iterator to PROC->PIPE->tail.  Increment it by 1 (and mod by the size of the PIPE buffer area) until tail = head.
	for (j = running->fd[i]->pipe_ptr->tail; (j != running->fd[i]->pipe_ptr->head); j = (j + 1) % PSIZE)
	{
		printf("%c\n", running->fd[i]->pipe_ptr->buf[j]);
	}
	printf("\n----------------------------------------\n");

	return 1;
}

// print running process' opened file descriptors
int pfd()
{
	int i, j, counter;

	printf("============v= PFD =v============\n");
	for (i = 0; i < NFD; i++)
	{
		printf("fd[%d]:\t", i);
		if (running->fd[i] != 0)
		{
			if (running->fd[i]->mode == READ_PIPE)
				printf("\n\tmode:  %s, ", MODE[0]);
			else if (running->fd[i]->mode == WRITE_PIPE)
				printf("\n\tmode:  %s, ", MODE[1]);
			printf("refCount:  %d, head:  %d\n\ttail:  %d, data:  %d, room:  %d\n\tnreader:  %d, nwriter:  %d, busy:  %d\n\tbuf = ",
				running->fd[i]->refCount, running->fd[i]->pipe_ptr->head,
				running->fd[i]->pipe_ptr->tail, running->fd[i]->pipe_ptr->data,
				running->fd[i]->pipe_ptr->room, running->fd[i]->pipe_ptr->nreader,
				running->fd[i]->pipe_ptr->nwriter, running->fd[i]->pipe_ptr->busy);
// Set iterator to PROC->PIPE->tail.  Increment it by 1 (and mod by the size of the PIPE buffer area) until we have read "data" number of bytes.
			counter = 0;
			for (j = running->fd[i]->pipe_ptr->tail; (counter < running->fd[i]->pipe_ptr->data); j = (j + 1) % PSIZE)
			{
				printf("%c", running->fd[i]->pipe_ptr->buf[j]);
				counter++;
			}
			printf("\n");
		}
		else
			printf("NULL\n");
	}
	printf("============^= PFD =^============\n");

	return 0;
}

int read_pipe(int fd, char *buf, int n)
{
	u16 segment, offset;
	char name[1024];
	char tmp, word;
	int i, fileDescriptor, numBytesLeftToRead, data;
	PIPE *p;
	tmp = '>';

	i = 0;

	fileDescriptor = fd;

	numBytesLeftToRead = n;//numBytesLeftToRead = get_word(running->uss, n);

//	check if there is data
//		if not then check for a writer
//			if no writer return error
//			if there is writer then sleep on data
//		if there is data then you read as much as you need & it has
	if (running->fd[fileDescriptor] == 0)
	{
		printf("Error 101.  fd[%d] is not open.\n", fileDescriptor);
		return -1;
	}
	if (running->fd[fileDescriptor]->mode != READ_PIPE)
	{
		printf("Error 202.  fd[%d] is not open for reading.\n", fileDescriptor);
		return -2;
	}

	i = 0;
	p = running->fd[fileDescriptor]->pipe_ptr;
	while (numBytesLeftToRead > 0)
	{
		if (p->data == 0)
		{
			if (p->nwriter <= 0)
			{
				printf("Error 303.  fd[%d] has no data to read and there are no writers.\n", fileDescriptor);
				return -3;
			}
			// There is no data but there is a writer so sleep on &p->data
			printf("No data, sleeping until more shows up.");
			ksleep(&(p->data));
		}
		else
		{
			printf("Reading byte\n");
			name[i] = p->buf[p->tail];
			i++;
			p->buf[p->tail] = 0;
			p->data--;
			p->room++;
			p->tail++;
			p->tail %= PSIZE;
			numBytesLeftToRead--;

			kwakeup(&(p->room));
		}
	}

	// Set the last one to NULL
	name[i] = 0;
	i = 0;
	while (name[i] != 0)
	{
		word = name[i];
		segment = (u16)running->uss;
		offset = (u16)(buf + i);
		put_byte(word, segment, offset);
		i++;
	}

	return i; // return number of bytes read
}

int write_pipe(int fd, char *buf, int n)
{
	u16 segment, offset;
	char name[1024];
	char tmp, word;
	int i, fileDescriptor, numBytesLeftToWrite, data, bytesWritten;
	PIPE *p;
	tmp = '>';
	bytesWritten = 0;
	i = 0;
	fileDescriptor = fd;
	numBytesLeftToWrite = n;

	if (running->fd[fileDescriptor] == 0)
	{
		printf("Error 101.  fd[%d] is not open.\n", fileDescriptor);
		return -1;
	}
	if (running->fd[fileDescriptor]->mode != WRITE_PIPE)
	{
		printf("Error 202.  fd[%d] is not open for writing.\n", fileDescriptor);
		return -2;
	}

	// store the message to be written in name[]
	while (i < numBytesLeftToWrite)
	{
		if (i >= 1024)
		{
			printf("write_pipe():  Attempting to write more than %d bytes. Stopping here, other bytes are being ignored.\n", 1024);
			break;
		}
		name[i] = get_byte(running->uss, (buf + i));
//printf("name[%d] = %c\n", i, name[i]);
		i++;
	}

//	--------------------------------------------------------------------------
//	(1). If (no READER on the pipe) ===> return BROKEN_PIPE_ERROR;
//	--------------------------------------------------------------------------
//	  (pipe still have READERs):
//	(2). If (pipe has room)  ===> write as much as it can until all nbytes are
//	                                 written or (3).
//	                          "wakeup" READERs that are waiting for data.  
//	(3)  If (no room in pipe)===>"wakeup" READERs that are waiting for data
//	                          "wait" for room; 
//	                           then try to write again from (1).
//	--------------------------------------------------------------------------
	i = 0;
	p = running->fd[fileDescriptor]->pipe_ptr;

	while (numBytesLeftToWrite > 0)
	{
		if (p->nreader <= 0)
		{
			printf("Error 303.  fd[%d] has no readers assigned to it. \\_^_/BROKEN_PIPE_ERROR\\_^_/\n", fileDescriptor);
			return -3;
		}
		if (p->room == 0)
		{
			// There is no room but there is a reader so sleep on &p->data
			printf("No room, sleeping until more is available.");
			ksleep(&(p->room));
		}
		else // Pipe has room
		{
			printf("Writing byte\n");
			p->buf[p->head] = name[i];
			i++;
			p->data++;
			p->room--;
			p->head++;
			p->head %= PSIZE;
			numBytesLeftToWrite--;
			bytesWritten++;

			kwakeup(&(p->data));
		}
	}

	return bytesWritten; // return number of bytes written
}

int numProcEmptyFds (PROC *p)
{
	int freeFDs, i;
	OFT *tmpOft;

	freeFDs = i = 0;
	tmpOft = running->fd[i];
	while (i < NFD)
	{
		if (tmpOft == 0)
		{
			freeFDs++;
		}
		tmpOft = running->fd[i];
		i++;
	}

	return freeFDs;
}

PIPE *getFreePipe()
{
	PIPE *newPipe;
	int i;

	i = 0;
	newPipe = &pipe[i];
	while (newPipe->busy == 1)
	{
		i++;
		if (i == NPIPE)
		{
			// No free pipes available
			return 0;
		}
		newPipe = &pipe[i];
	}

	return newPipe;
}

OFT *getFreeOft(int *arrayIndex)
{
	OFT *freeOft;
	int i;

	i = 0;
	freeOft = &oft[i];
	while (freeOft->refCount > 0)
	{
		i++;
		if (i == NOFT)
		{
			// No available OFTs
			*arrayIndex = -1;
			return 0;
		}
		freeOft = &oft[i];
	}

	*arrayIndex = i;
	return freeOft;
}

int addFD(PROC *p, OFT *o)
{
	int i;

	i = 0;
	while (p->fd[i] != 0)
	{
		i++;
	}
	p->fd[i] = o;
	printf("PROC %dadded fd[ %d] as: ", p->pid, i, MODE[(p->fd[i]->mode)]);
	if (p->fd[i]->mode == READ_PIPE)
		printf("%s\n", MODE[0]);
	else if(p->fd[i]->mode == WRITE_PIPE)
		printf("%s\n", MODE[1]);
	
	return 0;
}

// create a pipe; fill pd[0] pd[1] (in USER mode!!!) with descriptors
//int kpipe(int pd[2])
int kpipe(int value)
{
	PIPE *newPipe;
	int i, status, arrayIndex;
	u16 word, segment, offset;
	OFT *oftRead, *oftWrite;
	// Check to see if the current PROC has room for two new fd's
	status = numProcEmptyFds(running);

	if (status < 2)
	{
		printf("The running proc does not have enough room to add two more fds.\n");
		return -1;
	}

	// Acquire a pipe
	newPipe = getFreePipe();

	if (newPipe == 0)
	{
		printf("No free pipes at this moment.\n");
		return -1;
	}
	// newPipe is the first avaiable free pipe, we are using it now
	newPipe->busy = 1;
	newPipe->head = 0;
	newPipe->tail = 0;
	newPipe->data = 0;
	newPipe->room = PSIZE;
	// clear out the buffer
	for (i = 0; i < PSIZE; i++)
	{
		newPipe->buf[i] = 0;
/*//delete the lines below this v
//newPipe->buf[i] = 't';
if (i == 0) {newPipe->buf[i] = 'a';}
if (i == 1) {newPipe->buf[i] = 'b';}
if (i == 2) {newPipe->buf[i] = 'c';}
if (i == 3) {newPipe->buf[i] = 'd';}
if (i == 4) {newPipe->buf[i] = 'e';}
if (i == 5) {newPipe->buf[i] = 'f';}
if (i == 6) {newPipe->buf[i] = 'g';}
if (i == 7) {newPipe->buf[i] = 'h';}
if (i == 8) {newPipe->buf[i] = 'i';}
if (i == 9) {newPipe->buf[i] = 'j';}

//delete the lines above this ^*/
	}
/*//delete the lines below this v
newPipe->head = i-1;
newPipe->data = i;
newPipe->room = 0;
//delete the lines above this ^*/

	// Acquire an OFT for reading
	oftRead = getFreeOft(&arrayIndex);
	oftRead->refCount = 1;
	oftRead->mode = READ_PIPE;//oftRead->mode = 0;
	oftRead->pipe_ptr = newPipe;
	oftRead->pipe_ptr->nreader = 1;
	// Send the array index of the reader OFT to umode
	word = arrayIndex;
	segment = running->uss;
	offset = value;
	put_word(word, segment, offset);

	// Acquire an OFT for writing
	oftWrite = getFreeOft(&arrayIndex);
	oftWrite->refCount = 1;
	oftWrite->mode = WRITE_PIPE;//oftWrite->mode = 1;
	oftWrite->pipe_ptr = newPipe;
	oftWrite->pipe_ptr->nwriter = 1;
	// Send the array index of the writer OFT to umode
	word = arrayIndex;
	segment = running->uss;
	offset = (value + 2);
	put_word(word, segment, offset);

	// Add the OFTs to the running PROC
	addFD(running, oftRead);
	addFD(running, oftWrite);

	return 0;
}

int close_pipe(int fd)
{
	OFT *op;
	PIPE *pp;

	printf("proc %d close_pipe: fd=%d\n", running->pid, fd);

	op = running->fd[fd];
	running->fd[fd] = 0;                 // clear fd[fd] entry 

	if (op->mode == READ_PIPE)
	{
		pp = op->pipe_ptr;
		pp->nreader--;                   // dec n reader by 1

		if (--(op->refCount) == 0)
		{
			// last reader
			if (pp->nwriter <= 0)
			{
				// no more writers
				pp->busy = 0;             // free the pipe   
				return;
			}
		}

		kwakeup(&(pp->room));
		return;
	}

	if (op->mode == WRITE_PIPE)
	{
		pp = op->pipe_ptr;
		pp->nwriter--;                   // dec nwriter by 1

		if (--(op->refCount) == 0)
		{
			// last writer
			if (pp->nreader <= 0)
			{
				// no more readers
				pp->busy = 0;              // free pipe also 
				return;
			}
		}

		kwakeup(&(pp->data));
		return;
	}
}