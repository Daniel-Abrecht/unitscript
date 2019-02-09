#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
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

int us_prepare_execution_environment( gen_unitscript_t* unit, void* param, int(*func)(void*) ){
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

  pid_t fres = fork();
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
  close(pfds[0]);
  int openfdmax = sysconf(_SC_OPEN_MAX);
  if( dup2(pfds[1],openfdmax-1) != -1 ){
    close(pfds[1]);
    pfds[1] = fd = openfdmax - 1;
  }else{
    fd = pfds[1];
  }
  for( int fdi=3; fdi<openfdmax; fdi++ )
    if( fdi != fd )
      close(fdi);

  for( size_t i=0; i<unit->rlimits.length; i++ ){
    if( setrlimit( unit->rlimits.entries[i].resource, (struct rlimit[]){{
          .rlim_cur = unit->rlimits.entries[i].cur,
          .rlim_max = unit->rlimits.entries[i].max
        }}) == -1
    ) fprintf(stderr,"Warning: failed to set rlimit %s: %s\n",unit->rlimits.entries[i].name,strerror(errno));
  }

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
  if( unit->env_files )
    for( size_t i=0; i<unit->env_files->length; i++ )
      us_read_env_file(unit->env_files->entries[i].data);
  if( unit->env_scripts )
    for( size_t i=0; i<unit->env_scripts->length; i++ )
      us_exec_env_script(unit->env_scripts->entries[i].data);
  if( unit->env )
    for( size_t i=0; i<unit->env->length; i++ )
      setenv(
        unit->env->entries[i].key.data,
        unit->env->entries[i].value.data,
        true
      );
  setenv("PIDFILE",unit->pidfile->data,true);
  setenv("HOME",unit->userinfo->pw_dir,false);
  setenv("SHELL",unit->userinfo->pw_shell,false);

  umask(*unit->umask);

  ret = (*func)(param);

error:
  if( ret )
    write(fd,"!",1);
  close(fd);
  us_free(unit);
  exit(ret);
}

bool us_read_env_file( const char* file ){
  int fd = open(file,O_RDONLY);
  if( fd == -1 )
    return false;
  char buf[4096];
  buf[sizeof(buf)-1] = 0;
  size_t m = 0;
  while( true ){
    ssize_t n = read( fd, buf+m, sizeof(buf)-1-m );
    if( n<0 ){
      if( errno == EAGAIN )
        continue;
      perror("Warning: us_read_env_file: read failed");
      break;
    }
    m += n;
    buf[m] = 0;
    while( m ){
      char* value = strchr(buf,'=');
      char* comment = strchr(buf,'#');
      char* end = strchr(buf,'\n');
      if( !end && !n ){
        end = buf + m;
        m++;
      }
      if( ( !comment && !value && end ) || ( comment && comment < value ) || ( end && end < value ) ){
        if( end ){
          memmove(buf,end+1,(buf+m)-end+1);
          m -= end-buf+1;
          continue;
        }else{
          memmove(buf,comment+1,(buf+m)-comment+1);
          m -= comment-buf+1;
          break;
        }
      }
      if( !value || ( !end && !comment ) ) break;
      *value = 0;
      if( comment && ( !end || comment < end ) ){
        *comment = 0;
      }else{
        *end = 0;
      }
      if( setenv(buf,value+1,true) == -1 )
        perror("Warning: us_read_env_file: setenv failed");
      if(end){
        memmove(buf,end+1,(buf+m)-end+1);
        m -= end-buf+1;
        continue;
      }else{
        memmove(buf,comment+1,(buf+m)-comment+1);
        m -= comment-buf+1;
        break;
      }
    }
    if( m > sizeof(buf)-1 ){
      fprintf(stderr,"Warning: us_read_env_file: an environment variable or comment was too big\n");
      break;
    }
    if(!n) break;
  }
  close(fd);
  return true;
}

bool us_exec_env_script( const char* script ){
  int fds[2];
  if( pipe(fds) == -1 ){
    perror("Warning: us_exec_env_script: pipe failed");
    return false;
  }
  pid_t ret = fork();
  if( ret == -1 ){
    perror("Warning: us_exec_env_script: fork failed");
    close(fds[0]);
    close(fds[1]);
    return false;
  }
  if( ret ){
    char buf[4096];
    buf[sizeof(buf)-1] = 0;
    close(fds[1]);
    size_t m = 0;
    while( true ){
      ssize_t n = read( fds[0], buf+m, sizeof(buf)-1-m );
      if( n<0 ){
        if( errno == EAGAIN )
          continue;
        perror("Warning: us_exec_env_script: read failed");
        break;
      }
      m += n;
      buf[m] = 0;
      while( true ){
        size_t l = strlen(buf);
        if( l >= m && n )
          break;
        char* value = strchr(buf,'=');
        if(!value)
          break;
        *(value++) = 0;
        if( setenv(buf,value,true) == -1 )
          perror("Warning: us_exec_env_script: setenv failed");
        if( l < m ){
          memmove(buf,buf+l+1,m-l+1);
          m -= l + 1;
        }
      }
      if( m > sizeof(buf)-1 ){
        fprintf(stderr,"Warning: us_exec_env_script: an environment variable was too big\n");
        break;
      }
      if(!n) break;
    }
    close(fds[0]);
    while( waitpid(ret,0,0) == -1 && errno == EAGAIN );
    return true;
  }else{
    close(fds[0]);
    close(0);
    if( fds[1] != 1 )
      dup2(fds[1],1);
    close(fds[1]);
    clearenv();
    execlp("sh","sh","-c","set -a; . \"$1\" >&2; env -0","--",script,0);
    perror("Warning: us_exec_env_script: exec failed");
    return false;
  }
}

int us_exec( gen_string_t* program ){
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
    shell = strdup("sh -l");

  return shell;
}
