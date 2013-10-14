/*******************************************************
 *                 @uio.c file                         *
 *******************************************************/

//YOUR printf() for printing %c %s %d %x %l, etc. BASED ON putc()

/*typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned long   u32;*/

char *ctable = "0123456789ABCDEF";


int rpu(u16 x, u16 base)
{
	char c;
	if (x)
	{
		c = ctable[x % base]; 
		rpu(x / base, base);
		putc(c);
	}

	return 0;
}

int rpl(u32 x, u16 base)
{
	char c;
	if (x)
	{
		c = ctable[x % base]; 
		rpl(x / base, base);
		putc(c);
	}

	return 0;
}

int printu(u16 x)
{   
	if (x == 0) 
		putc(ctable[0]);
	else // x > 0
	{
		rpu(x, 10);
	}
//	putc(' ');

	return 0;
}

int printd(int x)
{   
	if (x == 0) 
		putc(ctable[0]);
	else if (x < 0)
	{
		putc('-');
		rpu((-1*x), 10);
	}
	else // x > 0
	{
		rpu(x, 10);
	}
//	putc(' ');

	return 0;
}

int prints(char *x) // prints string
{
	while(*x)
	{
		putc(*x++);
	}

	return 0;
}

int printx(u16 x) // prints hexidecimal
{
	if (x==0) 
		putc(ctable[0]);
	else if (x < 0)
	{
		putc('-');
		rpu(-1*x, 16);
	}
	else // x > 0
	{
		rpu(x, 16);
	}
//	putc(' ');

	return 0;
}

int printl(u32 x) // prints long/decimal
{
	if (x == 0)
		putc(ctable[0]);
	else if (x < 0)
	{
		putc('-');
		rpl(-1*x, 10);
	}
	else // x > 0
	{
		rpl(x, 10);
	}
//	putc(' ');

	return 0;
}

// print with formatting
int printf (char *fmt, ...)
{
	// NOTE: **(%c=char  %s=string  %d=int  %l=long  %x=hexidecimal)**
	char *cp = fmt;              // let cp point at the fmt string
	u16  *ip = (int *)&fmt + 1;  // ip points at first item to be printed on stack (e.g. 'a')
	u32  *up;                    // for getting LONG parameter off stack

	while (*cp) //loop until end of string
	{
		if ((*cp != '%') && (*cp != '\\'))	// just print anything that doesnt start with a '%' or '\'
		{
			putc(*cp); // prints the character
			if (*cp == '\n')
				putc('\r'); // need to print out '\r' when '\n' is found
			cp++;
			continue;
		}

		if (*cp == '%') // character is a '%'
		{
			cp++;
			switch(*cp)
			{
				case 'c':
					putc(*ip);
					break;
				case 's':
					prints(*ip);
					break;
				case 'd':
					printd(*ip);
					break;
				case 'u':
					printu(*ip);
					break;
				case 'l':
					up = (u32 *)ip;
					printl(*up);
					ip++; // increment ip by an extra two bytes for long
					break;
				case 'x':
					printx(*ip);
					break;
			}
			ip++; // increment ip by two bytes
		}
		cp++;
	}
	
	return 0;
}

int gets(char *buf)
{
	u16 i = 0;
	char c = ' ';
	while ((c != '\r') && (i < 64))
	{
		c = getc();
		buf[i] = c;
		i++;
	}
	buf[i] = '\0';

	return 0;
}
