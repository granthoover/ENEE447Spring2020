# Project 2 - Concepts to Know

**Problem:** We want programs (or simple routines/functions) to run at specific times and, sometimes, at regular intervals.

Reasons for wanting this:
- Checking for I/O at regular intervals
- Possibly calling kernel routines for program scheduling (context switching)
- Watchdog timers – ensure programs are not taking too long
- Allow a function to sleep for set intervals (e.g. audio sampling)

Why doesn’t our previous (Project 1) *while()* loop work?
- What happens if a program halts (blocking)?
- I/O takes *a lot* of time. Nothing can happen while we wait for it to be ready.

Possible solutions:
1. Spawn a new thread for each sequence. New problem: There are a limited number of cores so you can still get blocking. It’s also very difficult to share a timer between threads as they are spawning and dying. We need a central timekeeper.
2. Use a dynamic list of tasks to be run and times at which to run them.
   This is called a “callout table” in UNIX or a “timeout queue” generally.

## Timeout Queue Implementation

We can use a linked list to keep track of these tasks.

What goes into each node?
- Time to run/execute
- How many repeats?
- How long (time) between repeats
- Function pointer
- Function data/arguments
- Pointer to the next node

Example:

Tasks are added to the timeout queue in the following order:
1. Run X(5) at time = 3
2. Run Y(A) at time = 10
3. Run Z(0.275) at time = 4

The timeout queue looks like the following after each addition:

1. &#43;--------&#43;  
&nbsp;&#124; time: 3 &#124;  
&nbsp;&#124; func: X &#124;  
&nbsp;&#124; data: 5 &#124;  
&#43;--------&#43;

2. &#43;--------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;--------&#43;       
&nbsp;&#124; time: 3 &#124;----->&#124; time: 7 &#124;  
&nbsp;&#124; func: X &#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; func: Y &#124;  
&nbsp;&#124; data: 5 &#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; data: A &#124;  
&#43;--------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;--------&#43;

2. &#43;--------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;-------------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;--------&#43;       
&nbsp;&#124; time: 3 &#124;----->&#124; time: 1 &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124;----->&#124; time: 6 &#124;  
&nbsp;&#124; func: X &#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; func: Z &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; func: Y &#124;  
&nbsp;&#124; data: 5 &#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; data: 0.275 &nbsp;&#124;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#124; data: A &#124;  
&#43;--------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;-------------&#43;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&#43;--------&#43;

Why insert Z before Y in the list?

This way, we know that everything past Z in the list will execute after Z. If we keep *relative* times between successive nodes, we don’t have to traverse the entire list when checking if a node is ready to execute. If the HEAD is not ready (its time value doesn’t equal 0), none of the nodes are ready.

Another reason for using relative times: When we check the time, we only need to update the time value of the HEAD to reflect the amount of time *until* it should run.


## Functions

`int bring_timeoutq_current()`
   
* Updates the time value of the HEAD node based on the time that has passed since the last check (we keep this previous time in a global variable called then_usec), updates then_usec, and returns the amount of time left until the HEAD should execute.  

`void create_timeoutq_event(int timeout, int repeat, pfv_t function, namenum_t data)`
  
* Creates a new node and inserts it into the timeout queue.  

`int handle_timeout_event()`
  
* Checks if HEAD time value is less than some minimum time (ready to be executed), updates the time value of the second node if there is one, removes the HEAD from the list, runs its function, and puts it back into the queue if the task needs repeated (otherwise add the node back to the “freelist”). Returns 1 if an event is handled.  

Kernel *while()* loop now checks the amount of time until the next event should run, waits for that time, and then handles the next event.

## Important Notes

Function pointers take event pointers as arguments.

**Remember to check for empty lists and null pointers!**
