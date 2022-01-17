#include <stdio.h>
#include <unistd.h>

// structs & global variables
typedef struct meta_data {
  size_t size;
  unsigned char is_used;
  struct meta_data * prev_block;
  struct meta_data * next_block;
  struct meta_data * prev_free_block;
  struct meta_data * next_free_block;
} meta_data_t;

meta_data_t * block_tail = NULL;
meta_data_t * free_list_head = NULL;
unsigned long segment_size = 0;
unsigned long segment_free_space_size = 0;

typedef void (*add_func_t) (meta_data_t *);

// First Fit malloc/free
void *ff_malloc(size_t size);
void ff_free(void *ptr);

// Best Fit malloc/free
void *bf_malloc(size_t size);
void bf_free(void *ptr);

// functions for performance study
unsigned long get_data_segment_size(); // in bytes
unsigned long get_data_segment_free_space_size(); // in bytes

// functions used for implementing malloc
void * _malloc(size_t size, add_func_t f);
void * try_existed_block(size_t size, add_func_t f);
void * add_new_block(size_t size);

// functions used for implementing free
void _free(void * ptr, add_func_t f);

// functions used for manipulating free list
void add_to_free_list_ff(meta_data_t * block);
void add_to_free_list_bf(meta_data_t * block);
