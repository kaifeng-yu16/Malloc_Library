#include "my_malloc.h"

void *ff_malloc(size_t size) {
  return  f_malloc(size, &add_to_free_list_ff);
}

void ff_free(void *ptr) {
  f_free(ptr, &add_to_free_list_ff);
}

void *bf_malloc(size_t size) {
  return  f_malloc(size, &add_to_free_list_bf);
}

void bf_free(void *ptr) {
  f_free(ptr, &add_to_free_list_ff);
}

unsigned long get_data_segment_size() { // in bytes
  return segment_size;
}

unsigned long get_data_segment_free_space_size() { // in bytes
  return segment_free_space_size;
}

void * f_malloc(size_t size, add_func_t f) {
  if (size == 0) {
    return NULL;
  }
  void * addr = NULL;
  if ((addr = try_existed_block(size, f)) == NULL) {
    // require a new block
    return add_new_block(size) + sizeof(meta_data_t);
  }
  assert(((meta_data_t *)addr)->is_used == 1);
  return addr + sizeof(meta_data_t);
}

void * try_existed_block(size_t size, add_func_t f) {
  meta_data_t * ptr = free_list_head;
  while (ptr != NULL) {
    assert(ptr->is_used == 0);
    if (ptr->size >= size) {
      break;
    }
  }
  // no available free block
  if (ptr == NULL) {
    return NULL;
  }
  remove_block(ptr);
  // need to seperate into two blocks
  if (ptr->size > size + sizeof(meta_data_t)) {
    meta_data_t* block2 = split_block(ptr, size);
    f(block2);
  }
  return ptr;
}

void * add_new_block(size_t size) {
  void * ptr = sbrk(size + sizeof(meta_data_t));
  if (ptr == (void * )-1) {
    return NULL;
  }
  meta_data_t * block_meta = (meta_data_t *) ptr;
  block_meta->size = size;
  block_meta->is_used = 1;
  block_meta->prev_block = block_tail;
  block_tail->next_block = block_meta;
  block_tail = block_meta;
  block_meta->next_block = NULL;
  block_meta->prev_free_block = NULL;
  block_meta->next_free_block = NULL;
  
  segment_size += (size + sizeof(meta_data_t));
  return ptr;
}

void f_free(void * ptr, add_func_t f) {
  if (ptr == NULL) {
    return;
  }
  assert ((unsigned long)ptr >= sizeof(meta_data_t));
  meta_data_t * block = (meta_data_t *) (ptr - sizeof(meta_data_t));
  assert(block->is_used == 1);
  try_coalesce(block, f);
}

void try_coalese(meta_data_t * block, add_func_t f) {
  meta_data_t * new_block = block;
  // need to delete, not always true
  assert(block->is_used == 1);
  if (block->is_used = 0) {
    remove_block(block);
  }
  assert(block->is_used == 1);
  if (block->next_block->is_used == 0 &&
      (void *)(block) + sizeof(meta_data_t) + block->size == block->next_block) {
    remove_block(block->next_block);
    block->size += (block->next_block->size + sizeof(meta_data_t));
    block->next_block->next_block->prev_block = block;
    block->next_block = block->next_block->next_block;
  }
  if (block->prev_block->is_used == 0 &&
      (void *)(block->prev_block) + sizeof(meta_data_t) + block->prev_block->size == block) {
    remove_block(block->prev_block);
    block->prev_block->size += (block->size + sizeof(meta_data_t));
    block->prev_block->next_block = block->next_block;
    block->next_block->prev_block = block->prev_block;
    new_block = block->prev_block;
  }
  f(new_block);
}

void add_to_free_list_ff(meta_data_t * block) {
}

void add_to_free_list_bf(meta_data_t * block) {
}

void remove_block(meta_data_t * block) {
}

meta_data_t * split_block(meta_data_t * block1, size_t size) {
  return NULL;
}
