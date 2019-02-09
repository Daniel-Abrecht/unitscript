#ifndef US_EXEC_HELPER_H
#define US_EXEC_HELPER_H

#include <stdbool.h>

struct gen_unitscript;
struct gen_string;

int us_prepare_execution_environment( struct gen_unitscript* unit, void* param, int(*)(void*) );
char* us_get_shell(const char* prog);
int us_exec( struct gen_string* program );
void us_wait_exit(void);
void us_wait_start(void);
void us_wait_notification(int x);
bool us_exec_env_script( const char* script );
bool us_read_env_file( const char* file );

#endif
