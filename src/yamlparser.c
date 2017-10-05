#define US_YAML_PARSER_INTERNALS

#include <stdio.h>
#include <US/yamlparser.generator>

void free_yaml_string( us_string_t** ch ){
  us_string_t* c = *ch;
  if(!c) return;
  *ch = 0;
  if( c->data )
    free(c->data);
  free(c);
}

void free_yaml_map( us_map_t** ch ){
  us_map_t* c = *ch;
  if(!c) return;
  *ch = 0;
  while( c->length-- ){
    us_map_entry_t* e = c->entries + c->length;
    if( e->key.data )
      free(e->key.data);
    if( e->value.data )
      free(e->value.data);
  }
  if( c->entries )
    free(c->entries);
  free(c);
}

bool parse_yaml_string( us_parser_t* s, us_string_t** ch ){
  if( s->state != PARSER_STATE_VALUE )
    return true;
  us_string_t* c = calloc(1,sizeof(us_string_t));
  if(!c){
    perror("calloc failed");
    s->done = true;
    return false;
  }
  *ch = c;
  c->data = malloc(s->length+1);
  memcpy(c->data,s->value,s->length);
  c->data[s->length] = 0;
  c->length = s->length;
  return true;
}

bool parse_yaml_map( struct us_parser* s, us_map_t** ch ){
  yaml_token_t token;

  if( s->state != PARSER_STATE_MAPPING )
    return false;

  size_t length = 0;
  char* key = 0;
  bool done = false;

  us_map_t* c = calloc(1,sizeof(us_map_t));
  if(!c){
    perror("calloc failed");
    s->done = true;
    return false;
  }
  *ch = c;

  do {

    if( !yaml_parser_scan(s->parser, &token) ){
      fprintf(stderr,"yaml_parser_scan failed");
      s->done = true;
      return false;
    }

    bool next = false;

    switch( token.type ){
      case YAML_KEY_TOKEN: {
        s->state = PARSER_STATE_KEY;
      } break;
      case YAML_VALUE_TOKEN: {
        s->state = PARSER_STATE_VALUE;
      } break;
      case YAML_SCALAR_TOKEN: {
        if( s->state == PARSER_STATE_KEY ){
          length = token.data.scalar.length;
          key = malloc(length+1);
          if(!key){
            perror("malloc failed");
            s->done = true;
            return false;
          }
          memcpy(key,token.data.scalar.value,length);
          key[length] = 0;
        }else{
          s->value = (const char*)token.data.scalar.value;
          s->length = token.data.scalar.length;
          next = true;
        }
      } break;
      case YAML_BLOCK_MAPPING_START_TOKEN: {
        fprintf(stderr,"Expected <key>:<value> list\n");
        s->done = true;
        return false;
      } break;
      case YAML_BLOCK_END_TOKEN: {
        done = true;
      } break;
      default: break;
    }

    if( next && s->state == PARSER_STATE_VALUE ){
      us_map_entry_t* el = realloc( c->entries, (c->length+1) * sizeof(us_map_entry_t) );
      if(!el){
        perror("realloc failed");
        s->done = true;
        return false;
      }
      c->entries = el;
      us_map_entry_t* e = el + c->length;
      e->key.data = key;
      e->key.length = length;
      e->value.length = 0;
      c->length++;
      e->value.data = malloc(s->length+1);
      if(!e->value.data){
        perror("malloc failed");
        s->done = true;
        return false;
      }
      memcpy(e->value.data,s->value,s->length);
      e->value.data[s->length] = 0;
      e->value.length = s->length;
      key = 0;
    }

    s->done = token.type == YAML_STREAM_END_TOKEN;
    yaml_token_delete(&token);
  } while( !s->done && !done );

  if(key)
    free( key );

  return true;
}

bool parse_yaml_skip_unknown_mapping( struct us_parser* s ){
  size_t count = 1;
  do {
    yaml_token_t token;
    if( !yaml_parser_scan(s->parser, &token) ){
      fprintf(stderr,"yaml_parser_scan failed");
      s->done = true;
      return false;
    }
    if( token.type == YAML_BLOCK_MAPPING_START_TOKEN ){
      count++;
    }else if( token.type == YAML_BLOCK_END_TOKEN ){
      count--;
    }else if( token.type == YAML_STREAM_END_TOKEN ){
      s->done = true;
    }
    yaml_token_delete(&token);
  } while( !s->done && count );
  return true;
}

bool parse_yaml(FILE* file, void** ret, bool(*parser_func)(us_parser_t* s, void** ret), void(*free_func)(void** ret) ){
  yaml_parser_t parser;

  *ret = 0;

  if(!yaml_parser_initialize(&parser)){
    fprintf(stderr,"Failed to initialize parser!\n");
    return false;
  }

  yaml_parser_set_input_file(&parser, file);

  us_parser_t s = {
    .parser = &parser,
    .value = 0,
    .length = 0,
    .state = PARSER_STATE_MAPPING,
    .done = false
  };

  bool done = false;
  bool found = false;
  do {
    yaml_token_t token;
    if( !yaml_parser_scan(&parser, &token) ){
      fprintf(stderr,"yaml_parser_scan failed");
      goto failed_after_parser_initialize;
    }
    if( token.type == YAML_BLOCK_MAPPING_START_TOKEN )
      found = true;
    if( token.type == YAML_STREAM_END_TOKEN )
      done = true;
    yaml_token_delete(&token);
  } while( !done && !found );
  if(!found){
    fprintf(stderr,"No datas found\n");
    goto failed_after_parser_initialize;
  }

  if( !(*parser_func)( &s, ret ) )
    goto failed_after_parser_initialize;

  yaml_parser_delete(&parser);
  return true;
failed_after_parser_initialize:
  yaml_parser_delete(&parser);
  if(*ret)
    (*free_func)(ret);
  return false;
}
