#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <US/unitscript.h>

const char piddir[] = "/var/run/unitscript";

bool us_init(void){
  struct stat s;
  int ret;

  umask(0177);

  ret = stat(piddir,&s);
  if( ret == -1 ){
    if( errno == ENOENT && getuid() == 0 && getgid() == 0 ){
      umask(0);
      mkdir(piddir,01777);
      umask(0177);
      ret = stat(piddir,&s);
    }
    if( ret == -1  ){
      fprintf( stderr, "stat for %s failed, please create the directory with uid 0, gid 0 and mode 01777 or run the unitscript executable as root: %d: %s\n", piddir, errno, strerror(errno) );
      return false;
    }
  }

  if( ( s.st_mode & 07777 ) != 01777 ){
    fprintf( stderr, "Permission of directory %s set incorrectly, expected 01777, got %o\n", piddir, s.st_mode & 07777 );
    return false;
  }

  if( s.st_uid != 0 ){
    fprintf( stderr, "uid of directory %s set incorrectly, expected 0, got %d\n", piddir, s.st_uid );
    return false;
  }

  if( s.st_gid != 0 ){
    fprintf( stderr, "gid of directory %s set incorrectly, expected 0, got %d\n", piddir, s.st_gid );
    return false;
  }

  return true;
}

int main( int argc, const char* argv[] ){

  if( !us_init() )
    return 1;

  if( argc < 2 ){
    fprintf(stderr,"Usage: %s file {start,stop,status,restart,zap,check}\n",argv[0]);
    return 1;
  }

  if( argc != 3 ){
    fprintf(stderr,"Usage: %s {start,stop,status,restart,zap,check}\n",argv[1]);
    return 1;
  }

  FILE *file = fopen(argv[1], "r");
  if(!file){
    fprintf(stderr,"Failed to open file!\n");
    return 2;
  }

  us_unitscript_t* unit;
  if( !PARSE_YAML(unitscript,file,&unit) || !unit ){
    fprintf(stderr,"something went wrong!\n");
    fclose(file);
    return 3;
  }
  fclose(file);

  unit->file = argv[1];

  int ret = 0;

  if( us_prepare( unit ) ){

    if( !strcmp(argv[2],"start") ){
      ret = us_start( unit );
    }else if( !strcmp(argv[2],"stop") ){
      ret = us_stop( unit );
    }else if( !strcmp(argv[2],"restart") ){
      ret = us_stop( unit );
      if(!ret)
        ret = us_start( unit );
    }else if( !strcmp(argv[2],"status") ){
      ret = us_print_status( unit );
    }else if( !strcmp(argv[2],"zap") ){
      ret = us_zap( unit );
    }else if( !strcmp(argv[2],"check") ){
    }else{
      fprintf(stderr,"Invalid action\n");
      ret = 4;
    }

  }

  us_free(unit);

  return ret;
}
