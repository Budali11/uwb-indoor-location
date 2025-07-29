#include "util.h"
#include <stddef.h>
#include <stdint.h>

char *find_pattern(char *a, char *b, uint32_t a_len, uint32_t b_len) {
  char *match_ptr = 0;
  uint8_t isFound = 0;
  for (int i = 0; i < a_len; i++) {
    for (int j = 0; j < b_len; j++) {
      if (a[i + j] != b[j]) {
        isFound = 0;
        break;
      }
      isFound = 1;
    }
    if (isFound == 1) {
      match_ptr = a + i + b_len;
      return match_ptr;
    }
  }
  return NULL;
}
