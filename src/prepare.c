#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <US/unitscript.h>

int us_prepare( us_unitscript_t* unit ){

  if( unit->uid && unit->user ){
    fprintf(stderr,"The uid and user properties are mutually exclusive.\n");
    return false;
  }

  if( unit->gid && unit->group ){
    fprintf(stderr,"The gid and group properties are mutually exclusive.\n");
    return false;
  }

  if( !unit->uid && !unit->user ){
    fprintf(stderr,"Either uid or user must be specified\n");
    return false;
  }

  if( unit->user ){
    unit->userinfo = getpwnam( unit->user->data );
  }else if( unit->uid ){
    unit->userinfo = getpwuid( *unit->uid );
  }

  if( !unit->userinfo ){
    perror("Failed to optain info of specified user from /etc/passwd");
    return false;
  }

  if( unit->group ){
    unit->groupinfo = getgrnam( unit->group->data );
  }else if( unit->gid ){
    unit->groupinfo = getgrgid( *unit->gid );
  }else{
    unit->groupinfo = getgrgid( unit->userinfo->pw_gid );
  }

  if( !unit->groupinfo ){
    perror("Failed to optain info of group from /etc/passwd");
    return false;
  }

  return true;
}
