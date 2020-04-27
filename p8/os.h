/*
 * os.h
 */

//
// some useful types
//
typedef void (* pfv_t)();
typedef int (* pfi_t)();
typedef unsigned int (* pfu_t)();

typedef union namenum {
	char	name[4];
	short	half[2];
	long	num;
	void	*ptr;
	float	fnum;
} namenum_t;

#define TRUE 1
#define FALSE 0
#define NULL 0
#define DEAD 0xdeadbeef

//
// time
//

typedef unsigned long long uint64_t;

extern uint64_t get_time ( void );
extern unsigned long now_usec ( void );
extern unsigned long now_hrs ( void );
extern void wait( unsigned long usecs );
extern void set_timer();
extern void init_time();

#define ONE_USEC	0x1
#define ONE_MSEC	(ONE_USEC * 1000)
#define ONE_SEC		(ONE_MSEC * 1000)

//
// TIME values - caveats:
// a. this assumes you are using the 900MHz-driven clock-timer as "now"
// b. it is ridiculously inaccurate (only approximately == 1 usec, 1 msec, 1 sec)
// c. but, hey, it works for now - one can always trim it up to be more accurate later
//    (e.g., set these values to *precisely* the number of clock-timer cycles that
//    bring you to the desired time ... I just didn't have the *time* (haha get it?)
//    to do all that work)
//
#define	USEC(u)		(u * ONE_USEC)
#define	MSEC(m)		(m * ONE_MSEC)
#define	SECS(s)		(s * ONE_SEC)		// note we can only go up to 2000 seconds

#define MAX_BLOCK_TIME	USEC(500)

//
// some system parameters
//
#define NUM_CORES	4
#define NUM_THREADS	16
#define MAX_SLEEP_INTERVAL	ONE_MSEC

//
// callout
//

#include "llist.h"

struct event {
	LL_PTRS;
	int timeout;
	int repeat_interval;
	int	max_repeats;
	int	arg;
	pfv_t go;
	namenum_t data;
};

extern void init_timeoutq( void );
extern int bring_timeoutq_current( void );
extern void insert_event(struct event *);
extern void create_timeoutq_event( int, int, int, pfv_t, namenum_t );
extern int handle_timeoutq_event( void );

#define MIN_TIMEOUT ONE_MSEC

//
// dev
//

#include "dev.h"

enum dev_types {
	DEV_INVALID,
	DEV_WORD,
	DEV_CBUF,
	DEV_BLOCK,
};


//
// I/O
//

struct dev {
	char devname[8];
	int	devtype;
	pfv_t init;
	pfi_t read;
	pfi_t write;
	pfi_t rcheck;
	pfi_t wcheck;
};

extern void init_io();

enum readwrite {
	IO_READ,
	IO_WRITE,
	MAX_TYPES,
};

struct io {
	LL_PTRS;
	int	rw;
	int	type;
	int	device;
	int	threadid;
	namenum_t data;
};

extern struct io * get_io_entry(int type);
extern void free_io_entry(struct io *iop);
extern void do_dev_word(struct event *ep);
extern void do_dev_cbuffer(struct event *ep);

//
// kversion
//

extern char kversion[];

//
// led
//

extern void init_led();
extern void led_on();
extern void led_off();
extern void blink_led_stall(unsigned int reps);

//
// log/errno
//

enum debug_levels {
	DEBUG_LOW,
	DEBUG_MED,
	DEBUG_HI,
	DEBUG_NONE
};

#define DEBUG_LEVEL	DEBUG_NONE

extern void * GETPC();
#if 0
#define debug(x,y,z);
#else
#define debug(x,y,z);	if (x >= DEBUG_LEVEL) { dont_call_directly(GETPC(),y,z); }
#endif

extern void	dont_call_directly( void *, char *, unsigned long);
extern void	log( char *, long);
extern void	panic( int, char *);
extern void init_log( void );
extern void die( long pc );
extern void idump( int * );

