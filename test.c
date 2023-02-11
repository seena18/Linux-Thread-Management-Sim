#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lwp.h"
#include "schedulers.h"

#define MAXSNAKES  100

static void indentnum(void *num);

int main(int argc, char *argv[]){
  long i;
  printf("%d\n",sizeof(tid_t*));
  printf("Launching LWPS\n");

  /* spawn a number of individual LWPs */
  for(i=1;i<=5;i++) {
    lwp_create((lwpfun)indentnum,(void*)i);
  }

  lwp_start();

  /* spawn a number of individual LWPs */
  for(i=1;i<=5;i++) {
    int status,num;
    tid_t t;
    t = lwp_wait(&status);
    num = LWPTERMSTAT(status);
    printf("Thread %ld exited with status %d\n",t,num);
  }

  printf("Back from LWPS.\n");
  lwp_exit(0);
  return 0;
}

static void indentnum(void *num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
  long i;
  int howfar;

  howfar=(long)num;              /* interpret num as an integer */
  for(i=0;i<howfar;i++){
    printf("%*d\n",howfar*5,howfar);
    // printf("TID: %d",lwp_gettid());
    lwp_yield();
               /* let another have a turn */
  }
  
  lwp_exit(i);                  /* bail when done.  This should
                                 * be unnecessary if the stack has
                                 * been properly prepared
                                 */
}

