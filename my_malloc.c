#include "my_malloc.h"

void *ff_malloc(size_t size) {
  return  _malloc(size_t size, &add_to_free_list_ff);
}

void ff_free(void *ptr) {
  _free(void * ptr, &add_to_free_list_ff);
}

void *bf_malloc(size_t size) {
  return  _malloc(size_t size, &add_to_free_list_bf);
}

void bf_free(void *ptr) {
  _free(void * ptr, &add_to_free_list_ff);
}

unsigned long get_data_segment_size() { // in bytes
  return segment_size;
}

unsigned long get_data_segment_free_space_size() { // in bytes
  return segment_free_space_size;
}

void * _malloc(size_t size, add_func_t f) {
  if (size == 0) {
    return NULL;
  }
  void * addr = NULL;
  if ((addr = try_existed_block(size, f)) == NULL) {
    // require a new block
    return add_new_block(size_t size);
  }
  return addr;
}

void * try_existed_block(size_t size, add_func_t f) {
  return NULL;
}

void * add_new_block(size_t size) {
  return NULL;
}

void _free(void * ptr, add_func_t f) {
  if (ptr == NULL) {
    return;
  }

}

void add_to_free_list_ff(meta_data_t *) {
}

void add_to_free_list_bf(meta_data_t *) {
}
