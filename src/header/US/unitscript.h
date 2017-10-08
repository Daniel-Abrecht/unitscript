#ifndef US_UNITSCRIPT_H
#define US_UNITSCRIPT_H

#include <unitscript.template>
#include <sys/types.h>

enum us_status {
  US_STARTED,
  US_STOPPED
};

extern const char piddir[];

int us_prepare( us_unitscript_t* unit );
void us_free( us_unitscript_t* unit );

int us_start( us_unitscript_t* unit );
int us_stop( us_unitscript_t* unit );
int us_status( us_unitscript_t* unit, enum us_status* status, pid_t* );
int us_print_status( us_unitscript_t* unit );
int us_zap( us_unitscript_t* unit );

#endif
