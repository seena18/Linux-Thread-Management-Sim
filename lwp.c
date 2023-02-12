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
void lwp_wrap();
tid_t lwp_gettid(void);
void printlwps();
void removeThread();
void printz();

//setup scheduler
static struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next};
scheduler schedule = &rr_publish;

//setup linked lists and current vars for all 
//threads, zombie threads, and threads that are currently waiting

thread lwps = NULL;
thread lcurr = NULL;
thread curr = NULL;
thread zombies =NULL;
thread zcurr=NULL;
thread waits =NULL;
thread wcurr=NULL;

//counter to assign tids
tid_t count = 1;


tid_t lwp_create(lwpfun function,void *argument){
    //find out page size
    long int pagesize=sysconf(_SC_PAGE_SIZE);
    struct rlimit lim;
    int limresult = getrlimit(RLIMIT_STACK, &lim);
    if(limresult==-1){
        lim.rlim_cur=8000000;
    }
    if(lim.rlim_cur%pagesize!=0){
        lim.rlim_cur=lim.rlim_cur+(pagesize-lim.rlim_cur%pagesize);
    }
    //allocate stack and thread vars
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
        fprintf(stderr,"Stack allocation failed");
        return NO_THREAD;
    }
    if(c==MAP_FAILED){
        fprintf(stderr,"Context allocation failed");
        return NO_THREAD;
    }
    //initialize thread
    c->stack=stack;
    c->stacksize=lim.rlim_cur;
    c->tid=count++;

    //move stack to topmost address 
    stack = (unsigned long *)
    ((unsigned long)c->stack + (unsigned long)c->stacksize-8); 
    stack[-1] = (unsigned long)lwp_wrap; 
    stack[-2] = (unsigned long)stack;
    //setup registers and status
    c->state.fxsave=FPU_INIT;
    c->state.rdi=(unsigned long)function;
    c->state.rsi=(unsigned long)argument;
    c->state.rbp=(unsigned long)(stack-2);
    c->status=LWP_LIVE;
    

    c->lib_one = NULL;
    c->lib_two = NULL;
    c->sched_two = NULL;
    c->sched_one = NULL;
    //add thread to scheduler
    schedule->admit(c);
    //add new thread to allthread list (lwps)
    if(lwps==NULL){
        lwps=c;
        lcurr=lwps;
    }
    else{
        lcurr->lib_one=c;
        lcurr=lcurr->lib_one;
    }
    
    
    return c->tid;
}

void lwp_start(){
    //create new thread (parent) and add to scheduler and all thread list
    thread new = (thread)malloc(sizeof(context));
    new->tid=count++;
    new->status=LWP_LIVE;
    schedule->admit(new);
    if(lwps==NULL){
            lwps=new;
            lcurr=lwps;
        }
        else{
            lcurr->lib_one=new;
            lcurr=lcurr->lib_one;
        }

    curr=new;
    
    lwp_yield();
}

void lwp_yield(){

    thread temp;
    temp = curr;
    //get next in scheduler to switch into
    curr=schedule->next();
    //if thread null, exit with status
    if(curr==NULL){
        lwp_exit(temp->status);
    }
    //save old thread, switch to new thread
    else{
        swap_rfiles(&temp->state, &curr->state);
    }
    
    
}

void lwp_exit(int exitval){

    if(curr!=NULL){
        //remove thread from scheduler 
        // assign it its status
        schedule->remove(curr);
        curr->status=MKTERMSTAT(LWP_TERM,(exitval & 0xFF));

        //add exited thread to zombies list
        if(zombies==NULL){
            zombies=curr;
            zcurr=zombies;
        }
        else{
            zcurr->lib_two=curr;
            zcurr=zcurr->lib_two;
        }
        //readmit waiting parent into scheduler
        if(waits!=NULL){
            schedule->admit(waits);
            waits=waits->exited;
        }
        //switch to next up in scheduler
        lwp_yield();
    }
    
}

