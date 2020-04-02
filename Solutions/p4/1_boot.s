.globl _start
_start:
    b res_handler		// RESET                 runs in SVC mode
    b hang				// UNDEF INSTR           runs in UND mode
    b svc_handler		// SWI (TRAP)            runs in SVC mode
    b hang				// PREFETCH ABORT        runs in ABT mode
    b hang				// DATA ABORT            runs in ABT mode
    b hang				// HYP MODE              runs in HYP mode
    b irq_nop		// IRQ INTERRUPT         runs in IRQ mode
	// FIQ can simply be written here, since this is the end of the table:
    b hang		// FIQ INTERRUPT         runs in FIQ mode

.equ    FIQSTACK0 , 0xf0000
.equ    IRQSTACK0 , 0xe0000
.equ    SVCSTACK0 , 0xd0000
.equ    KSTACK0   , 0xc0000
.equ    USTACK0   , 0x80000
.equ    USTACK1   , 0x81000

.equ	USR_mode,	0x10
.equ	FIQ_mode,	0x11
.equ	IRQ_mode,	0x12
.equ	SVC_mode,	0x13
.equ	HYP_mode,	0x1A
.equ	SYS_mode,	0x1F
.equ	No_Int,		0xC0


res_handler:
    mrc p15, 0, r0, c1, c0, 0 @ Read System Control Register
@   orr r0, r0, #(1<<2)       @ dcache enable
    orr r0, r0, #(1<<12)      @ icache enable
    and r0, r0, #0xFFFFDFFF   @ turn on vector table at 0x0000000 (bit 12)
    mcr p15, 0, r0, c1, c0, 0 @ Write System Control Register

// check core ID
	mrc     p15, 0, r0, c0, c0, 5
	ubfx    r0, r0, #0, #2
	cmp     r0, #0					// is it core 0?
	beq     core0

	// it is not core0, so do things that are appropriate for SVC level as opposed to HYP
	// like set up separate stacks for each core, etc.

	beq     hang

hang: 	wfi
		b hang

core0:
	// set up timer
//	mov		r0, #1						@ turn on timer via the CNTP_CTL reg
//	mcr		p15, 0, r0, c14, c2, 1		@ turn on timer via the CNTP_CTL reg

//	ldr		r0, =19200000				@ counter frequency in Hz
//	mcr		p15, 0, r0, c14, c0, 0    	@ set the counter frequency
//	ldr		r0, =40200000				@ set the initial timer value to count down from
//	mcr		p15, 0, r0, c14, c2, 0		@ set the initial timer value to count down from

//	mov		r0, #0x0F					@ timer generates IRQ interrupts
//	ldr		r1, =0x40000040				@ set up timer interrupts (base address)
//	str		r0, [r1, #0]				@ set up timer interrupts to core 0
//	str		r0, [r1, #4]				@ set up timer interrupts to core 1
//	str		r0, [r1, #8]				@ set up timer interrupts to core 2
//	str		r0, [r1, #12]				@ set up timer interrupts to core 3

	// Initialize SPSR in all modes.
	MOV    R0, #0
	MSR    SPSR, R0
	MSR    SPSR_svc, R0
	MSR    SPSR_und, R0
	MSR    SPSR_hyp, R0
	MSR    SPSR_abt, R0
	MSR    SPSR_irq, R0
	MSR    SPSR_fiq, R0

	// Initialize ELR_hyp (necessary?)
	MOV		R0, #0
	MSR		ELR_hyp, R0

	// set up stacks (only need SVC and K at this point, but what the heck)
	cps		#IRQ_mode
	mov		sp, # IRQSTACK0

	cps		#FIQ_mode
	mov		sp, # FIQSTACK0

	cps		#SVC_mode
	mov		sp, # SVCSTACK0

	cps		#SYS_mode
	mov		sp, # KSTACK0
	bl		init_kernel

	// set up user stack and jump to shell
	cps		#USR_mode
	mov		sp, # USTACK0
	bl		run_shell
//	bl		do_blinker
	b hang

