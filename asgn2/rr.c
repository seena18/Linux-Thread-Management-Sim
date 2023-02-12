#include "lwp.h"
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
thread first = NULL;
thread last = NULL;
void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next(void);
void printlist();

// moves on to next thread in scheduler
thread rr_next(void) {
    thread curr;
    
    // if empty scheduler
    if(first==NULL){
        return NULL;
    }
    else{
        curr=first;
        first=first->sched_one;
        if(first!=NULL){
            first->sched_two=NULL;
        }
        else{
            last=NULL;
        }
        curr->sched_one=NULL;
        curr->sched_two=NULL;
        
        // moves executed thread to end of scheduler
        rr_admit(curr);
        
        return curr;
    }
}

// removes given thread from scheduler
void rr_remove(thread victim){

    thread curr=first;

    while(curr!=NULL && curr->tid != victim->tid){
        curr = curr->sched_one;
    }
    // if target thread not found
    if(curr==NULL || curr->tid != victim->tid){
        return;
    }
    
    if(curr->sched_one){
        curr->sched_one->sched_two=curr->sched_two;
    }
    // if target thread is at end of scheduler
    else{
        last=curr->sched_two;
    }
    if(curr->sched_two){
        curr->sched_two->sched_one=curr->sched_one;
    }
    // if target thread is at beginning of scheduler
    else{
        first=curr->sched_one;
    }
}

// adds given thread to scheduler
void rr_admit(thread new){
    new->sched_one=NULL;
    // new->sched_two=NULL;

    // if empty scheduler
    if(first==NULL && last==NULL){
        first=new;
        last=new;
    }
    else{
        last->sched_one=new;
        new->sched_two=last;
        last=new;
    }

}
