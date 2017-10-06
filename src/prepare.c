#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdlib.h>
#include <US/unitscript.h>

int us_prepare( us_unitscript_t* unit ){
  int ret = 0;

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

  size_t pwsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if( pwsize == (size_t)-1 )
    pwsize = 1024 * 4; // 3KB

  struct passwd* pwmemory = malloc( sizeof(struct passwd) + pwsize );
  if( !pwmemory ){
    perror("malloc failed");
    return false;
  }

  if( unit->user ){
    ret = getpwnam_r( unit->user->data, pwmemory, (char*)(pwmemory+1), pwsize, &unit->userinfo );
  }else if( unit->uid ){
    ret = getpwuid_r( *unit->uid, pwmemory, (char*)(pwmemory+1), pwsize, &unit->userinfo );
  }

  if( !unit->userinfo || ret == -1 ){
    perror("Failed to optain info of specified user from /etc/passwd");
    free(pwmemory);
    unit->userinfo = 0;
    return false;
  }

  size_t grsize = sysconf(_SC_GETGR_R_SIZE_MAX);
  if( grsize == (size_t)-1 )
    pwsize = 1024 * 4; // 3KB

  struct group* grmemory = malloc( sizeof(struct group) + grsize );
  if( !pwmemory ){
    perror("malloc failed");
    return false;
  }

  if( unit->group ){
    ret = getgrnam_r( unit->group->data, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }else if( unit->gid ){
    ret = getgrgid_r( *unit->gid, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }
  if( ret == -1 || !unit->groupinfo ){
    ret = getgrgid_r( unit->userinfo->pw_gid, grmemory, (char*)(grmemory+1), grsize, &unit->groupinfo );
  }
  if( ret == -1 || !unit->groupinfo ){
    perror("malloc failed");
    free(pwmemory);
    free(grmemory);
    unit->groupinfo = 0;
    unit->userinfo = 0;
    return false;
  }

  unit->member_groups_count = 0;
  for( char** it = unit->groupinfo->gr_mem; it && *it; it++ )
    unit->member_groups_count++;
  if( unit->member_groups_count ){
    unit->member_groups = malloc( unit->member_groups_count );
    if(!unit->member_groups){
      perror("malloc failed");
      free(pwmemory);
      free(grmemory);
      unit->groupinfo = 0;
      unit->userinfo = 0;
      return false;
    }
    size_t i = 0;
    for( char** it = unit->groupinfo->gr_mem; it && *it; it++ ){
      struct group* g = getgrnam(*it);
      if( g )
        unit->member_groups[i++] = g->gr_gid;
    }
    unit->member_groups_count = i;
  }

  if( !unit->groupinfo ){
    perror("Failed to optain info of group from /etc/passwd");
    return false;
  }

  if( !unit->program ){
    perror("No program specified\n");
    return false;
  }

  return true;
}

void us_free( us_unitscript_t* unit ){
  if( unit->userinfo )
    free(unit->userinfo);
  if( unit->groupinfo )
    free(unit->groupinfo);
  FREE_YAML( unitscript, &unit );
}
