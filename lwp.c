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
    // FILE *fp = fopen("log.txt", "a+");
    // fprintf(fp,"rlim: %d",lim.rlim_cur);
    // fclose(fp);
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
    c->stack=stack;
    c->stacksize=lim.rlim_cur;
    c->tid=count++;
    stack = (unsigned long *)
    ((unsigned long)c->stack + (unsigned long)c->stacksize-8);
    // FILE *fp = fopen("log.txt", "a+");
    // fprintf(fp,"stack init: %d",sizeof(lwp_exit));
    // fclose(fp);
    // stack[0] = (unsigned long)lwp_exit;
    // fp = fopen("log.txt", "a+");
    // fprintf(fp,"stack init2\n");
    // fclose(fp);
    stack[-1] = (unsigned long)lwp_wrap; 
    stack[-2] = (unsigned long)stack;
    
    c->state.fxsave=FPU_INIT;
    c->state.rdi=(unsigned long)function;
    c->state.rsi=(unsigned long)argument;
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
        lcurr=lcurr->lib_one;
    }
    
    
    return c->tid;
}
void lwp_start(){
    
    thread new = (thread)malloc(sizeof(context));
    new->tid=count++;
    new->status=LWP_LIVE;
    schedule->admit(new);
    // fprintf(stderr,"Ptid: %d\n",new->tid);
    counter++;
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
        // fprintf(stderr,"Exitval: %d\n",exitval);
        // fprintf(stderr,"Exitval: %d\n",(exitval & 0xFF));
        curr->status=MKTERMSTAT(LWP_TERM,(exitval & 0xFF));
        if(zombies==NULL){
            zombies=curr;
            zcurr=zombies;
        }
        else{
            zcurr->lib_two=curr;
            zcurr=zcurr->lib_two;
        }
        // printz();
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
    // printz();
    // FILE *fp = fopen("test.txt", "a");
    // fprintf(fp,"Wait: %d\n", curr->tid);
    // fclose(fp);
    // fprintf(stderr,"Wait: %d\n", curr->tid);
    if(zombies==NULL){
        schedule->remove(curr);
        counter--;
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
    if(zombies!=NULL){
        thread temp=zombies;
        // fprintf(stderr,"TID: %d\n", zombies->tid);
        if (status!=NULL){
            *status=temp->status;
        }
        zombies=zombies->lib_two;
        tid_t t=temp->tid;
        removeThread(temp);
        munmap(temp->stack,sizeof(temp->stacksize));
        // fprintf(stderr,"TID: %d\n", temp->tid);
        if(temp!=NULL){
            free(temp);
        }
        
    
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

thread tid2thread(tid_t vtid) {
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
void lwp_set_scheduler(scheduler sched){
    
    if(sched==schedule){
        return;
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
        // fprintf(stderr,"Switching sched\n");
        if (sched->init) { 
            sched->init();
        }
        int index=0;

        while((tmp=schedule->next())!=NULL){
            // fprintf(stderr,"On tid: %d\n",tmp->tid);
            
            schedule->remove(tmp);
            sched->admit(tmp);
            
            // fprintf(stderr,"Success admit tid: %d\n",tmp->tid);
            
            // fprintf(stderr,"Success remove tid: %d\n",tmp->tid);
            index++;
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
void printlwps(){
    thread l=lwps;
    while(l){
        fprintf(stderr,"%d->",l->tid);
        l=l->lib_one;
    }
    fprintf(stderr,"\n");

}
void lwp_wrap(lwpfun fun, void *arg){
    int rval;
    rval=fun(arg);
    lwp_exit(rval);

}
void removeThread(thread victim){
    // fprintf(stderr,"test1\n");
    thread prev=NULL;
    thread curr=lwps;
    while(curr!=NULL && curr->tid != victim->tid){
        prev = curr;
        curr = curr->lib_one;
    }
    // fprintf(stderr,"test2\n");
    if(curr==NULL || curr->tid != victim->tid){
        return;
    }
    
    if(curr->lib_one && prev){
        // fprintf(stderr,"test3\n");
        prev->lib_one = curr->lib_one;
        
        // fprintf(stderr,"test4\n");
    }
    
    else if(curr->lib_one==NULL && prev){
        prev->lib_one=NULL;
    }
    
    

}