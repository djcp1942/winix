#include <lib.h>



int getchar(){
    struct message m;
    return _syscall(SYSCALL_GETC,&m);
}
