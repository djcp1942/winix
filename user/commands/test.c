/**
 * 
 *  testing route
 *
 * @author Bruce Tan
 * @email brucetansh@gmail.com
 * 
 * @author Paul Monigatti
 * @email paulmoni@waikato.ac.nz
 * 
 * @create date 2017-08-23 06:13:47
 * 
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/debug.h>
#include <ucontext.h>
#include <sys/times.h>
#include <sys/fcntl.h>

#define CMD_PROTOTYPE(name)    int name(int argc, char**argv)


// Maps strings to function pointers
struct cmd_internal {
    int (*handle)(int argc, char **argv);
    char *name;
};

CMD_PROTOTYPE(test_malloc);
CMD_PROTOTYPE(test_so);
CMD_PROTOTYPE(test_float);
CMD_PROTOTYPE(test_fork);
CMD_PROTOTYPE(test_thread);
CMD_PROTOTYPE(test_alarm);
CMD_PROTOTYPE(test_eintr);
CMD_PROTOTYPE(test_nohandler);
CMD_PROTOTYPE(test_vfork);
CMD_PROTOTYPE(test_deadlock);
CMD_PROTOTYPE(test_ipc);
CMD_PROTOTYPE(test_signal);
CMD_PROTOTYPE(test_while);

struct cmd_internal test_commands[] = {
    { test_malloc, "malloc"}, 
    { test_so, "stack"},
    { test_float, "float"},
    { test_thread, "thread"},
    { test_alarm, "alarm"},
    { test_eintr, "eintr"},
    { test_vfork, "vfork"},
    { test_deadlock, "deadlock"},
    { test_ipc, "ipc"},
    { test_signal, "signal"},
    { test_while, "while"},
    { test_nohandler, NULL},
    {0}
};

int main(int argc, char **argv){
    struct cmd_internal* handler;
    if(!argv[1])
        return test_nohandler(argc-1, argv+1);

    handler = test_commands;
    while(handler && handler->name != NULL && strcmp(argv[1], handler->name)) {
        handler++;
    }
    // Run it
    return handler->handle(argc-1, argv+1);
}

int test_nohandler(int argc, char** argv){
    int i;
    struct cmd_internal* handler;
    handler = test_commands;
    printf("Available Test Options\n");
    for( i = 0; i < sizeof(test_commands) / sizeof(struct cmd_internal) - 1; i++){
        printf(" * %s\n",handler->name);
        handler++;
    }
    return 0;
}

int test_while(int argc, char** argv){
    char buf[2];
    int ret;
    
    enable_syscall_tracing();
    buf[1] = '\0';
    while(1){
        ret = getchar();
        buf[0] = ret;
        printf("got %d %s\n", ret, buf);
    }
    
    return 0;
}

void usr_handler(int signum){
    printf("SIGUR1 start\n");
    raise(SIGUSR2);
    printf("SIGUR1 end\n");
}

void usr2_handler(int signum){
    printf("SIGUR2 start\n");
    raise(SIGINT);
    printf("SIGUR2 end\n");
}

void int_handler(int signum){
    printf("SIGINT start\n");
    raise(SIGTERM);
    printf("SIGINT end\n");
}

void term_handler(int signum){
    printf("SIGTERM start\n");
    printf("SIGTERM end\n");
}

int test_signal(int argc, char **argv){
    struct sigaction sa;
    int i;
    sigset_t set, prevset;

    // Check out what would happen after changing
    // the following sigemptyset to sigfillset
    // explain why

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    // setup signal handlers
    sa.sa_handler = usr_handler;
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = usr2_handler;
    sigaction(SIGUSR2, &sa, NULL);
    sa.sa_handler = int_handler;
    sigaction(SIGINT, &sa, NULL);
    sa.sa_handler = term_handler;
    sigaction(SIGTERM, &sa, NULL);
    
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, &prevset);

    // send SIGUSR1 to itself
    // since SIGUSR1 is currently blocked by sigprocmask
    // SIGUSR1 will be pended until sigsuspend
    raise(SIGUSR1);

    // check pending signals
    sigpending(&set);
    printf("pending sigs %x\n",set);

    if(sigismember(&set, SIGUSR1))
        printf("SIGUSR1 is pending\n");

    // unblock all pending signals
    printf("signal handler usr1 should be called after this\n");
    sigsuspend(&prevset);
    return 0;
}

int test_vfork(int argc, char **argv){

    pid_t pid = vfork();
    if(pid == 0){
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        exit(0);
    }
    printf("parent awaken\n");
    return 0;
}

int test_ipc(int argc, char **argv){
    pid_t pid;
    struct message m;
    if(pid = fork()){
        m.type = 100;
        winix_sendrec(pid,&m);
        printf("received %d from child\n",m.reply_res);
        wait(NULL);
    }else{
        winix_receive(&m);
        printf("received %d from parent\n",m.type);
        m.reply_res = 200;
        winix_send(getppid(), &m);
        exit(0);
    }
    return 0;
}

int test_deadlock(int argc, char **argv){
    pid_t pid;
    struct message m;
    if(pid = fork()){
        int n = 100000;
        while(n--);
        if(winix_send(pid,&m))
            perror("send to child");
        kill(pid,SIGKILL);
        wait(NULL);
    }else{
        int ret;
        ret = winix_send(getppid(),&m);
        while(1);
    }
    return 0;
}



void usr1h(int sig){
    printf("usr1h\n");
}

void usr2h(int sig){
    printf("usr2h\n");
}

int test_float(int argc, char **argv){
    int foo;

    signal(SIGFPE, SIG_IGN);
    foo = 1 / 0;
    return 0;
}

void stack_overflow(int a){
    stack_overflow(a);
}

int test_so(int argc, char **argv){
    if(!fork()){// child
        printf("Generating stack overflow ....\n");
        stack_overflow(1);
    }else{
        int status;
        wait(&status);
    }
    return 0;
}


int seconds = 1;
int cont;

void signal_handler(int signum){
    printf("\n%d seconds elapsed\n",seconds);
}

void alarm_handler(int signum){
    printf("\n%d seconds elapsed\n",seconds);
    cont = 0;
}

/**

    The difference between test_alarm and test_eintr is that
    test_signal will return to the main shell loop after issuing the
    alarm syscall, thus it will be blocked until a new character arrives.
    After one second, when SIGALRM is scheduled to be delivered, and when
    shell process is still blocked, the kernel will temporarily schedule 
    the process, and call the signal handler. Upon exiting signal handler,
    the shell process will be blocked again.

    On the other side, test_alarm() simply calls a while loop to wait for
    the signal to be delivered, this serves as a simple test case for signal
    but avoids to test on the edge cases where signal is delivered while 
    a process is blocked by a system call.

**/
int test_alarm(int argc, char **argv){
    int i;
    seconds = (argc > 1) ? atoi(argv[1]) : 1;
    if(seconds < 0)
        seconds = 0;
    signal(SIGALRM,alarm_handler);
    alarm(seconds);
    cont = 1;
    i = 10000;
    while(cont){
        putchar('!');
        while(i--);
        i = 10000;
    }
        
    return 0;
}

