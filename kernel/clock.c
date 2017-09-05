/**
 * 
 * Winix clock exception handler
 *
 * @author Bruce Tan
 * @email brucetansh@gmail.com
 * 
 * @author Paul Monigatti
 * @email paulmoni@waikato.ac.nz
 * 
 * @create date 2016-09-19
 * 
*/

#include "winix.h"

//System uptime, stored as number of timer interrupts since boot
PUBLIC int system_uptime = 0;

//Global variable for the next timeout event jiddles
PUBLIC clock_t next_timeout = 0;

void deliver_alarm(int proc_nr, clock_t time){
    cause_sig(get_proc(proc_nr),SIGALRM);
}

void handle_timer(struct timer *timer){

    if(timer != NULL && timer->time_out == system_uptime)
        timer->handler(timer->proc_nr,timer->time_out);
    else
        kprintf("Inconsis alarm %d %d from %d\n",system_uptime,next_timeout,timer->proc_nr);
}

/**
 * Timer (IRQ2)
 *
 * Side Effects:
 *   system_uptime is incremented
 *   if there is an immediate timer, relevant handler is called
 *   scheduler is called (i.e. this handler does not return)
 **/
void clock_handler(){
    RexTimer->Iack = 0;

    //Increment uptime, and check if there is any alarm
    system_uptime++;
    if(next_timeout == system_uptime)
        handle_timer(dequeue_alarm());

    sched();
}
