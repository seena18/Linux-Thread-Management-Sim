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

int counter=0;
thread lwps = NULL;
thread lcurr = NULL;

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
    unsigned long *stack =
    mmap(
        NULL,
        lim.rlim_cur,
        PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK,
        -1,
        0);
    thread c = (thread)malloc(sizeof(context));
    if(stack==MAP_FAILED){
        // fprintf(stderr,"Stack allocation failed");
        return NO_THREAD;
    }
    if(c==MAP_FAILED){
        // fprintf(stderr,"Context allocation failed");
        return NO_THREAD;
    }
    c->stack=stack;
    c->stacksize=lim.rlim_cur;
    c->tid=count++;
    stack = (unsigned long *)
    ((unsigned long)c->stack + (unsigned long)c->stacksize);
    // stack[0] = (unsigned long)lwp_exit; 
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
    counter++;
    if(lwps==NULL){
        lwps=c;
        lcurr=lwps;
    }
    else{
        lcurr->lib_one=c;
    }
    

    return c->tid;
}
void lwp_start(){
    thread new = (thread)malloc(sizeof(context));
    new->tid=count++;
    new->status=LWP_LIVE;
    schedule->admit(new);
    counter++;
    if(lwps==NULL){
            lwps=new;
            lcurr=lwps;
        }
        else{
            lcurr->lib_one=new;
        }

    curr=new;
    lwp_yield();
}

void lwp_yield(){
    // FILE *fp = fopen("test.txt", "a");
    
    thread temp;
    temp = curr;
    
    curr=schedule->next();
    
    if(curr==NULL){
        exit(temp->status);
    }
    else{
        // fprintf(fp,"Yield %d to %d\n\n",temp->tid,curr->tid);
        // fclose(fp);
        swap_rfiles(&temp->state, &curr->state);
    }
    
    
}

void lwp_exit(int exitval){
    // FILE *fp = fopen("test.txt", "a");
    if(curr!=NULL){
        schedule->remove(curr);
        counter--;
        curr->status=MKTERMSTAT(LWP_TERM,exitval);
        if(zombies==NULL){
            zombies=curr;
            zcurr=zombies;
        }
        else{
            zcurr->exited=curr;
        }
        
        if(waits!=NULL){
            schedule->admit(waits);
            counter++;
            waits=waits->exited;
        }
        
        // fprintf(fp,"Exit: %d\n",curr->tid);
        // fclose(fp);
        lwp_yield();
    }
    
}

tid_t lwp_wait(int *status){
    // FILE *fp = fopen("test.txt", "a");
    // fprintf(fp,"Wait: %d\n", curr->tid);
    // fclose(fp);
    if(zombies==NULL){
        schedule->remove(curr);
        counter--;
        if(waits==NULL){
            waits=curr;
            wcurr=waits;
        }
        else{
            wcurr->exited=curr;
        }
        lwp_yield();
    }
    
    thread temp=zombies;
    if (status!=NULL){
        *status=temp->status;
    }
    zombies=zombies->exited;
    tid_t t=temp->tid;
    munmap(temp->stack,sizeof(temp->stacksize));
    free(temp);
    if(t){
        return t;
    }
    else{
        return NO_THREAD;
    }
    
    
    
}
tid_t lwp_gettid(void) {
    if (!curr) {
        return NO_THREAD;
    }
    return curr->tid;
}

thread tid2thread(tid_t tid) {
    thread temp = lwps;
    while(temp!=NULL && temp->tid != tid){
        temp = temp->lib_one;
    }
    if(temp && temp->tid == tid){
        return temp;
    }
    else{
        return NULL;
    }
    
}
void lwp_set_scheduler(scheduler sched){
    if (sched->init) { /* check if init is NULL */
        sched->init();
    }
    thread tmp;
    if(sched==NULL){
        scheduler s = &rr_publish;
        while((tmp=schedule->next())!=NULL){
            s->admit(tmp);
            schedule->remove(tmp);
        }
        if (schedule->shutdown) { 
        schedule->shutdown();
    }
        schedule=s;
    }
    else{
        while((tmp=schedule->next())!=NULL){
            sched->admit(tmp);
            schedule->remove(tmp);
        }
        if (schedule->shutdown) { 
        schedule->shutdown();
    }
        schedule=sched;
    }
    
}
scheduler lwp_get_scheduler(void) {
    return schedule;
}