#include "lwp.h"
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <stdlib.h>

void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next(void);
void lwp_exit(int exitval);
tid_t lwp_gettid(void);

static struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next};
scheduler schedule = &rr_publish;

thread lwps = NULL;
thread curr = NULL;
thread zombies =NULL;
thread zcurr=NULL;
thread waits =NULL;
thread wcurr=NULL;
tid_t count = 1;



tid_t lwp_create(lwpfun function,void *argument){
    long int pagesize=sysconf(_SC_PAGE_SIZE);
    struct rlimit lim;
    int limresult = getrlimit(RLIMIT_STACK, &lim);
    if(limresult==-1){
        lim.rlim_cur=8000000;
    }
    if(lim.rlim_cur%pagesize!=0){
        lim.rlim_cur=lim.rlim_cur+(pagesize-lim.rlim_cur%pagesize);
    }
    unsigned long *stack = mmap(NULL,lim.rlim_cur,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,-1,0);
    thread c = (thread)malloc(sizeof(context));
    if(stack==MAP_FAILED){
        fprintf(stderr,"Stack allocation failed");
        return NO_THREAD;
    }
    if(c==MAP_FAILED){
        fprintf(stderr,"Context allocation failed");
        return NO_THREAD;
    }
    c->stack=stack;
    c->stacksize=lim.rlim_cur;
    c->tid=count++;

    stack = (unsigned long *)((unsigned long)c->stack + (unsigned long)c->stacksize);
    stack[-1] = (unsigned long)function; 
    stack[-2] = (unsigned long)stack;

    c->state.fxsave=FPU_INIT;
    c->state.rdi=(unsigned long)argument;
    c->state.rbp=(unsigned long)(stack-2);
    c->status=LWP_LIVE;
    

    c->lib_one = NULL;
    c->lib_two = NULL;
    c->sched_two = NULL;
    c->sched_one = NULL;

    schedule->admit(c);
    if (lwps) {
        lwps->lib_two = c;
        c->lib_one = lwps;
        lwps = c;
    }
    else lwps = c;
    

    return c->tid;
}
void lwp_start(){
    thread new = (thread)malloc(sizeof(context));
    new->tid=count++;
    new->status=LWP_LIVE;
    schedule->admit(new);
    curr=new;
    lwp_yield();
}

void lwp_yield(){
    thread temp;
    temp = curr;
    curr=schedule->next();
    if(curr==NULL){
        exit(temp->status);
    }
    else{
        swap_rfiles(&temp->state, &curr->state);
    }
}

void lwp_exit(int exitval){
    if(curr!=NULL){
        schedule->remove(curr);
        curr->status=MKTERMSTAT(LWP_TERM,exitval);
        if(zombies==NULL){
            zombies=curr;
            zcurr=zombies;
        }
        else{
            zcurr->exited=curr;
        }
        lwp_yield();
    }
    
}

tid_t lwp_wait(int *status){
    
    while(zombies==NULL){
        lwp_yield();
    }
    thread temp=zombies;
    *status=temp->status;
    zombies=zombies->exited;
    tid_t t=temp->tid;
    munmap(temp->stack,sizeof(temp->stacksize));
    free(temp);
    return t;
}
tid_t lwp_gettid(void) {
    if (!curr) {
        return NO_THREAD;
    }
    return curr->tid;
}