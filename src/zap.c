#include <stdio.h>
#include <unistd.h>
#include <US/unitscript.h>

int us_zap( gen_unitscript_t* unit ){
  if( unlink( unit->pidfile->data ) == -1 ){
    perror("unlink failed");
    return 1;
  }
  return 0;
}
