#define _DEFAULT_SOURCE
#include <errno.h>
#include <syslog.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bsd/libutil.h>
#include <US/logging.h>
#include <US/exec_helper.h>
#include <US/unitscript.h>

static int start_exec( void* x ){
  us_unitscript_t* unit = x;

  umask(0177);

  switch( unit->startcheck ){
    case STARTCHECK_EXIT: {
      us_wait_exit();
    } break;
    case STARTCHECK_START: {
      us_wait_start();
    } break;
    case STARTCHECK_NOTIFICATION: {
      int nfd = unit->notifyfd ? *unit->notifyfd : 3;
      us_wait_notification(nfd);
      char val[32];
      snprintf(val,sizeof(val),"%d",nfd);
      setenv("NOTIFICATION_FD",val,true);
    } break;
    case STARTCHECK_UNKNOWN: return 1;
  }

  struct pidfh * pfh = 0;
  if( *unit->manage_pidfile ){
    pid_t otherpid;
    while( true ){
      pfh = pidfile_open( unit->pidfile->data, 0600, &otherpid );
      if( !pfh ){
        if( errno == EAGAIN ){
          usleep(100000); // 0.1s
          continue;
        }else if( errno == EEXIST ){
          fprintf( stderr, "Already running with pid %d\n", otherpid );
        }else{
          perror( "pidfile_open failed" );
        }
        return 2;
      }else break;
    }
  }

  switch( unit->logging ){
    case US_LOGGING_SYSLOG: {
      int info = us_syslog_redirect(unit,LOG_INFO);
      if( info ){
        dup2(info,1);
        if( info != 1 )
          close(info);
      }
      int error = us_syslog_redirect(unit,LOG_ERR);
      if( error ){
        dup2(error,2);
        if( error != 2 )
          close(error);
      }
    } break;
    case US_LOGGING_STDIO: break;
    case US_LOGGING_NONE: {
      close(1);
      close(2);
    } break;
    case US_LOGGING_DEFAULT: return 3;
  }

  pidfile_write(pfh);

  umask(*unit->umask);
  int ret = us_exec(unit->program);

  if(pfh)
    pidfile_remove(pfh);

  return ret;
}

int us_start( us_unitscript_t* unit ){
  return us_prepare_execution_environment( unit, unit, &start_exec );
}