int test_eintr(int argc, char **argv){
    int i;
    pid_t pid;
    pid_t fr;
    
    seconds = (argc > 1) ? atoi(argv[1]) : 1;
    signal(SIGALRM,signal_handler);
    alarm(seconds);
    return 0;
}

ucontext_t mcontext;
#define THREAD_STACK_SIZE    56

void func(int arg) {
      printf("Hello World! I'm thread %d\n",arg);
}

int test_thread(int argc, char **argv){
    int i,j,num = 2;
    int count = 1;
    void **thread_stack_op;
    ucontext_t *threads; 
    ucontext_t *cthread;

    if(argc > 1)
        num = atoi(argv[1]);
    // ucontext represents the context for each thread
    threads = malloc(sizeof(ucontext_t) * num);
    if(threads == NULL)
        goto err;
    
    // thread_stack_op saves the original pointer returned by malloc
    // so later we can use it to free the malloced memory
    thread_stack_op = malloc(sizeof(int) * num);
    if(thread_stack_op == NULL)
        goto err_free_threads;
        
    cthread = threads;
    // Allocate stack for each thread
    for( i = 0; i < num; i++){
        if ((thread_stack_op[i] =  malloc(THREAD_STACK_SIZE)) != NULL) {
            cthread->uc_stack.ss_sp = thread_stack_op[i];
            cthread->uc_stack.ss_size = THREAD_STACK_SIZE;
            cthread->uc_link = &mcontext;
            makecontext(cthread,func,1,count++);
            cthread++;

            if(i%50 == 0)
                putchar('!');
        }else{
            goto err_free_all;
        }
    }
    putchar('\n');
    
    cthread = threads;
    // scheduling the threads
    // note that we are using user thread library, 
    // so we have to manually schedule all the threads.
    // Currently the scheduling algorithm is just simple a round robin
    for( i = 0; i < num; i++){
        swapcontext(&mcontext,cthread++);
    }
    
    err_free_all:
        for( j = 0; j < i; j++){
            free(thread_stack_op[j]);
        }
        free(thread_stack_op);
    err_free_threads:
        free(threads);
    err:
        if(errno == ENOMEM)
            perror("malloc failed");
    return 0;
}


int test_malloc(int argc, char **argv){
    
    void *p0 = malloc(512);
    void *p1 = malloc(512);
    void *p2 = malloc(1024);
    void *p3 = malloc(512);
    void *p4 = malloc(1024);
    void *p5 = malloc(2048);
    void *p6 = malloc(512);
    void *p7 = malloc(1024);
    void *p8 = malloc(512);
    void *p9 = malloc(1024);
    free(p5);
    free(p6);
    free(p2);
    free(p8);
    print_heap();
  
    return 0;
}
