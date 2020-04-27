
#include "../syscall.h"
#include "../dev.h"
#include "u_text.h"

typedef union namenum {
	char	name[4];
	short	half[2];
	long	num;
	void	*ptr;
	float	fnum;
} namenum_t;

#define LSH(x,y)	(((unsigned long)(x & 0xff)) << y)
#define	SYSCALL_NUM(a,b,c,d)	(unsigned long)(LSH(a,0) | LSH(b,8) | LSH(c,16) | LSH(d,24))

#define	CMD_LOG		SYSCALL_NUM('L','O','G','\0')
#define	CMD_TIME	SYSCALL_NUM('T','I','M','E')
#define	CMD_LED		SYSCALL_NUM('L','E','D','\0')
#define	CMD_EXIT	SYSCALL_NUM('E','X','I','T')
#define	CMD_DUMP	SYSCALL_NUM('D','U','M','P')
#define	CMD_RUN		SYSCALL_NUM('R','U','N','\0')
#define	CMD_PS		SYSCALL_NUM('P','S','\0','\0')


char input_buffer[1024];

void
notmain()
{
	register int i=0,ch,led=0;
	char *cp;
	char *next;
	namenum_t cmd[5];
	char name[5];
	uint64_t time_usec;

	puts("Running the eggshell on core "); put4x(syscall_get_cpuid()); puts(".\n");

	puts("Available commands:\n");
	puts(" RUN  = "); put32x(CMD_RUN); puts("\n");
	puts(" PS   = "); put32x(CMD_PS); puts("\n");
	puts(" TIME = "); put32x(CMD_TIME); puts("\n");
	puts(" LED  = "); put32x(CMD_LED); puts("\n");
	puts(" LOG  = "); put32x(CMD_LOG); puts("\n");
	puts(" EXIT = "); put32x(CMD_EXIT); puts("\n");
	puts(" DUMP = "); put32x(CMD_DUMP); puts("\n");

	while (1) {
		puts("\nPlease enter a command.\n c"); put4x(syscall_get_cpuid()); puts("> ");

		//
		// reads in characters until newline
		//
		for (i=0; (ch=syscall_read_word(DEV_CONSOLE)) >= 0x20; i++) {
			/* Convert to uppercase */
			if ((ch>='a') && (ch<='z')) ch-=32;

			syscall_write_word(DEV_CONSOLE, ch);

			input_buffer[i] = (char)(ch & 0xff);
			input_buffer[i+1] = '\0';
		}
		puts("\n");

		for (i=0; i<5; i++) cmd[i].num = 0;

		//
		// parses the line of characters
		//
		for (i=0, cp = input_buffer, next = tokenize(input_buffer, ' ');
			i < 4 && cp != NULL && is_printing(*cp);
			next = tokenize((cp[0] == '"' ? cp+1 : cp), (cp[0] == '"' ? '"' : ' ')), i++) {

			if (cp[0] == '0' && cp[1] == 'X') {
				// assume it's a hexadecimal value
				cmd[i].num = atol_hex(cp);
			} else if (cp[0] == '"') {
				cmd[i].ptr = cp+1;
			} else if (cp[0] >= '0' && cp[0] <= '9') {
				// assume it's a decimal value
				cmd[i].num = atol_dec(cp);
			} else {
				cmd[i].num = 0x00000000;
				strcpy4(cmd[i].name, cp);
			}
			cp = next;
		}

		//
		// handles the command
		//
		switch(cmd[0].num) {
			case CMD_PS:
				puts("CMD_PS\n");
				syscall_list_processes();
				break;
			case CMD_RUN:
				puts("CMD_RUN ["); puts(cmd[1].name); puts(", "); put32x(cmd[2].num); puts("]\n");
				syscall_start_thread(cmd[1].num, cmd[2].num);
				break;
			case CMD_DUMP:
				if (cmd[3].num == 0) cmd[3].num = 1;
				puts("CMD_DUMP ["); put32x(cmd[1].num); puts(", "); put32x(cmd[2].num);
					puts(", by "); put32x(cmd[3].num); puts("]\n");
				syscall_dump_range(cmd[1].num, cmd[2].num, cmd[3].num);
				break;
			case CMD_LOG:
				puts("CMD_LOG [");
				if (cmd[1].ptr) puts(cmd[1].ptr);
				puts("]\n");
				syscall_write_cbuffer(DEV_KLOG, cmd[1].ptr, CBUFSIZE);
				break;
			case CMD_TIME:
				puts("CMD_TIME = [");
				syscall_read_cbuffer(DEV_CLOCK, &time_usec, 8);
				put32x((long)((time_usec >> 32) & 0xffffffff));
				puts(" ");
				put32x((long)(time_usec & 0xffffffff));
				puts("]\n");
				break;
			case CMD_LED:
				if (led) {
					puts("CMD_LED - off\n");
					syscall_write_word(DEV_LED, 0);
				} else {
					puts("CMD_LED - on\n");
					syscall_write_word(DEV_LED, 1);
				}
				led = ~led;
				break;
			case CMD_EXIT:
				puts("CMD_EXIT exiting ...\n");
				return;
				break;
			default:
				strcpy4(name, cmd[0].name);
				name[4] = '\0';
				puts("unknown command: ");
				puts(name);
				puts(" - ");
				put32x(cmd[0].num);
				puts("\n");
				break;
		}
    }

	return;
}