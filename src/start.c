#include <stdio.h>
#include <US/unitscript.h>

int us_start( us_unitscript_t* unit ){
  (void)unit;
  puts("us_start: TODO");

  printf( "%.*s\n", (int)unit->user->length, unit->user->data );
  printf( "%.*s\n", (int)unit->group->length, unit->group->data );
  printf( "%.*s\n", (int)unit->program->length, unit->program->data );

  if( unit->env ){
    for( size_t i=0; i<unit->env->length; i++ )
      printf( "%.*s: %.*s\n",
        (int)unit->env->entries[i].key.length,
        unit->env->entries[i].key.data,
        (int)unit->env->entries[i].value.length,
        unit->env->entries[i].value.data
      );
  }

  return 0;
}