.equ    T_bit,   0x20                @ Thumb bit (5) of CPSR/SPSR.
old_svc_handler:
    STMFD   sp!, {r0-r3, r12, lr}  @ Store registers
    MOV     r1, sp                 @ Set pointer to parameters
    MRS     r0, spsr               @ Get spsr
    STMFD   sp!, {r0, r3}          @ Store spsr onto stack and another
                                   @ register to maintain 8-byte-aligned stack
    TST     r0, #T_bit             @ Occurred in Thumb state?
    LDRNEH  r0, [lr,#-2]           @ Yes: Load halfword and...
    BICNE   r0, r0, #0xFF00        @ ...extract comment field
    LDREQ   r0, [lr,#-4]           @ No: Load word and...
    BICEQ   r0, r0, #0xFF000000    @ ...extract comment field

    @ r0 now contains SVC number
    @ r1 now contains pointer to stacked registers

    BL      trap_handler            @ Call main part of handler

    LDMFD   sp!, {r0, r3}          @ Get spsr from stack
    MSR     SPSR_cxsf, r0          @ Restore spsr
    LDMFD   sp!, {r0-r3, r12, pc}^ @ Restore registers and return


// courtesy of Prof Vince Weaver, U Maine
svc_handler:

    stmia   sp,{r0-lr}^     @ Save all user registers r0-lr
						@ (the ^ means user registers)

    str lr,[sp,#60]     @ store saved PC on stack

    mrs ip, SPSR        @ load SPSR (assume ip not a swi arg)
    str ip,[sp,#64]     @ store on stack

    sub sp,sp,#80

    @ Call the C version of the handler

    bl  trap_handler

    @ Put our return value of r0 on the stack so it is
    @ restored with the rest of the saved registers

    add sp,sp,#80

    str r0,[sp]

    ldr r0,[sp,#64]     @ pop saved CPSR
    msr SPSR_cxsf, r0       @ move it into place

    ldr lr,[sp,#60]     @ restore address to return to

    @ Restore saved values.  The ^ means to restore the userspace registers
    ldmia   sp, {r0-lr}^
    movs    pc, lr



            .word 0
            .word 0
shellsave:  .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0

            .word 0
            .word 0
blinksave:  .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0
            .word 0

save_r13_irq: .word 0
save_r14_irq: .word 0



badval: .word   0xdeadbeef


save_r0_usr: .word 0

threadstate:    .word 0
blinkisrunning: .word 0
//0 for shell, 1 for blinking, current state
//is running: 0 for no, 1 for yes

@
@ based on code from rpi discussion boards
@
irq_nop:
	// save the registers
	str r0, save_r0_usr
	ldr r0, threadstate
	cmp r0, #0                            @ is the shell running or is blinker running?
    bne blinker2shell

    mov r0, #1                              @ set new threadstate to blinker and save
    str r0, threadstate



shell2blinker:
    mov r0, #1
    str r0, threadstate

    ldr r0, save_r0_usr

    str     r13, save_r13_irq           @ save the IRQ stack pointer
    ldr     r13, =shellsave            @ load the IRQ stack pointer with address of TCB
    add     r13, r13, #56               @ jump to middle of TCB for store up and store down
    stmia   sp, {sp, lr}^               @ store the USR stack pointer & link register, upwards
    push    {r0-r12, lr}                @ store USR regs 0-12 and IRQ link register (r14), downwards

    // clear timer interrupt (we get here from timer)
    bl  clear_timer_interrupt
	// reset the timer
	bl		set_timer

	ldr r0, blinkisrunning
    cmp r0, #0
    beq start_blinker                    @ blinker is not running, so start it, otherwise continue restoring blinker rf

	// restore the registers
    ldr     r13, =blinksave            @ load the IRQ stack pointer with address of TCB
    pop     {r0-r12, lr}                @ load USR regs 0-12 and IRQ link register (r14), upwards
    ldmia   sp, {sp, lr}^               @ load the USR stack pointer & link register, upwards
    nop                                 @ evidently it's a god idea to put a NOP after a LDMIA
    ldr     r13, save_r13_irq           @ restore the IRQ stack pointer from way above
    subs    pc, lr, #4                  @ return from exception


blinker2shell:
    mov r0, #0
    str r0, threadstate

    ldr r0, save_r0_usr

    str     r13, save_r13_irq           @ save the IRQ stack pointer
    ldr     r13, =blinksave            @ load the IRQ stack pointer with address of TCB
    add     r13, r13, #56               @ jump to middle of TCB for store up and store down
    stmia   sp, {sp, lr}^               @ store the USR stack pointer & link register, upwards
    push    {r0-r12, lr}                @ store USR regs 0-12 and IRQ link register (r14), downwards

    // clear timer interrupt (we get here from timer)
    bl  clear_timer_interrupt
	// reset the timer
	bl		set_timer

	// restore the registers
    ldr     r13, =shellsave            @ load the IRQ stack pointer with address of TCB
    pop     {r0-r12, lr}                @ load USR regs 0-12 and IRQ link register (r14), upwards
    ldmia   sp, {sp, lr}^               @ load the USR stack pointer & link register, upwards
    nop                                 @ evidently it's a god idea to put a NOP after a LDMIA
    ldr     r13, save_r13_irq           @ restore the IRQ stack pointer from way above
    subs    pc, lr, #4                  @ return from exception

start_blinker:
    mov r0, #1
    str r0, blinkisrunning
    cps #USR_mode
    mov		sp, # USTACK1
    bl do_blinker
    b hang

