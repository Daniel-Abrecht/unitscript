#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <US/unitscript.h>

int us_prepare( us_unitscript_t* unit ){
  int ret = 0;

  if( unit->uid && unit->user ){
    fprintf(stderr,"The 'uid' and 'user' properties are mutually exclusive.\n");
    goto failed;
  }

  if( unit->gid && unit->group ){
    fprintf(stderr,"The 'gid' and 'group' properties are mutually exclusive.\n");
    goto failed;
  }

  if( !unit->uid && !unit->user ){
    fprintf(stderr,"Either 'uid' or 'user' must be specified\n");
    goto failed;
  }

  size_t pwsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if( pwsize == (size_t)-1 )
    pwsize = 1024 * 4; // 3KB

  struct passwd* pwmemory = malloc( sizeof(struct passwd) + pwsize );
  if( !pwmemory ){
    perror("malloc failed");
    goto failed;
  }

  if( unit->user ){
    ret = getpwnam_r( unit->user->data, pwmemory, (char*)(pwmemory+1), pwsize, &unit->userinfo );
  }else if( unit->uid ){
    ret = getpwuid_r( *unit->uid, pwmemory, (char*)(pwmemory+1), pwsize, &unit->userinfo );
  }

  if( !unit->userinfo || ret == -1 ){
    perror("Failed to optain info of specified user from /etc/passwd");
    goto failed_after_getpw;
  }

  size_t grsize = sysconf(_SC_GETGR_R_SIZE_MAX);
  if( grsize == (size_t)-1 )
    pwsize = 1024 * 4; // 3KB

  struct group* grmemory = malloc( sizeof(struct group) + grsize );
  if( !grmemory ){
    perror("malloc failed");
    goto failed_after_getpw;
  }

  if( unit->group ){
    ret = getgrnam_r( unit->group->data, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }else if( unit->gid ){
    ret = getgrgid_r( *unit->gid, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }
  if( ret == -1 || !unit->groupinfo ){
    ret = getgrgid_r( unit->userinfo->pw_gid, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }
  if( ret == -1 || !unit->groupinfo ){
    perror("Failed to optain info of group from /etc/passwd");
    goto failed_after_getgr;
  }

  unit->member_groups_count = 0;
  for( char** it = unit->groupinfo->gr_mem; it && *it; it++ )
    unit->member_groups_count++;
  if( unit->member_groups_count ){
    unit->member_groups = malloc( unit->member_groups_count );
    if(!unit->member_groups){
      perror("malloc failed");
      goto failed_after_getgr;
    }
    size_t i = 0;
    for( char** it = unit->groupinfo->gr_mem; it && *it; it++ ){
      struct group* g = getgrnam(*it);
      if( g )
        unit->member_groups[i++] = g->gr_gid;
    }
    unit->member_groups_count = i;
  }

  if( !unit->program ){
    perror("No 'program' specified\n");
    goto failed_after_getgr;
  }

  static const char startcheck_info[] = {
    "  exit: The process forks itself, wait until it exits and check the return code\n"
    "  start: The moment the executable is executed is considered a successful start\n"
    "  notification: The process writes to a newline to a file descriptor to indicate the start succeeded. You can specify the file descriptor using the notifyfd property, default is 3\n"
  };

  if( !unit->startcheck_str ){
    fprintf(stderr,"'start check' method not specified. Can be one of:\n%s",startcheck_info);
    goto failed_after_getgr;
  }

  struct {
    const char* key;
    enum startcheck value;
  } startcheck_options[] = {
    {"exit",STARTCHECK_EXIT},
    {"start",STARTCHECK_START},
    {"notification",STARTCHECK_NOTIFICATION}
  };
  for( size_t i=0, n=sizeof(startcheck_options)/sizeof(*startcheck_options); i<n; i++ ){
    if( strcmp(startcheck_options[i].key,unit->startcheck_str->data) )
      continue;
    unit->startcheck = startcheck_options[i].value;
    break;
  }

  if( !unit->startcheck ){
    fprintf(stderr,"Unsupported 'start check' method. Can be one of:\n%s",startcheck_info);
    goto failed_after_getgr;
  }

  static const char logging_info[] = {
    "  default: equivalent to syslog if 'start check' is 'start', stdout if 'start check' is 'exit', invalid otherwise\n"
    "  syslog: redirect stdout and stderr to syslog before executing 'program'\n"
    "  stdio: keep stdout and stderr unchanged\n"
    "  none: close stdout and stderr before execution\n"
  };

  if( unit->logging_str ){
    struct {
      const char* key;
      enum startcheck value;
    } logging_options[] = {
      {"default",US_LOGGING_DEFAULT},
      {"syslog",US_LOGGING_SYSLOG},
      {"stdio",US_LOGGING_STDIO},
      {"none",US_LOGGING_NONE},
    };
    bool found = false;
    for( size_t i=0, n=sizeof(logging_options)/sizeof(*logging_options); i<n; i++ ){
      if( strcmp(logging_options[i].key,unit->logging_str->data) )
        continue;
      unit->logging = logging_options[i].value;
      found = true;
      break;
    }
    if( !found ){
      fprintf( stderr, "Unsupported 'logging' mode. Can be one of:\n%s", logging_info );
      goto failed_after_getgr;
    }
  }

  if( unit->logging == US_LOGGING_DEFAULT ){
    switch( unit->startcheck ){
      case STARTCHECK_START: {
        unit->logging = US_LOGGING_SYSLOG;
      } break;
      case STARTCHECK_NOTIFICATION: {
        fprintf( stderr, "'logging' mode 'default' invalid for 'start check' 'notification'. Can be one of:\n%s", logging_info );
      }
      case STARTCHECK_EXIT: {
        unit->logging = US_LOGGING_STDIO;
      } break;
      case STARTCHECK_UNKNOWN: break;
    }
  }

  if( !unit->pidfile ){
    unit->pidfile = malloc(sizeof(*unit->pidfile));
    if( !unit->pidfile ){
      perror("malloc failed");
      goto failed_after_getgr;
    }
    const char* name = strrchr(unit->file,'/');
    if( !name ){
      name = unit->file;
    }else{
      name++;
    }
    unit->pidfile->length = strlen(piddir) + strlen(name) + 5;
    unit->pidfile->data = malloc( unit->pidfile->length + 1 );
    if( !unit->pidfile->data ){
      perror("malloc failed");
      free(unit->pidfile);
      unit->pidfile = 0;
      goto failed_after_getgr;
    }
    snprintf(unit->pidfile->data,unit->pidfile->length+1,"%s/%s.pid",piddir,name);
    unit->pidfile->data[unit->pidfile->length] = 0;
  }

  if( !unit->manage_pidfile ){
    switch( unit->startcheck ){
      case STARTCHECK_NOTIFICATION: {
        fprintf(stderr,
          "'manage pidfile' must be specified for 'start check' 'notification'. "
          "Valid values are 'yes' or 'no'. "
          "Default for 'start check' 'exit' is 'no'. "
          "Default for 'start check' 'start' is 'yes'. \n"
        );
        goto failed_after_getgr;
      } break;
      case STARTCHECK_EXIT: {
        unit->manage_pidfile = malloc(sizeof(*unit->manage_pidfile));
        if(!unit->manage_pidfile){
          perror("malloc failed");
          goto failed_after_getgr;
        }
        *unit->manage_pidfile = false;
      } break;
      case STARTCHECK_START: {
        unit->manage_pidfile = malloc(sizeof(*unit->manage_pidfile));
        if(!unit->manage_pidfile){
          perror("malloc failed");
          goto failed_after_getgr;
        }
        *unit->manage_pidfile = true;
      } break;
      case STARTCHECK_UNKNOWN: break;
    }
  }

  return true;
failed_after_getgr:
  free(grmemory);
  unit->groupinfo = 0;
failed_after_getpw:
  free(pwmemory);
  unit->userinfo = 0;
failed:
  return false;
}

void us_free( us_unitscript_t* unit ){
  if( unit->userinfo )
    free(unit->userinfo);
  if( unit->groupinfo )
    free(unit->groupinfo);
  FREE_YAML( unitscript, &unit );
}
