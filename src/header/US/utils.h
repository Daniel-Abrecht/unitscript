#ifndef US_UTILS_H
#define US_UTILS_H

#include <stddef.h>

#define US_UNPACK(...) __VA_ARGS__

typedef long us_integer_t;
typedef bool us_boolean_t;

typedef struct us_string {
  char* data;
  size_t length;
} us_string_t;

typedef struct us_map_entry {
  us_string_t key;
  us_string_t value;
} us_map_entry_t;

typedef struct us_map_t {
  us_map_entry_t* entries;
  size_t length;
} us_map_t;

#endif