enum err_numbers {
	ERRNO_NONE,
	ERRNO_ASSERT,
	ERRNO_SPACE,
	ERRNO_INTERRUPT,
	ERRNO_SYSCALL,
	ERRNO_DEVHANDLER,
	/* add new ones above this line */
	ERRNO_MAX
};

#define NOVAL	0xbadf000d

#define	LOG_LR(); \
{ long lr; \
	asm volatile ("mov %[lr], lr\n" \
					: [lr] "=r" (lr) \
					: \
					: ); \
	log("lr = ", lr); \
}

#define	LOG_SP(); \
{ long sp; \
	asm volatile ("mov %[sp], sp\n" \
					: [sp] "=r" (sp) \
					: \
					: ); \
	log("sp = ", sp); \
}


//
// test utils
//

int is_printing(char c);
long atol_dec(char *s);
long atol_hex(char *s);
void strcpy4(char *to, char *from);
int strcpyN(char *to, char *from, int bufsiz);
int strlen(char *s);

//
// thread
//

extern void	scheduler(int);
extern void	wake_thread(long, long);
extern void	init_threads();
extern void create_thread(char *name, char *filename);
extern void dump_tcb(long);
extern void dump_processes();
extern long thread_activeid();

enum states {
	THREAD_INIT,
	THREAD_SLEEP,
	THREAD_RUN,
	MAX_THREAD_STATES,
};

enum tcb_regs {
	REG_r0,
	REG_r1,
	REG_r2,
	REG_r3,
	REG_r4,
	REG_r5,
	REG_r6,
	REG_r7,
	REG_r8,
	REG_r9,
	REG_r10,
	REG_r11,
	REG_r12,
	REG_sp,
	REG_lr,
	REG_pc,
	REG_spsr,
	REG_ttbr,
	REG_asid,
	// insert new ones here
	TCB_NUMREGS,
};

//
// UART
//

extern unsigned int uart_lcr ( void );
extern unsigned int uart_recv ( void );
extern int uart_recvcheck ( void );
extern int uart_sendcheck ( void );
extern void uart_send ( unsigned int );
extern void uart_flush ( void );
extern void uart_put2d ( unsigned long );
extern void uart_put3d ( unsigned long );
extern void uart_put8x ( unsigned int );
extern void uart_put12x ( unsigned int );
extern void uart_put32x ( unsigned int );
extern void uart_put64x ( unsigned long long );
extern void uart_puts ( char * );
extern void uart_putns ( char * , int );
extern void init_uart ( void );

//
// utils
//

extern void PUT32 ( unsigned long, unsigned long );
extern unsigned long GET32 ( unsigned long );
extern void * GETPC ( void );
extern void idle ( void );
extern void sync ( void );
extern void hang ( void );
extern void dummy ( void );
extern long cpu_id ( void );
extern void enable_irq ( void );
extern void enable_fiq ( void );
extern void disable_irq ( void );
extern void disable_fiq ( void );

//
// VM routines
//

extern void init_vm ( void );
long vm_allocate();
void vm_deallocate( long address );
void map( unsigned int ASID, unsigned int vaddr, unsigned int paddr, int io, int g );
void initialize_table( long asid );
unsigned long *vm_pagetable( long asid );
unsigned long vm_translate( long thread, long addr );

extern void invalidate_tlb();
extern void sync();
extern unsigned int readTTBCR();
extern void writeTTBCR(unsigned int);
extern unsigned int readTTBR0();
extern void writeTTBR0(unsigned int);
extern unsigned int readTTBR1();
extern void writeTTBR1(unsigned int);
extern unsigned int readCONTEXTIDR();
extern void writeCONTEXTIDR(unsigned int);
extern unsigned int readDACR();
extern void writeDACR(unsigned int);
extern unsigned int readSCTLR();
extern void writeSCTLR(unsigned int);
extern void setSMP();
