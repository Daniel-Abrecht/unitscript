#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <US/logging.h>
#include <US/exec_helper.h>
#include <US/unitscript.h>

static int start_exec( void* x ){
  us_unitscript_t* unit = x;

  switch( unit->startcheck ){
    case STARTCHECK_EXIT: {
      us_wait_exit();
    } break;
    case STARTCHECK_START: {
      us_wait_start();
    } break;
    case STARTCHECK_NOTIFICATION: {
      us_wait_notification(unit->notifyfd ? *unit->notifyfd : 3);
    } break;
    case STARTCHECK_UNKNOWN: return 1;
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
    case US_LOGGING_DEFAULT: return 2;
  }

  return us_exec(unit->program);
}

int us_start( us_unitscript_t* unit ){
  return us_prepare_execution_environment( unit, unit, &start_exec );
}
