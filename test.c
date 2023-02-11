#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "lwp.h"
#include "schedulers.h"

#define MAXSNAKES  100

static void indentnum(void *num);

int main(int argc, char *argv[]){


  printf("Launching LWPS\n");

  /* spawn a number of individual LWPs */
  long  i = 100;
  lwp_create((lwpfun)indentnum, (void*)i);


  lwp_start();

  /* spawn a number of individual LWPs */

    
int status;
  lwp_wait(&status);

  printf("Bye\n");
  lwp_exit(0);
  return 0;
}

static void indentnum(void *num) {
  /* print the number num num times, indented by 5*num spaces
   * Not terribly interesting, but it is instructive.
   */
    int howfar;

    howfar=(long)num;
    
    if (howfar!=0){
      fprintf(stderr,"%d\n",howfar);
      howfar--;
    

      lwp_create((lwpfun)indentnum, (void*)howfar);
      int status;
      tid_t t;
      t = lwp_wait(&status);
    }
    lwp_exit(howfar);

  }
                   /* bail when done.  This should
                                 * be unnecessary if the stack has
                                 * been properly prepared
                                 */


