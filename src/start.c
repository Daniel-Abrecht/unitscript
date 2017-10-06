#include <US/exec_helper.h>
#include <US/unitscript.h>

int fdtmp = 0;

static int start_exec( void* x ){
  us_unitscript_t* unit = x;

  return us_exec(unit->program);
}

int us_start( us_unitscript_t* unit ){
  return us_prepare_execution_environment( unit, unit, &start_exec );
}
