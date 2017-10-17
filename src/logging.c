#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <US/unitscript.h>
#include <US/logging.h>


int us_syslog_redirect( struct us_unitscript* unit, int priority ){
  int fds[2];

  if( pipe(fds) ){
    perror("pipe failed");
    return -1;
  }

  pid_t ret = fork();
  if( ret == -1 ){
    perror("fork failed\n");
    close(fds[0]);
    close(fds[1]);
    return -1;
  }
  if( ret ){
    close(fds[0]);
    return fds[1];
  }

  int openfdmax = sysconf(_SC_OPEN_MAX);
  for( int fdi=0; fdi<openfdmax; fdi++ )
    if( fdi != fds[0] )
      close(fdi);

  const char* file = strrchr(unit->file,'/');
  if(!file){
    file = unit->file;
  }else{
    file++;
  }
  openlog(file,0,LOG_DAEMON);
  us_free(unit);

  do {
    char msg[1024*4];
    size_t i = 0;
    while( i<sizeof(msg)-1 && ( ( (ret=read(fds[0],msg+i,1)) == -1 && errno == EAGAIN ) || ( ret==1 && msg[i] != '\n' ) ) )
      i++;
    msg[i] = 0;
    if(i) syslog( priority, "%s", msg );
  } while(ret);

  exit(0);
}
