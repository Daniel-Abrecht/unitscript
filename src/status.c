#define _DEFAULT_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <bsd/libutil.h>
#include <US/unitscript.h>

int us_status( us_unitscript_t* unit, enum us_status* status, pid_t* pid ){
  struct pidfh * pfh = 0;
  pid_t otherpid;
  while( true ){
    pfh = pidfile_open( unit->pidfile->data, 0600, &otherpid );
    if( !pfh ){
      if( errno == EAGAIN ){
        usleep(100000); // 0.1s
        continue;
      }else if( errno == EEXIST ){
        if(status) *status = US_STARTED;
        if(pid) *pid = otherpid;
        return 0;
      }else{
        perror( "pidfile_open failed" );
        return 10;
      }
    }else break;
  }
  if(status) *status = US_STOPPED;
  if(pid) *pid = 0;
  if(pfh) pidfile_remove(pfh);
  return 0;
}

int us_print_status( us_unitscript_t* unit ){
  (void)unit;
  enum us_status status;
  pid_t pid;
  int ret = us_status( unit, &status, &pid );
  if(ret)
    return ret;
  switch( status ){
    case US_STARTED: {
      printf("Running at pid %d\n",pid);
      ret = 0;
    } break;
    case US_STOPPED: {
      printf("Not running\n");
      ret = 1;
    } break;
  }
  return ret;
}
