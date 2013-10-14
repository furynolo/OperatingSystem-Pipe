
/*************************************************************************
  usp  1   2   3   4   5   6   7   8   9  10   11   12    13  14  15  16
----------------------------------------------------------------------------
 |uds|ues|udi|usi|ubp|udx|ucx|ubx|uax|upc|ucs|uflag|retPC| a | b | c | d |
----------------------------------------------------------------------------
***************************************************************************/

#define PA 13
#define PB 14
#define PC 15
#define PD 16
#define AX  8

//int color;

/****************** syscall handler in C ***************************/
int kcinth()
{
	u16    segment, offset;
	int    a,b,c,d, r;
	segment = running->uss;
	offset = running->usp;

	/** get syscall parameters from ustack **/
	a = get_word(segment, offset + 2*PA);
	b = get_word(segment, offset + 2*PB);
	c = get_word(segment, offset + 2*PC);
	d = get_word(segment, offset + 2*PD);

	switch(a)
	{
		case 0 : r = running->pid;		break;
		case 1 : r = do_ps();			break;
		case 2 : r = chname(b);			break;
		case 3 : r = kmode();			break;
		case 4 : r = tswitch();			break;
		case 5 : r = do_wait(b);		break;
		case 6 : r = do_exit(b);		break;
		case 7 : r = do_fork();			break;
		case 8 : r = do_exec(b);		break;

		case 30 : r = kpipe(b);			break;
		case 31 : r = read_pipe(b,c,d);	break;
		case 32 : r = write_pipe(b,c,d);break;
		case 33 : r = close_pipe(b);	break;
		case 34 : r = pfd();			break;

		case 90: r =  getc();			break;
		case 91: //color=running->pid+11;
				r =  putc(b);           break;
		case 99: do_exit(b);			break;
		default: printf("invalid syscall # : %d\n", a);
	}

	// Put the return value into the UMODE segment @ (running->usp + 2*AX)
	put_word(r, segment, offset + 2*AX);
}