tid_t lwp_wait(int *status){
    //if there are no zombies we remove parent from scheduler
    // and then add parent to waitlist and switch to next thread
    if(zombies==NULL){
        schedule->remove(curr);
        if(waits==NULL){
            waits=curr;
            wcurr=waits;
        }
        else{
            wcurr->exited=curr;
            wcurr=wcurr->exited;
        }
        lwp_yield();
    }
    //once parent is readmitted to scheduler, (no longer waiting)
    //begin process of deallocating the exited zombie thread 
    if(zombies!=NULL){
        thread temp=zombies;
        if (status!=NULL){
            *status=temp->status;
        }
        zombies=zombies->lib_two;
        tid_t t=temp->tid;
        removeThread(temp);
        munmap(temp->stack,sizeof(temp->stacksize));
        if(temp!=NULL){
            free(temp);
        }
            return t;
    }
    else{
        return NO_THREAD;
    }
    
    
    
}
//return tid of current thread
tid_t lwp_gettid(void) {
    if (!curr) {
        return NO_THREAD;
    }
    return curr->tid;
}
//return thread given tid
thread tid2thread(tid_t vtid) {
    if(vtid == NO_THREAD)
        return NULL;
    
    thread temp = lwps;
    fprintf(stderr,"vtid: %d\n",vtid);
    printlwps();
    while(temp!=NULL && temp->tid != vtid){
        temp = temp->lib_one;
    }

    if(temp!=NULL && temp->status == LWP_LIVE && temp->tid == vtid){
        fprintf(stderr,"\nreturn: %d\n",temp->tid);
        return temp;
    }
    else{
        return NULL;
    }
    
}
//switch schedulers
void lwp_set_scheduler(scheduler sched){
    //if given same scheduler do nothing
    if(sched==schedule){
        return;
    }
    thread tmp;
    //if new scheduler is null, default to round robin
    if(sched==NULL){
        
        scheduler s = &rr_publish;
        //get next item in scheduler
        //remove it from old add it to new
        while((tmp=schedule->next())!=NULL){
            schedule->remove(tmp);
            s->admit(tmp);
        }
        //shutdown old scheduler
        if (schedule->shutdown) { 
        schedule->shutdown();
    }
    //assign scheduler variable to new
        schedule=s;
    }
    //if new scheduler is not null
    else{
        if (sched->init) { 
            sched->init();
        }
        //get next item in scheduler
        //remove it from old add it to new scheduler
        while((tmp=schedule->next())!=NULL){ 
            schedule->remove(tmp);
            sched->admit(tmp);

        }
        //shutdown old scheduler
        if (schedule->shutdown) { 
            schedule->shutdown();
        }
        //assign scheduler variable to new scheduler
        schedule=sched;
    }
    
}
scheduler lwp_get_scheduler(void) {
    return schedule;
}
// print current list of zombies
void printz(){
    FILE *fp = fopen("test.txt", "a");
    thread z=zombies;
    fprintf(fp,"zombies: ");
    while(z){
        fprintf(fp,"%d->",z->tid);
        z=z->exited;
    }
    fprintf(fp,"\n");
    fclose(fp);
}
// print all current threads
void printlwps(){
    thread l=lwps;
    while(l){
        fprintf(stderr,"%d->",l->tid);
        l=l->lib_one;
    }
    fprintf(stderr,"\n");

}

//wrapper function for lwp_exit
void lwp_wrap(lwpfun fun, void *arg){
    int rval;
    rval=fun(arg);
    lwp_exit(rval);

}

//removes a thread from allthread list (lwps)
//once the parent has deallocated it
void removeThread(thread victim){

    thread prev=NULL;
    thread curr=lwps;
    while(curr!=NULL && curr->tid != victim->tid){
        prev = curr;
        curr = curr->lib_one;
    }

    if(curr==NULL || curr->tid != victim->tid){
        return;
    }
    
    if(curr->lib_one && prev){
        prev->lib_one = curr->lib_one;
        
    }
    
    else if(curr->lib_one==NULL && prev){
        prev->lib_one=NULL;
    }
    
    

}