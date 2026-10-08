#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RUNTIME_ENTRY __attribute__((force_align_arg_pointer))
#define MAX_PRINT_DEPTH 64

extern void go(void);

static int64_t decode(int64_t v) {
  return v >> 1;
}

static int is_encoded_number(int64_t v) {
  return v & 1;
}

static void print_value(int64_t v, int depth) {
  if (is_encoded_number(v)) {
    printf("%" PRId64, decode(v));
    return;
  }
  if (v == 0) {
    printf("0");
    return;
  }
  if (depth >= MAX_PRINT_DEPTH) {
    printf("...");
    return;
  }
  int64_t *array = (int64_t *)v;
  int64_t size = array[0];
  printf("{s:%" PRId64, size);
  for (int64_t i = 0; i < size; i++) {
    printf(", ");
    print_value(array[i + 1], depth + 1);
  }
  printf("}");
}

RUNTIME_ENTRY void print(int64_t v) {
  print_value(v, 0);
  printf("\n");
}

RUNTIME_ENTRY void *allocate(int64_t encoded_words, int64_t fill) {
  if (!is_encoded_number(encoded_words)) {
    fprintf(stderr, "allocate: size %" PRId64 " is not an encoded number\n", encoded_words);
    exit(-1);
  }
  int64_t words = decode(encoded_words);
  if (words < 0) {
    fprintf(stderr, "allocate: negative size %" PRId64 "\n", words);
    exit(-1);
  }
  int64_t *array = malloc((size_t)(words + 1) * sizeof(int64_t));
  if (array == NULL) {
    fprintf(stderr, "allocate: out of memory\n");
    exit(-1);
  }
  array[0] = words;
  for (int64_t i = 0; i < words; i++) {
    array[i + 1] = fill;
  }
  return array;
}

RUNTIME_ENTRY int64_t input(void) {
  char line[128];
  if (fgets(line, sizeof(line), stdin) == NULL) {
    return 1;
  }
  int64_t n = strtoll(line, NULL, 10);
  return (n << 1) | 1;
}

RUNTIME_ENTRY void tuple_error(int64_t *array, int64_t length, int64_t index) {
  printf("attempted to use position %" PRId64 " in an array that only has %" PRId64 " positions\n",
         decode(index), decode(length));
  exit(-1);
}

RUNTIME_ENTRY void array_tensor_error_null(int64_t line) {
  printf("attempted to use a zero-initialized variable at line %" PRId64 "\n", decode(line));
  exit(-1);
}

RUNTIME_ENTRY void array_error(int64_t line, int64_t length, int64_t index) {
  printf("attempted to use position %" PRId64 " in an array that only has %" PRId64 " positions at line %" PRId64 "\n",
         decode(index), decode(length), decode(line));
  exit(-1);
}

RUNTIME_ENTRY void tensor_error(int64_t line, int64_t dimension, int64_t length, int64_t index) {
  printf("attempted to use position %" PRId64 " in dimension %" PRId64 " of a tensor that only has %" PRId64 " positions at line %" PRId64 "\n",
         decode(index), decode(dimension), decode(length), decode(line));
  exit(-1);
}

int main(void) {
  setvbuf(stdout, NULL, _IOFBF, 1 << 16);
  go();
  fflush(stdout);
  return 0;
}
