#ifndef US_EXEC_HELPER_H
#define US_EXEC_HELPER_H

struct us_unitscript;
struct us_string;

int us_prepare_execution_environment( struct us_unitscript* unit, void* param, int(*)(void*) );
char* us_get_shell(const char* prog);
int us_exec( struct us_string* program );
void us_wait_exit(void);
void us_wait_start(void);
void us_wait_notification(int x);

#endif
