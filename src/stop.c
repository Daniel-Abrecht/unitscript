#define _DEFAULT_SOURCE
#include <stdio.h>
#include <US/unitscript.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

int us_stop( gen_unitscript_t* unit ){
  enum us_status status;
  pid_t pid;
  int ret = us_status( unit, &status, &pid );
  if(ret)
    return ret;
  switch( status ){
    case US_STARTED: {
      if( kill(pid,SIGTERM) == -1 ){
        if( errno != ESRCH ){
          perror("kill failed");
          return 1;
        }
      }else{
        while( true ){
          ret = us_status( unit, &status, &pid );
          if( ret || status == US_STOPPED )
            break;
          usleep( 100000 ); // sleep 0.1 seconds
        };
      }
    } break;
    case US_STOPPED: break;
  }
  return 0;
}
