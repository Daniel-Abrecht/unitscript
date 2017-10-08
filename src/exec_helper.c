#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <US/unitscript.h>
#include <US/exec_helper.h>

enum exec_env_role {
  EXEC_ENV_ROLE_PARENT,
  EXEC_ENV_ROLE_CHILD
};

static int fd = -1;
static enum exec_env_role role = EXEC_ENV_ROLE_PARENT;

int us_prepare_execution_environment( us_unitscript_t* unit, void* param, int(*func)(void*) ){
  int ret = 0;

  if( role != EXEC_ENV_ROLE_PARENT ){
    fprintf(stderr,"us_prepare_execution_environment mustn't be called recurively\n");
    return 10;
  }

  int pfds[2];
  if( pipe( pfds ) ){
    perror("pipe failed");
    return 11;
  }

  int fres = fork();
  if( fres == -1 ){
    close(pfds[0]);
    close(pfds[1]);
  }else if( fres ){
    fd = pfds[0];
    close(pfds[1]);
    char c = 0;
    int rres;
    bool out = false;
    while( ( (rres = read(fd,&c,1)) == -1 && errno == EINTR ) || ( rres == 1 && c != '\n' ) )
      out = true;
    if( out && c != '\n' ){
      while( waitpid(fres,&ret,0) == -1 && errno == EINTR );
      if(WIFEXITED(ret))
        ret = WEXITSTATUS(ret);
    }
    close(fd);
    return ret;
  }

  role = EXEC_ENV_ROLE_CHILD;

  fd = pfds[1];
  close(pfds[0]);
  int openfdmax = sysconf(_SC_OPEN_MAX);
  for( int fdi=3; fdi<openfdmax; fdi++ )
    if( fdi != fd )
      close(fdi);

  if( unit->groupinfo->gr_gid != getgid() ){
    if( setgroups(unit->member_groups_count,unit->member_groups) ){
      perror("setgroups failed");
      ret = 15;
      goto error;
    }
    if( setgid(unit->groupinfo->gr_gid) ){
      perror("setgid failed");
      ret = 16;
      goto error;
    }
  }

  if( setuid(unit->userinfo->pw_uid) ){
    perror("setuid failed");
    ret = 17;
    goto error;
  }

  clearenv();
  if( unit->env )
    for( size_t i=0; i<unit->env->length; i++ )
      setenv(
        unit->env->entries[i].key.data,
        unit->env->entries[i].value.data,
        true
      );
  setenv("PIDFILE",unit->pidfile->data,true);

  ret = (*func)(param);
error:
  if( ret )
    write(fd,"!",1);
  close(fd);
  us_free(unit);
  exit(ret);
}

int us_exec( us_string_t* program ){
  int ret = 0;

  if( role != EXEC_ENV_ROLE_CHILD ){
    fprintf(stderr,"us_exec must be called in a callback of us_prepare_execution_environment\n");
    return 1;
  }

  char name[] = "/tmp/unitscriptXXXXXX";
  int fdtmp = mkstemp(name);
  if( !fdtmp ){
    perror("failed to open temporary file");
    return 2;
  }
  unlink(name);

  write(fdtmp,program->data,program->length);
  lseek(fdtmp,0,SEEK_SET);
  dup2(fdtmp,0);
  close(fdtmp);

  char* shell = us_get_shell(program->data);
  if( !shell ){
    perror("us_get_shell failed");
    return 3;
  }
  char* argument = strchr(shell,' ');
  if( argument )
    *(argument++) = 0;

  if(argument){
    ret = execlp(shell,shell,argument,"/dev/stdin",0);
  }else{
    ret = execlp(shell,shell,"/dev/stdin",0);
  }
  if( ret == -1 )
    perror("execlp failed");

  free(shell);
  close(0);
  return 4;
}

void us_wait_exit(void){
  write(fd,"+",1);
  close(fd);
}

void us_wait_start(void){
  fcntl(fd,F_SETFD,FD_CLOEXEC);
}

void us_wait_notification(int x){
  if( x != fd ){
    dup2(fd,x);
    close(fd);
    fd = x;
  }
}

char* us_get_shell(const char* prog){

  char* shell = 0;

  if( prog[0] == '#'
   && prog[1] == '!'
  ){
    const char* end = strchr(prog + 2,'\n');
    if(!end)
      end = prog + strlen(prog+2);
    size_t length = end - prog - 2;
    if( length > 256 )
      return 0;
    shell = malloc(length+1);
    if( !shell )
      return 0;
    memcpy(shell,prog+2,length);
    shell[length] = 0;
  }

  if(!shell)
    shell = strdup("sh");

  return shell;
}
