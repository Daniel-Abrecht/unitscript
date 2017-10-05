#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <US/unitscript.h>

int main( int argc, const char* argv[] ){
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

  FREE_YAML( unitscript, &unit );

  return ret;
}
